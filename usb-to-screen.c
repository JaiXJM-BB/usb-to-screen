#include <stdlib.h>
#include "usb-to-screen.h"
#include "parser.h"
int verbose = 0;

/* Screen Global Objects */
screen_context_t context;
screen_display_t display;
screen_event_t event;

/* usbd Global Objects */
struct usbd_connection *usb_conn;
usbd_device_ident_t usb_idents;
usbd_connect_parm_t usb_parms;
usbd_funcs_t usb_funcs;

/* hidd Global Objects */
struct hidd_connection *hid_conn;
hidd_device_ident_t hid_idents;
hidd_connect_parm_t hid_parms;
hidd_funcs_t hid_funcs;

/* synchronization */
pthread_mutex_t insert_mutex;

/*Combined Data Storage*/
combined_device_info_t * device_info_registry = NULL;

//#########################################
/**
 * Initializes the relevant screen objects
 */
int init_screen(){
	//Context
	if(screen_create_context(&context, SCREEN_INPUT_PROVIDER_CONTEXT)){
		return -1;
	}
	//Event
	if(screen_create_event(&event)){ 
		screen_destroy_context(context);
		return -1;
	}
	update_display();
} //init_screen

/**
 * Destroys all created screen objects
 */
void close_screen(){
	screen_destroy_event(event);
	screen_destroy_context(context);
} //close_screen

int init_usbd(int argc, char* argv[]){
	usb_idents.vendor 	= USBD_CONNECT_WILDCARD;
	usb_idents.device 	= USBD_CONNECT_WILDCARD;
	usb_idents.dclass 	= USBD_CONNECT_WILDCARD;
	usb_idents.subclass = USBD_CONNECT_WILDCARD;
	usb_idents.protocol = USBD_CONNECT_WILDCARD;

	usb_funcs.nentries 	= _USBDI_NFUNCS;
	usb_funcs.insertion = on_usbd_insert;
	usb_funcs.removal 	= on_usbd_remove;
	usb_funcs.event 	= on_usbd_event;

	usb_parms.path = NULL;
	usb_parms.vusb = USB_VERSION;
	usb_parms.vusbd = USBD_VERSION;
	usb_parms.flags = 0;
	usb_parms.argc = argc;
	usb_parms.argv = argv;
	usb_parms.evtbufsz = 0;
	usb_parms.ident = &usb_idents;
	usb_parms.funcs = &usb_funcs;
	usb_parms.connect_wait = USBD_CONNECT_WAIT;

	if(EOK != (errno = usbd_connect(&usb_parms, &usb_conn))) return -1;

	return 0;
}

void close_usbd(){
	usbd_disconnect(usb_conn);
}

int init_hidd(){
	hid_idents.product_id	= HIDD_CONNECT_WILDCARD;
	hid_idents.vendor_id	= HIDD_CONNECT_WILDCARD;
	hid_idents.version		= HIDD_CONNECT_WILDCARD;

	hid_funcs.nentries 		= _HIDDI_NFUNCS;
	hid_funcs.insertion		= on_hidd_insert;
	hid_funcs.removal 		= on_hidd_remove;
	hid_funcs.event			= on_hidd_async;
	hid_funcs.report		= on_hidd_report;

	hid_parms.path 			= NULL;
	hid_parms.vhid 			= HID_VERSION;
	hid_parms.vhidd 		= HIDD_VERSION;
	hid_parms.flags			= 0;
	hid_parms.evtbufsz		= 0;
	hid_parms.device_ident	= &hid_idents;
	hid_parms.funcs			= &hid_funcs;
	hid_parms.connect_wait  = HIDD_CONNECT_WAIT;

	if(EOK != (errno = hidd_connect(&hid_parms, &hid_conn))) return -1;
	
	return 0;
}

void close_hidd(){
	hidd_disconnect(hid_conn);
}

