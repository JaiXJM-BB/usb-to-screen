#include <stdlib.h>
#include "usb-to-screen.h"
#include "controller_mappings.h"


/* Screen Global Objects */
screen_context_t context;
screen_display_t display;
screen_event_t event;

/* usbd Global Objects */
struct usbd_connection *conn;
usbd_device_ident_t idents;
usbd_connect_parm_t parms;
usbd_funcs_t funcs;

/* storage of references */
combined_device_info_t* list;

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
	return 0;
} //init_screen

/**
 * Destroys all created screen objects
 */
void close_screen(){
	screen_destroy_event(event);
	screen_destroy_context(context);
} //close_screen

int init_usbd(int argc, char* argv[]){
	idents.vendor 	= 0x046d;
	idents.device 	= 0xc21d;
	idents.dclass 	= USBD_CONNECT_WILDCARD;
	idents.subclass = USBD_CONNECT_WILDCARD;
	idents.protocol = USBD_CONNECT_WILDCARD;

	funcs.nentries 	= _USBDI_NFUNCS;
	funcs.insertion = on_usbd_insert;
	funcs.removal 	= on_usbd_remove;
	funcs.event 	= on_usbd_event;

	parms.path = NULL;
	parms.vusb = USB_VERSION;
	parms.vusbd = USBD_VERSION;
	parms.flags = 0;
	parms.argc = argc;
	parms.argv = argv;
	parms.evtbufsz = 0;
	parms.ident = &idents;
	parms.funcs = &funcs;
	parms.connect_wait = USBD_CONNECT_WAIT;

	if(EOK != (errno = usbd_connect(&parms, &conn))) return -1;

	return 0;
}

void close_usbd(){
	usbd_disconnect(conn);
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
void on_usbd_insert(struct usbd_connection* conn, usbd_device_instance_t *inst){
	printf( "--- Insertion ---\n" );
	printf( " Path: %02x\n", inst->path );
	printf( " Devno: %02x\n", inst->devno );
	printf( " Generation: %04x\n", inst->generation );
	printf( " Vendor: %08x\n", inst->ident.vendor );
	printf( " Device: %08x\n", inst->ident.device );
	printf( " Class: %08x\n", inst->ident.dclass );
	printf( " Subclass: %08x\n", inst->ident.subclass );
	printf( " Protocol: %08x\n", inst->ident.protocol );
	printf( " Config: %08x\n", inst->config );
	printf( " Iface: %08x\n", inst->iface );
	printf( " Alternate: %08x\n\n", inst->alternate );

	struct usbd_urb* urb;
	struct usbd_pipe* pipe;
	struct usbd_device* device;
	struct usbd_descriptors_t* desc;
	struct usbd_desc_node* node;
	void* data;

	usbd_attach(conn, inst, 0, &device);

	desc = usbd_parse_descriptors(device, NULL, USB_DESC_DEVICE, 0, &node);
	if(!desc){ printf("Device fail \n"); return;}
	desc = usbd_parse_descriptors(device, node, USB_DESC_CONFIGURATION, 0, &node);
	if(!desc){ printf("conf fail \n"); return;}
	desc = usbd_parse_descriptors(device, node, USB_DESC_INTERFACE, 0, &node);
	if(!desc){ printf("interface fail \n"); return;}
	desc = usbd_parse_descriptors(device, node, USB_DESC_ENDPOINT, 1, &node);
	if(!desc){ printf("endpoint fail \n"); return;}

	urb = usbd_alloc_urb(NULL);
	data = usbd_alloc(32);

	usbd_setup_bulk(urb, URB_DIR_IN, data, 32); //guaranteed success

	if(usbd_open_pipe(device, desc, &pipe)){printf("Pipe open failure\n"); return;}

	combined_device_info_t* comb_data = calloc(1, sizeof(combined_device_info_t));
	comb_data->data = data;
	comb_data->data_len_expect = 32;
	comb_data->inst = inst;
	screen_create_device_type(&(comb_data->device), context, SCREEN_EVENT_GAMEPAD);

	usbd_io(urb, pipe, on_urb_receive, comb_data, USBD_TIME_DEFAULT); //if EOK process data
}

void on_usbd_remove(struct usbd_connection* conn, usbd_device_instance_t *inst){
	printf( "--- Removal ---\n" );
	printf( " Path: %02x\n", inst->path );
	printf( " Devno: %02x\n", inst->devno );
	printf( " Generation: %04x\n", inst->generation );
	printf( " Vendor: %08x\n", inst->ident.vendor );
	printf( " Device: %08x\n", inst->ident.device );
	printf( " Class: %08x\n", inst->ident.dclass );
	printf( " Subclass: %08x\n", inst->ident.subclass );
	printf( " Protocol: %08x\n", inst->ident.protocol );
	printf( " Config: %08x\n", inst->config );
	printf( " Iface: %08x\n", inst->iface );
	printf( " Alternate: %08x\n\n", inst->alternate );
}

void on_usbd_event (struct usbd_connection* conn, usbd_device_instance_t *inst, uint16_t type){

}

void fire_screen_event(combined_device_info_t* comb_dev){
	if(!comb_dev) return;
	if(!comb_dev->data) return;
	if(!comb_dev->device) return;
	if(!comb_dev->inst) return;

	int(*parser)(int mode, int data_len, uint8_t * data);
	parser = get_parser(comb_dev->inst->ident.vendor, comb_dev->inst->ident.device);

	uint32_t button = parser(PARSER_MODE_BUTTON, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);
	uint32_t analog1x = parser(PARSER_MODE_ANALOG1x, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);
	uint32_t analog1y = parser(PARSER_MODE_ANALOG1y, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);
	uint32_t analog2x = parser(PARSER_MODE_ANALOG2x, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);
	uint32_t analog2y = parser(PARSER_MODE_ANALOG2y, comb_dev->data_len_expect, (uint8_t*) comb_dev->data);

	const int type = SCREEN_EVENT_GAMEPAD;
	screen_set_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type);
	screen_set_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &button);
	screen_set_event_property_pv(event, SCREEN_PROPERTY_DEVICE, &(comb_dev->device));
	if(screen_inject_event(display, event)!=0) printf("Inject failed w errno %d\n", errno);
}

void on_urb_receive(struct usbd_urb* urb, struct usbd_pipe* pipe, void* user_data){
	uint8_t * data = (uint8_t *)(((combined_device_info_t*) user_data)->data);

	printf("Receive: ");
	for (int i = 0; i < 32; i++){
		printf("%02x", data[i]);
		if(i%4==3) printf(" ");
	}
	printf("\n");

	fire_screen_event(((combined_device_info_t*) user_data));
	
	usbd_setup_bulk(urb, 
					URB_DIR_IN, 
					((combined_device_info_t*) user_data)->data, 
					((combined_device_info_t*) user_data)->data_len_expect
	);
	usbd_io(urb, pipe, on_urb_receive, user_data, USBD_TIME_INFINITY);
}

void usb_to_screen_signal_handler(int signo){
	close_screen();
	close_usbd();
	_exit(0);
}


/**
 * main function of usb-to-screen
 */
int main(int argc, char* argv[]){
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

	//Processing Loop
	for( ; ; ) sleep(60);

	//Allocate a urb block
	
	//Set urb for bulk input
	//30 is arbitrary
	

	//submit urb to the USB stack for processing
	//need a pipe somehow
	//the urb is a pointer to shared memory
	

	//end of processing loop

	close_screen();
	close_usbd();
	
	return 0;
} //main