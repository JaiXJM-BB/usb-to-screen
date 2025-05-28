#include "parser.h"
extern int verbose;

/* To Any Future Devs: 
 * You can extend the list of supported controllers below by incrementing CONTROLLER_COUNT
 * and then adding its VID, PID, the size of the Joystick range (as if it were unsigned, 
 * eg. -1 to 2 would be 0 to 4, so 4 would be the size), and the parser reference.
 * 
 * Once you add the controller here, make sure to add a parser function below. Otherwise it will default to generic.
 */
#define CONTROLLER_COUNT 2
const struct _device_lookup_storage _device_lookup[CONTROLLER_COUNT][3] = {
/*	 VID   | PID   | Joystick Res | Parser Function  */
	{0x046d, 0xc21d, 0x10000      , prs_v046d_pc21d}, //F310
	{0x046d, 0xc20c, 0x100        , prs_v046d_pc20c}  //WingMan Precision
};


/* Parser Control */
int (*get_parser(int vid, int pid))(int mode, int data_len, uint8_t * data){
	if(verbose) printf("Getting parser for %08d %08d.\n", vid, pid);
	for(int i = 0; i< CONTROLLER_COUNT; i++){
		if(_device_lookup[i]->vid == vid && _device_lookup[i]->pid == pid){
			if(verbose) printf("Specialized Parser Found.\n");
			return _device_lookup[i]->parser;
		}
	}
	if(verbose) printf("Generic Parser Found.\n");
	return prs_generic;
}

/**
 * Returns Joystick Size if valid, -1 if invalid
 */
int check_allowed(int vid, int pid){
	for (int i = 0; i < CONTROLLER_COUNT; i++){
		if(_device_lookup[i]->vid == vid && _device_lookup[i]->pid == pid) return _device_lookup[i]->js_res;
	}
	return -1;
}

//#######################################
//########## Parsing Functions ##########
//#######################################
/* To Future Devs:
 * Parser functions should be placed here.
 * They take in an int `mode`, int `data_len`, and a uint8_t* data buffer, sized to data_len. 
 * Return 0 if invalid.
 * `mode` is one of the following:
 * PARSER_MODE_BUTTON, in which it should return all button input sorted as screen game buttons
 * PARSER_MODE_ANALOG1x,
 * PARSER_MODE_ANALOG1y,
 * PARSER_MODE_ANALOG2x,
 * PARSER_MODE_ANALOG2y.
 */

/* Generic */
int prs_generic(int mode, int data_len, uint8_t * data){
	if(verbose >=3) printf("Generic Parser Call; Device not Supported.\n");
	return 0;
}

/* Logitech */
// F310
int prs_v046d_pc21d(int mode, int data_len, uint8_t * data){
	if(data_len < 14) return 0;
	int to_return = 0;

	switch(mode){
		case PARSER_MODE_BUTTON:
			uint32_t button = 0;
			if(verbose >= 3)
				printf("buttons q:");

			button |= (SCREEN_A_GAME_BUTTON  * ((data[3]&0x10)?1:0))
					| (SCREEN_B_GAME_BUTTON  * ((data[3]&0x20)?1:0))
					| (SCREEN_X_GAME_BUTTON  * ((data[3]&0x40)?1:0))
					| (SCREEN_Y_GAME_BUTTON  * ((data[3]&0x80)?1:0))
					| (SCREEN_L1_GAME_BUTTON * ((data[3]&0x01)?1:0))
					| (SCREEN_L2_GAME_BUTTON * ((data[4]&0xFF)?1:0))
					| (SCREEN_L3_GAME_BUTTON * ((data[2]&0x40)?1:0))
					| (SCREEN_R1_GAME_BUTTON * ((data[3]&0x02)?1:0))
					| (SCREEN_R2_GAME_BUTTON * ((data[5]&0xFF)?1:0))
					| (SCREEN_R3_GAME_BUTTON * ((data[2]&0x80)?1:0))
					| (SCREEN_DPAD_UP_GAME_BUTTON * ((data[2]&0x01)?1:0))
					| (SCREEN_DPAD_DOWN_GAME_BUTTON * ((data[2]&0x02)?1:0))
					| (SCREEN_DPAD_LEFT_GAME_BUTTON * ((data[2]&0x04)?1:0))
					| (SCREEN_DPAD_RIGHT_GAME_BUTTON * ((data[2]&0x08)?1:0))
					| (SCREEN_MENU1_GAME_BUTTON * ((data[2]&0x10)?1:0))
					| (SCREEN_MENU2_GAME_BUTTON * ((data[2]&0x20)?1:0))
					| (SCREEN_MENU3_GAME_BUTTON * ((data[3]&0x04)?1:0));
			to_return =  button;
			break;
		case PARSER_MODE_ANALOG1x:
			if(verbose >= 3)
				printf("analog1x:");
			to_return = (data[6] *0x100) + data[7]  - 32768;
			break;
		case PARSER_MODE_ANALOG1y:
			if(verbose >= 3)
				printf("analog1y:");
			to_return = (data[8] *0x100) + data[9]  - 32768;
			break;
		case PARSER_MODE_ANALOG2x:
			if(verbose >= 3)
				printf("analog2x:");
			to_return = (data[10] *0x100) + data[11]  - 32768;
			break;
		case PARSER_MODE_ANALOG2y:
			if(verbose >= 3)
				printf("analog2y:");
			to_return = (data[12] *0x100) + data[13]  - 32768;
			break;
	}
	if(verbose >= 3)
		printf(" %08x\n", to_return);
	return to_return;
}

//WingMan Precision
int prs_v046d_pc20c(int mode, int data_len, uint8_t * data){
	if(data_len < 3) return 0;

	switch(mode){
		case PARSER_MODE_BUTTON:
			uint32_t button = 0;
			button |= (SCREEN_A_GAME_BUTTON  * ((data[0]&0x01)?1:0))
					| (SCREEN_B_GAME_BUTTON  * ((data[0]&0x02)?1:0))
					| (SCREEN_X_GAME_BUTTON  * ((data[0]&0x04)?1:0))
					| (SCREEN_Y_GAME_BUTTON  * ((data[0]&0x08)?1:0))
					| (SCREEN_L1_GAME_BUTTON * ((data[0]&0x10)?1:0))
					| (SCREEN_R1_GAME_BUTTON * ((data[0]&0x20)?1:0));
			return button;
		case PARSER_MODE_ANALOG1x:
			return data[1];
		case PARSER_MODE_ANALOG1y:
			return data[2];
		default:
			return 0x80; //no input - middle.
		}
	return 0;
}