//###################################
void update_display(){
	screen_display_t* disps;
	int disp_count;

	screen_get_context_property_iv(context, SCREEN_PROPERTY_DISPLAY_COUNT, &disp_count);
	disps = calloc(disp_count, sizeof(screen_display_t));
	screen_get_context_property_pv(context, SCREEN_PROPERTY_DISPLAYS, disps);
	display = disps[0];
	free(disps);
}

//###################################
void on_hidd_insert(struct hidd_connection *conn, hidd_device_instance_t *inst){

}

void on_hidd_remove(struct hidd_connection *conn, hidd_device_instance_t *inst){
	if(verbose) printf("Attempting removal from HID\n");
	hidd_reports_detach(conn, inst);
}

void on_hidd_async (struct hidd_connection *conn, hidd_device_instance_t *inst, _uint16 type){}

void on_hidd_report(struct hidd_connection *conn, struct hidd_report *report, void *data, _uint32 len, _uint32 flags, void *user_data){

}

void on_usbd_insert(struct usbd_connection* conn, usbd_device_instance_t *inst){
	pthread_mutex_lock(&insert_mutex);

	if(verbose) 
		printf("Attempting to attach to %x %x via USB... \n", inst->ident.vendor, inst->ident.device);

	if(check_allowed(inst->ident.vendor, inst->ident.device) == -1){ 
		if(verbose) printf("Attach failure: not supported.\n");
		return;
	}

	struct usbd_device* device = NULL;
	struct usbd_desc_node* node, *junk;

	int ret = usbd_attach(conn, inst, 0, &device);
	if(ret!=EOK){
		printf("Error in device attach %d\n", ret); 
		pthread_mutex_unlock(&insert_mutex); 
		return;
	}

	usbd_device_descriptor_t* device_desc = usbd_device_descriptor(device, &node);

	for(int confno = 1; confno <= device_desc->bNumConfigurations; confno++){
		usbd_configuration_descriptor_t* conf_desc = 
			usbd_configuration_descriptor(device, confno, &node);

		for(int intfno = 0; intfno < conf_desc->bNumInterfaces; intfno++){
			usbd_interface_descriptor_t* intf_conf = 
				usbd_interface_descriptor(device, confno, intfno, 0, &node);

			for(int endno = 0; endno <= intf_conf->bNumEndpoints; endno++){
				if(endno % 2 == 0) continue; //only odd(input) endpoints

				struct usbd_descriptors_t* desc = usbd_parse_descriptors(device, node, USB_DESC_ENDPOINT, endno, &junk);
				if(desc){
					combined_device_info_t* comb_data = calloc(1, sizeof(combined_device_info_t));
					comb_data->vid = inst->ident.vendor;
					comb_data->pid = inst->ident.device;
					if(verbose) printf("Setting up combined data for %08x %08x\n", comb_data->vid, comb_data->pid);
					comb_data->attached = device;
					comb_data->joystick_size = check_allowed(inst->ident.vendor, inst->ident.device);

					comb_data->data_len_expect = ((usbd_endpoint_descriptor_t*) desc)->wMaxPacketSize;
					comb_data->urb = usbd_alloc_urb(NULL);
					comb_data->data = usbd_alloc(comb_data->data_len_expect);
					
					usbd_setup_bulk(comb_data->urb, URB_DIR_IN, comb_data->data, comb_data->data_len_expect);
					if(usbd_open_pipe(device, desc, &(comb_data->pipe))){
						usbd_free(comb_data->data);
						usbd_free_urb(comb_data->urb);
						continue;
					}

					const int button_count = 19;
					screen_create_device_type(&(comb_data->device), context, SCREEN_EVENT_GAMEPAD);
					screen_set_device_property_iv(comb_data->device, SCREEN_PROPERTY_PRODUCT, &(inst->ident.device));
					screen_set_device_property_iv(comb_data->device, SCREEN_PROPERTY_VENDOR, &(inst->ident.vendor));
					screen_set_device_property_iv(comb_data->device, SCREEN_PROPERTY_BUTTON_COUNT, &button_count);

					comb_data->next = device_info_registry;
					device_info_registry = comb_data;

					if(verbose) printf("Seting up io callback... \n");
					usbd_io(comb_data->urb, comb_data->pipe, on_urb_receive, comb_data, USBD_TIME_DEFAULT); 
				}//Check
			}//Endpoint
		}//Interface
	}//Config
	if(verbose) printf("Attach Success.\n");
	pthread_mutex_unlock(&insert_mutex);
}//Function

