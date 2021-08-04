#ifndef DISPLAY_H_
#define DISPLAY_H_


#include "global_vars.h"

extern void display_clear();
extern void display_init();
extern void display_display();
extern void display_text(byte line_nr, String s);
extern void display_text(byte line_nr, String s, uint32_t color);
extern void display_error(String s);

#endif

