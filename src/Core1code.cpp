//Core1:  NTRIP Client Code
#include <Arduino.h>
#include <base64.h>
#include <WiFi.h>
#include "global_vars.h"
#include "uti_dbg.h"
#include "Network_AOG.h"
#include "Display.h"
#include "SDCard.h"

unsigned long lTime =0;

void Core1code( void * pvParameters )
{

  DBG("\nTask1 running on core ");
  DBG((int)xPortGetCoreID(),1);


  while (!EE_done)
  {  // wait for eeprom data
     delay(10);
  }
  
  // by JG: zusaetzliche Abfrage auf "wifi_connect", da bis zum Aufbau der WiFi-Verbindung sonst auf uninitialisertes " ntripCl" zugegriffen wird
  while ((my_WiFi_Mode != WIFI_STA) || (!wifi_connected))
  {
     DBG("Waiting for WiFi Access\n");
     delay(1000);
   }



  lifesign  = millis();  //init timers 
  repeatGGA = millis();  // 
  refreshDisplay = millis();

  DBG("\nRTCM/NMEA Baudrate: ");
  DBG(NtripSettings.baudOut, 1);

  DBG("\nNTRIP-Enable: ");
  DBG(NtripSettings.enableNtrip , 1);
  

  for(;;)
  { // MAIN LOOP FOR THIS CORE
   // by JG: zusaetzliche Abfrage auf "wifi_connect", da bis zum Aufbau der WiFi-Verbindung sonst auf uninitialisertes " ntripCl" zugegriffen wird
   if ((NtripSettings.enableNtrip==1) && (my_WiFi_Mode == WIFI_STA) && (wifi_connected))
   {
     if (restart == 0)
     {
        DBG("\nRequesting Sourcetable:\n\n");
        if(!getSourcetable()) DBG("SourceTable request error !!\n");
        DBG("try starting RTCM stream !!!!!\n");
        if (!startStream()) DBG("Stream request error\n");
        DBG("\n*** RTCM stream started at serial1 (* = RTCM-Package,  G = GGA-sent)\n");
        restart = 1;
     }     
     if (!getRtcmData())
     {
        //DBG("\nstopped receiving data \n");
        DBG("\n*** Can not reach hoster, internet connection broken\n");
        delay(5000);
        DBG("\nTrying to reconnect\n"); 
        restart=0;
     }  
   }
   else  
   {
    delay(1);
    if (NtripSettings.enableNtrip == 0)
    {
     DBG("\n*** ESP Ntrip Client is switched OFF\n");
     NtripRunState ='0';
     if (my_WiFi_Mode != WIFI_STA) DBG("No WiFi connection\n");
     delay(1000);
    }
  }
   
  // by JG zuerst den WiFi-status abfragen !! 
  if ((WiFi.status() != WL_CONNECTED && wifi_connected)) 
  {
    my_WiFi_Mode = 0;
    wifi_connected = false;
    DBG("WiFi offline, trying to reconnect\n");
    WiFi_Start_STA();
    if (my_WiFi_Mode == WIFI_STA) 
    {
     restart = 0;
    }  
  }
  
   

 } // End of (for ever)
} // End of core1code

//###########################################################################################
