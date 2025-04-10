#include "screen/screen.h"

#define PARSER_MODE_BUTTON 0
#define PARSER_MODE_ANALOG1x 1
#define PARSER_MODE_ANALOG1y 2
#define PARSER_MODE_ANALOG2x 3
#define PARSER_MODE_ANALOG2y 4

int check_allowed(int vid, int pid);

int (*get_parser(int vid, int pid))(int mode, int data_len, uint8_t * data);

/* Generic */
int prs_generic(int mode, int data_len, uint8_t * data);

/* Logitech */
// F310
int prs_v046d_pc21d(int mode, int data_len, uint8_t * data);
// WingMan Precision
int prs_v046d_pc20c(int mode, int data_len, uint8_t * data);
