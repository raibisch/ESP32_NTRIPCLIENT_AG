#ifndef GLOBAL_VARS_H_
#define GLOBAL_VARS_H_


#define USE_BLUETOOTH  // possibility to use bluetooth to transfer data to AOG later on, but needs lots of memory.


//#define USE_ACCEL  // by JG keine IMU

// mit Display
#define USE_DISPLAY

// Boardauswahl
//#define USE_WEMOSBOARD // OLED Display auf Wemos Board
#define USE_M5STICKC     // M5 Stick-C
//#define USE_M5STACK    // M5 Stack Core

// by JG ACHTUNG gilt nur für Testaufbau auf Wemos-Board 
#ifdef USE_WEMOSBOARD
#ifdef USE_DISPLAY
 #include <SSD1306.h> // alias for `#include "SSD1306Wire.h"'
#endif 
#define RX1     14
#define TX1     12
define  BAUDGPS 9600
#define SERVER_HOSTNAME "GNSSM8"
#endif

// M5 Stick mit ublox M8 (ohne Backup-Bat daher immer 9600Bd)
#ifdef USE_M5STICKC
#include <M5StickC.h>
#define RX1 36
#define TX1 26
#define BAUDGPS 9600
#define SERVER_HOSTNAME "GNSSM8"
#endif

// M5Stack Core (ist eigentlich auch Juergen bis auf das F9P-Modul)
#ifdef USE_M5STACK
#include <M5Stack.h>                                         /* Include M5 Stack library */
#define USE_SDCARD
// -----DLG M5Stack Core mit F9P-Modul
#define RX1 13
#define TX1 5
// Achtung hier ist TXD auf der internen LED  GPIO-05 !!!
//#define LED 2 // --> macht nichts (da LED auf GPIO-05)
#define BAUDGPS 460800
//#define BAUDGPS 230400
//#define BAUDGPS 115200
#define SERVER_HOSTNAME "GNSSP9"
#endif

#include <BluetoothSerial.h>
#include <TinyGPSxx.h>




#define SDA     21  //I2C Pins
#define SCL     22

#define LED_PIN_WIFI   36   // WiFi Status LED

//########## BNO055 adress 0x28 ADO = 0 set in BNO_ESP.h means ADO -> GND
//########## MMA8451 adress 0x1D SAO = 0 set in MMA8452_AOG.h means SAO open (pullup!!)

//#define restoreDefault_PIN 4  // set to 1 during boot, to restore the default values

extern bool debugmode;
extern byte my_WiFi_Mode;

extern const unsigned int LOOP_TIME;
extern unsigned int lastTime;
extern unsigned int currentTime;


// Variables ------------------------------
// WiFistatus LED 
// blink times: searching WIFI: blinking 4x faster; connected: blinking as times set; data available: light on; no data for 2 seconds: blinking
extern unsigned int LED_WIFI_time;
extern unsigned int LED_WIFI_pulse;   //light on in ms 
extern unsigned int LED_WIFI_pause;
extern boolean LED_WIFI_ON;

// by JG
extern TinyGPSPlus gps;




// program flow
extern bool wifi_connected;
extern bool ntrip_enabled;

// by JG
extern bool bluetooth_connected;

extern bool EE_done;
extern bool restart;
extern unsigned long repeat_ser;   
//int error = 0;
extern unsigned long repeatGGA;
extern unsigned long lifesign;
extern unsigned long aogntriplife;
// by JG
extern unsigned long refreshDisplay;


extern char NtripRunState;

#include "credential.sec"

struct Storage
{
  
  char ssid[24]        = WIFI_ssid;          // WiFi network Client name
  char password[24]    = WIFI_pass;      // WiFi network password
  unsigned long timeoutRouter = 65;           // Time (seconds) to wait for WIFI access, after that own Access Point starts 

  // Ntrip Caster Data
  char host[40]        = NTRIP_host;    // Server IP
  int  port            = 2102;                // Server Port
  char mountpoint[40]  = "agrar_2G";   // Mountpoint
  char ntripUser[40]    = NTRIP_user;     // Username
  char ntripPassword[40] = NTRIP_pass;    // Password

  byte sendGGAsentence = 1; // 0 = No Sentence will be sended
                            // 1 = fixed Sentence from GGAsentence below will be sended
                            // 2 = GGA from GPS will be sended
  
  byte GGAfreq =10;         // time in seconds between GGA Packets

  //char GGAsentence[100] = "$GPGGA,051353.171,4751.637,N,01224.003,E,1,12,1.0,0.0,M,0.0,M,,*6B"; //hc create via www.nmeagen.org
  char GGAsentence[100]   = "$GPGGA,211218.386,4952.302,N,00857.571,E,1,12,1.0,0.0,M,0.0,M,,*68"; // GGA at Raibach UD36
  
  long baudOut = 9600;     // Baudrate of RTCM Port

  byte send_NMEA  = 0;   // 0 = Transmission of NMEA Off
                            // old   1 = Transmission of NMEA Sentences to AOG via Ethernet-UDP
                            // by JG 1 = Transmission of NMEA to USB-Serial-port (Serial) 
                            // 2 = Bluetooth attention: not possible if line USE_BLUETOOTH= false

  byte enableNtrip   = 1;   // 0 = NTRIP disabled
                            // 1 = ESP NTRIP Client enabled
                            // 2 = AOG NTRIP Client enabled (Port=2233)
  
  // by JG (nicht verwendet) 
  //byte AHRSbyte      = 0;  
  // by JG

 
  // 10 Ground-Control-Points
  char gcp[10][33]={ "xxx1 000000.00 0000000.00 000.0", 
                     "xxx2 000000.00 0000000.00 000.0", 
                     "xxx3 000000.00 0000000.00 000.0",
                     "xxx4 000000.00 0000000.00 000.0", 
                     "xxx5 000000.00 0000000.00 000.0",
                     "xxx6 000000.00 0000000.00 000.0",
                     "xxx7 000000.00 0000000.00 000.0",
                     "xxx8 000000.00 0000000.00 000.0", 
                     "xxx9 000000.00 0000000.00 000.0",
                      };
 
}; 

extern Storage NtripSettings;
//extern BluetoothSerial SerialBT;

#endif