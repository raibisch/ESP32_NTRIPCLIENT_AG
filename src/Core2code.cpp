#include <Arduino.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include "global_vars.h"

#include "uti_dbg.h"
#include "Network_AOG.h"
#include "TinyGPS++.h"
#include "Display.h"
#include "SDCard.h"


//Core2: this task serves the Webpage and handles the GPS NMEAs

// by JG wird bei mir nicht gebraucht
//AsyncUDP udpRoof;



#ifdef USE_BLUETOOTH
BluetoothSerial SerialBT;
#endif

int case_count=0;

//bool bt_rx_connected = false;

void SerialBT_Traffic()
{
 
 if (SerialBT.hasClient())
 {
   bluetooth_connected = true;
   while (SerialBT.available())
   {
     char cBTread = SerialBT.read();
     if (cBTread == -1)
     {
         SerialBT.disconnect();
         bluetooth_connected = false;
         DBG("*** ERROR-Read-BT");
     }

     Serial1.write(cBTread);
   }
 }
 else 
 {
   if (bluetooth_connected == true)
   {
     bluetooth_connected = false;
     SerialBT.disconnect();
   }
   //SerialBT.disconnect();
   if (NtripSettings.send_NMEA == 1)
   {
     while (Serial.available())
     {
       Serial1.write(Serial.read());
     }
   }
 }
 
}


int i_traffic=0;
static char c;
static bool b_NMEA_connected = false;
//------------------------------------------------------------------------------------------
//Read serial GPS data
//-------------------------------------------------------------------------------------------
void Serial_GPS_TXD()
{

  byte cnt = 0;
  while (Serial1.available()) 
  { 
   cnt++;
   if (cnt > 100)
   {
     break;
   }
   c = Serial1.read();
   gps.encode(c);
  
#ifdef USE_BLUETOOTH
   if( SerialBT.hasClient() )
   {
      if (!bluetooth_connected)
      {
         SerialBT.setTimeout(1000);
         DBG("*** START-BT");
         debug = 1;
         bluetooth_connected = true;
         //Serial.flush();
         //delay(10);
      }  

      SerialBT.write(c);

      if (SerialBT.getWriteError() > 0)
      {
        DBG("*** WRITE-ERROR-BT");
        SerialBT.clearWriteError();
        SerialBT.disconnect();
        delay(1);
        bluetooth_connected = false;
      }
          
  }
  else if (!b_NMEA_connected)
  {
    /*
     if (tcpNMEAserver.hasClient())
     {
      b_NMEA_connected = true;
      tcpNMEAclient = tcpNMEAserver.available();
      //tcpNMEAclient.setTimeout(2);
      //tcpNMEAclient.setNoDelay(true); // by JG
      Serial1.flush();
      tcpNMEAserver.setTimeout(1000);
      //tcpNMEAserver.setNoDelay(true);
      tcpNMEAclient.setTimeout(1000);
      //tcpNMEAclient.setNoDelay(true);
      DBG("*** tcpNMEA connected...");
     }
     */
  }
  else
#endif
  {
      if (bluetooth_connected) 
      { 
        SerialBT.disconnect();
        SerialBT.clearWriteError();
        bluetooth_connected = false;
        DBG("*** END-BT");
        delay(10);
      }
      if (NtripSettings.send_NMEA == 1)
      {
        debug = 0;
        Serial.print(c);
      }
   }

   if (tcpNMEAclient.connected())
   {
     tcpNMEAclient.write(c);
     while (tcpNMEAclient.available())
     {
      Serial1.write(tcpNMEAclient.read());
      yield();
     }
   }
   else if (b_NMEA_connected)
   { 
     b_NMEA_connected = false;
      DBG("*** tcpNMEA DISconnected...");
   }
   
  }

}  


void Core2code( void * pvParameters )
{
  
 DBG("\nTask2 running on core ");
 DBG((int)xPortGetCoreID(), 1);
/*

*/

// Start WiFi Client
 while (!EE_done)
 {  // wait for eeprom data
  delay(100);
 }
 DBG("WiFi_Start_STA in Task2");
 
 if (!wifi_connected)
 {
  WiFi_Start_STA();
  delay(100);
 }

 // by JG
 // LINUX verbindung
 // sudo rfcomm connect /dev/rfcomm0 30:AE:A4:73:83:06 1 &
//  COM36 ist auf rfcomm0 gelinkt
  
 #ifdef USE_BLUETOOTH
     if(!SerialBT.begin("NTRIP"))
     {
      DBG("\nAn error occurred initializing Bluetooth\n");
     }
     else
     {
       DBG("\nBluetooth-ID: 'NTRIP'\n");
       //SerialBT.disconnect();
       //SerialBT.setPin("0000");
       //SerialBT.enableSSP();
       SerialBT.clearWriteError();
       SerialBT.flush();
       SerialBT.clearWriteError();
     }
     
 #endif


 if (my_WiFi_Mode == 0) WiFi_Start_AP(); // if failed start AP

 DBG("WiFi-Mode: ");
 DBG(my_WiFi_Mode,1);

 repeat_ser = millis();
 

  // by JG
  //udpRoof.listen(portMy);
  //UDPReceiveNtrip();
  
  for(;;)
  { //main loop core2
    if (wifi_connected == true)
    {
	    WiFi_Traffic();
      delay(1);
		  Serial_GPS_TXD();
      delay(1);
      SerialBT_Traffic();
    }
    yield();
    //tcpNMEAI();
    
  
   }//end main loop core 2
}
