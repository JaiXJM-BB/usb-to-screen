#include "screen/screen.h"
#include "sys/usbdi.h"
#include "sys/hiddi.h"
#include "errno.h"

/* Initialization and Shutdown functions */
int  init_screen ();
void close_screen();
int  init_usbd   ();
void close_usbd  ();
int  init_hidd	 ();
void close_hidd	 ();

/* Callbacks */
void on_usbd_insert(struct usbd_connection* conn, usbd_device_instance_t *inst);
void on_usbd_remove(struct usbd_connection* conn, usbd_device_instance_t *inst);
void on_usbd_event (struct usbd_connection* conn, usbd_device_instance_t *inst, uint16_t type);

void on_urb_receive(struct usbd_urb* urb, struct usbd_pipe* pipe, void* user_data);

/* Struct for mapping */
typedef struct _device_info combined_device_info_t;

struct _device_info{
	usbd_device_instance_t* inst;
	struct usbd_device** attached;
	struct usbd_urb* urb;
	struct usbd_pipe* pipe;

	screen_device_t device;

	int data_len_expect;
	void* data;

	combined_device_info_t* next;
};

/* Helpers */
void fire_screen_event(combined_device_info_t* comb_data);
void update_display();
