#include "controller_mappings.h"

#define CONTROLLER_COUNT 2
const int _allowed_controllers[CONTROLLER_COUNT][2] = {
	{0x046d,0xc21d}, //F310
	{0x046d,0xc20c} //WingMan Precision
};

int check_allowed(int vid, int pid){
	for (int i = 0; i < CONTROLLER_COUNT; i++){
		if(_allowed_controllers[i][0] == vid && _allowed_controllers[i][1] == pid) return 0;
	}
	return -1;
}

/* Parser Control */
uint32_t (*get_parser(int vid, int pid))(int mode, int data_len, uint8_t * data){
	if(vid == 0x046d && pid == 0xc21d) return prs_v046d_pc21d;
	if(vid == 0x046d && pid == 0xc20c) return prs_v046d_pc20c;
	return prs_generic;
}

//#####################################################################

/* Generic */
uint32_t prs_generic(int mode, int data_len, uint8_t * data){
	printf("Generic Parser Call; Device not Supported.\n");
	return 0;
}

/* Logitech */
// F310
uint32_t prs_v046d_pc21d(int mode, int data_len, uint8_t * data){
	if(data_len < 14) return 0;

	switch(mode){
		case PARSER_MODE_BUTTON:
			uint32_t button = 0;
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
			return button;
		case PARSER_MODE_ANALOG1x:
			return (data[6] << 8) | data[7];
		case PARSER_MODE_ANALOG1y:
			return (data[8] << 8) | data[9];
		case PARSER_MODE_ANALOG2x:
			return (data[10] << 8) | data[11];
		case PARSER_MODE_ANALOG2y:
			return (data[12] << 8) | data[13];
	}
	return 0;
}

//WingMan Precision
uint32_t prs_v046d_pc20c(int mode, int data_len, uint8_t * data){
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