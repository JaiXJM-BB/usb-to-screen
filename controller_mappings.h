#include "screen/screen.h"

#define PARSER_MODE_BUTTON 0
#define PARSER_MODE_ANALOG1x 1
#define PARSER_MODE_ANALOG1y 2
#define PARSER_MODE_ANALOG2x 3
#define PARSER_MODE_ANALOG2y 4

int check_allowed(int vid, int pid);

uint32_t (*get_parser(int vid, int pid))(int mode, int data_len, uint8_t * data);

/* Generic */
uint32_t prs_generic(int mode, int data_len, uint8_t * data);

/* Logitech */
// F310
uint32_t prs_v046d_pc21d(int mode, int data_len, uint8_t * data);