void on_usbd_remove(struct usbd_connection* conn, usbd_device_instance_t *inst){
	struct usbd_device * device;
    device = usbd_device_lookup(conn, inst);
	if(verbose) printf("Attempting to remove %d %d on USB... \n", inst->ident.vendor, inst->ident.device);

	if(device == NULL){
        // Handle a case where this device wasn't attached (do nothing)
    }else{

		if(verbose >=3 ) printf("Device is attached, proceeding wih removal...\n");
		combined_device_info_t ** comb_parser;
		comb_parser = &device_info_registry;

		combined_device_info_t *temp, *prev;
		temp = *comb_parser;

		if(temp != NULL && temp->pid == inst->ident.device && temp->vid == inst->ident.vendor){
			*comb_parser = temp->next;
			fire_screen_event_close(temp);
			if(verbose >=3 ) printf("Found data at the start of the list. Destroying Device...\n");
			screen_destroy_device(temp->device);
			if(verbose >=3 ) printf("Freeing URB...\n");
			usbd_free_urb(temp->urb);
			if(verbose >=3 ) printf("Freeing data buffer...\n");
			usbd_free(temp->data); //usbd_mphys is no longer needed in QNX 8
			if(verbose >=3 ) printf("Freeing combined data struct...\n");
			free(temp);
			if(verbose >=3 ) printf("Completed.\n");
		}else{
			while(temp != NULL && temp->pid != inst->ident.device && temp->vid != inst->ident.vendor){
				prev = temp;
				temp = temp->next;
			}

			if(temp != NULL){
				fire_screen_event_close(temp);
				if(verbose >=3 ) printf("Found data mid list. Destroying Device...\n");
				screen_destroy_device(temp->device);
				if(verbose >=3 ) printf("Freeing URB...\n");
				usbd_free_urb(temp->urb);
				if(verbose >=3 ) printf("Freeing data buffer...\n");
				usbd_free(temp->data); //usbd_mphys is no longer needed in QNX 8
				if(verbose >=3 ) printf("Freeing combined data struct...\n");
				free(temp);
				if(verbose >=3 ) printf("Completed.\n");
			}
		}

        usbd_detach(device);
    }
}

void on_usbd_event (struct usbd_connection* conn, usbd_device_instance_t *inst, uint16_t type){}

void fire_screen_event(combined_device_info_t* comb_dev){
	if(!comb_dev) return;
	if(!comb_dev->data) return;
	if(!comb_dev->device) return;

	int(*parser)(int mode, int data_len, uint8_t * data);
	parser = get_parser(comb_dev->vid, comb_dev->pid);

	int analog0[3], analog1[3], button;
	button     = parser(PARSER_MODE_BUTTON,   comb_dev->data_len_expect, (uint8_t*) comb_dev->data);
	analog0[0] = parser(PARSER_MODE_ANALOG1x, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);
	analog0[1] = parser(PARSER_MODE_ANALOG1y, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);
	analog1[0] = parser(PARSER_MODE_ANALOG2x, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);
	analog1[1] = parser(PARSER_MODE_ANALOG2y, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);

	if(verbose >= 2)
		printf("A0: %d %d A1: %d %d\n", analog0[0], analog0[1], analog1[0], analog1[1]);
	

	analog0[2] = 0;
	analog1[2] = 0;

	const int type = SCREEN_EVENT_GAMEPAD;
	screen_set_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type);
	screen_set_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &button);
	screen_set_event_property_iv(event, SCREEN_PROPERTY_ANALOG0, &analog0);
	screen_set_event_property_iv(event, SCREEN_PROPERTY_ANALOG1, &analog1);
	screen_set_event_property_pv(event, SCREEN_PROPERTY_DEVICE, &(comb_dev->device));
	screen_set_event_property_iv(event, SCREEN_PROPERTY_SIZE, &(comb_dev->joystick_size));
	if(screen_inject_event(display, event)!=0) printf("Inject failed w errno %d\n", errno);
}

