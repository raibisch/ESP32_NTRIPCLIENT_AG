
#include <Arduino.h>
//--------------------------------------------------------------
//  Debug Messaging
//--------------------------------------------------------------


bool debug = 1;  // Print Debug Messages to Serial0

void DBG(
    String out, byte nl){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}

void DBG(int out, byte nl){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}

void DBG(long out, byte nl){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}

void DBG(char out, byte nl){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}

void DBG(char out, byte type, byte nl){ // type = HEX,BIN,DEZ..
  if (debug == 1) {
    if (nl) Serial.println(out,type);
    else Serial.print(out,type);
  }
}

void DBG(IPAddress out, byte nl){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}