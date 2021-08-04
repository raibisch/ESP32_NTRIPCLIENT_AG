#ifndef JGSDCARD_H_
#define JGSDCARD_H_


#include "global_vars.h"

extern void sd_init();
extern bool sd_writefile(fs::FS &fs, const char * path, String s);
extern bool sd_readfile(fs::FS &fs, const char * path);

#endif