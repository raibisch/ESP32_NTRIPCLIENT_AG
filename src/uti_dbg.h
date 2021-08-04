#ifndef UTI_DBG_H_
#define UTI_DBG_H_


#include <Arduino.h>

extern bool debug;
extern void DBG(String out, byte nl = 0);
extern void DBG(int out, byte nl = 0);
extern void DBG(long out, byte nl = 0);
extern void DBG(char out, byte nl = 0);
extern void DBG(char out, byte type, byte nl = 0); // type = HEX,BIN,DEZ..
extern void DBG(IPAddress out, byte nl = 0);


#endif