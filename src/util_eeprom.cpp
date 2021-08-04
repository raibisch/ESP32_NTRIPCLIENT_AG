
#include <Arduino.h>
#include <EEPROM.h>
#include "global_vars.h"
#include "uti_dbg.h"

//--------------------------------------------------------------
//  EEPROM Data Handling
//--------------------------------------------------------------
#define EEPROM_SIZE 1024
#define EE_ident1 0xED  // Marker Byte 0 + 1
#define EE_ident2 0xED

//--------------------------------------------------------------
byte EEprom_empty_check(){
    
  if (!EEPROM.begin(EEPROM_SIZE))  
  {
     DBG("failed to initialise EEPROM\n"); delay(1000);
     return 0;
  }
  
  if (EEPROM.read(0)!= EE_ident1 || EEPROM.read(1)!= EE_ident2)
  {
     DBG("Empty EEPROM\n");
     return 1;  // is empty
  }

  DBG(" EEPROM has valid init-data\n"); delay(1000);
  return 2;

 }


//--------------------------------------------------------------
void EEprom_write_all()
{  // called if EEPROM empty
  EEPROM.write(0, EE_ident1);
  EEPROM.write(1, EE_ident2);
  EEPROM.write(2, 0); // reset Restart blocker
  EEPROM.put(3, NtripSettings);
  EEPROM.commit();
}

//--------------------------------------------------------------
void EEprom_read_all()
{
  
  EEPROM.get(3, NtripSettings);
  
}
//--------------------------------------------------------------
//  Restore EEprom Data
//--------------------------------------------------------------
void restoreEEprom()
{
  // by JG z.Z. kein Input für restoreDefault
  //byte get_state  = digitalRead(restoreDefault_PIN);
  bool get_state = false;
  if (debugmode) 
  {
    get_state = true;
  }
  
  if (get_state)
   DBG("State: restoring default values !\n");
  else 
   DBG("State: read default values from EEPROM\n");
  
  if (EEprom_empty_check()==1 || get_state) 
  { //first start?
    EEprom_write_all();     //write default data
  }

  if (EEprom_empty_check()==2) { //data available
    EEprom_read_all();
   }
  //EEprom_show_memory();  //
  EE_done =1;   
}

//--------------------------------------------------------------
void EEprom_show_memory(){
byte c2=0, data_;
  DBG(EEPROM_SIZE, 1);
  DBG(" bytes read from Flash . Values are:\n");
  for (int i = 0; i < EEPROM_SIZE; i++)
  { 
    data_=byte(EEPROM.read(i));
    if (data_ < 0x10) Serial.print("0");
    DBG(data_,HEX); 
    if (c2==15) {
       DBG(" ");
      }
    else if (c2>=31) {
           DBG("",1); //NL
           c2=-1;
          }
    else DBG(" ");
    c2++;
  }
}







   