void fire_screen_event_hid();

void fire_screen_event_close(combined_device_info_t* comb_dev){
	if(!comb_dev) return;
	if(!comb_dev->data) return;
	if(!comb_dev->device) return;

	const int type = SCREEN_EVENT_CLOSE;
	const int attached = 0;
	screen_set_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type);
	screen_set_event_property_iv(event, SCREEN_PROPERTY_ATTACHED, &attached);
	screen_set_event_property_pv(event, SCREEN_PROPERTY_DEVICE, &(comb_dev->device));
	screen_set_event_property_iv(event, SCREEN_PROPERTY_SIZE, &(comb_dev->joystick_size));
	if(screen_inject_event(display, event)!=0) printf("Inject failed w errno %d\n", errno);
}

void on_urb_receive(struct usbd_urb* urb, struct usbd_pipe* pipe, void* user_data){
	uint8_t * data = (uint8_t *)(((combined_device_info_t*) user_data)->data);

	if(verbose >= 2){
		printf("Receive: ");
		for (int i = 0; i < ((combined_device_info_t*) user_data)->data_len_expect; i++){
			printf("%02x", data[i]);
			if(i%4==3) printf(" ");
		}
		printf("\n");
    }

	fire_screen_event(((combined_device_info_t*) user_data));
	
	usbd_setup_bulk(urb, 
					URB_DIR_IN, 
					((combined_device_info_t*) user_data)->data, 
					((combined_device_info_t*) user_data)->data_len_expect
	);
	usbd_io(urb, pipe, on_urb_receive, user_data, USBD_TIME_INFINITY);
}

void usb_to_screen_signal_handler(int signo){
	/* Clean Combined Data */
	combined_device_info_t * temp, * next;
	next = device_info_registry;
	device_info_registry = NULL;

	if(verbose >=3 ) printf("Clearing All Data...\n");
	while(next != NULL){
		temp = next;
		next = next->next;

		fire_screen_event_close(temp);
		if(verbose >=3 ) printf("Destroying Device...\n");
		screen_destroy_device(temp->device);
		if(verbose >=3 ) printf("Freeing URB...\n");
		usbd_free_urb(temp->urb);
		if(verbose >=3 ) printf("Freeing data buffer...\n");
		usbd_free(temp->data); //usbd_mphys is no longer needed in QNX 8
		if(verbose >=3 ) printf("Freeing combined data struct...\n");
		free(temp);
		if(verbose >=3 ) printf("Completed.\n");
	}
	
	/* Closing Functions */
	close_screen();
	close_usbd();
	close_hidd();
	_exit(0);
}


/**
 * main function of usb-to-screen
 */
int main(int argc, char* argv[]){
	//Process options (Verbosity)
	int opt;
	while((opt = getopt(argc, argv, "V")) !=- 1) verbose += (opt == 'V');
	if(verbose > 0) printf("Verbosity Enabled: Level %d\n", verbose);

	//synchronization
	if(pthread_mutex_init(&insert_mutex, NULL)!=0) return 1;

	//Initialize the system
	if(init_screen() == -1) return 1;
	if(init_usbd(argc, argv) == -1){
		close_screen();
		return 1;
	}

	//Signals
	signal (SIGHUP, SIG_IGN);
	signal (SIGPWR, SIG_IGN);
	signal (SIGTERM, usb_to_screen_signal_handler);
	signal (SIGKILL, usb_to_screen_signal_handler);

	//Loop until SIGTERM/SIGKILL
	for( ; ; ) sleep(60);

	//Close using signal handler.
	usb_to_screen_signal_handler(SIGKILL);
	
	return 0;
} //main