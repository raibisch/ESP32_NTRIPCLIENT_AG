#include <Arduino.h>

//libraries -------------------------------
#include <Wire.h>
#include <WiFi.h>
#include <base64.h>
#include <BluetoothSerial.h>

#include "Network_AOG.h"
#include "util_eeprom.h"
#include "global_vars.h"
#include "Core1code.h"
#include "Core2code.h"
#include "uti_dbg.h"
#include "Display.h"
#include "SDCard.h"



TaskHandle_t Core1;
TaskHandle_t Core2;
// ESP32 Ntrip Client by Coffeetrac
// Release: V1.26
// 01.01.2019 W.Eder
// Enhanced by Matthias Hammer 12.01.2019
//##########################################################################################################
//### Setup Zone ###########################################################################################
//### Just Default values ##################################################################################

// BY JG : Juergen Goldmann 30.6.2021 : 
//Umbau des Codes für PlatformIO
// 
// Idee für neuen Webserver: https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/

Storage NtripSettings;
bool debugmode = false;
byte my_WiFi_Mode = 1;  // WIFI_STA = 1 = Workstation  WIFI_AP = 2  = Accesspoint

// by JG
TinyGPSPlus gps;

// Variables ------------------------------
// WiFistatus LED 
// blink times: searching WIFI: blinking 4x faster; connected: blinking as times set; data available: light on; no data for 2 seconds: blinking
unsigned int LED_WIFI_time = 0;
unsigned int LED_WIFI_pulse = 700;   //light on in ms 
unsigned int LED_WIFI_pause = 700;   //light off in ms
boolean LED_WIFI_ON = false;



// program flow
bool wifi_connected= false;

// by JG
bool bluetooth_connected = false;
bool ntrip_enabled = false;
bool EE_done = false;
bool restart= false;


unsigned long repeat_ser;   
//int error = 0;
unsigned long repeatGGA, lifesign, aogntriplife, refreshDisplay;

// by JG
char NtripRunState = '_';


//loop time variables in microseconds
const unsigned int LOOP_TIME = 100; //10hz 
unsigned int lastTime = LOOP_TIME;
unsigned int currentTime = LOOP_TIME;

// LON/LAT UTM Conversation
/* Part of code is  from (c) Chris Veress 2014-2019 */
/*                                      MIT License */
#include <math.h>
#define FLOAT double
#define SIN sin
#define COS cos
#define TAN tan
#define POW pow
#define SQRT sqrt
#define FLOOR floor

#define pi 3.14159265358979

/* Ellipsoid model constants (actual values here are for WGS84) */
#define sm_a 6378137.0
#define sm_b 6356752.314
#define sm_EccSquared 6.69437999013e-03

#define UTMScaleFactor 0.9996


// DegToRad
// Converts degrees to radians.
FLOAT DegToRad(FLOAT deg) {
  return (deg / 180.0 * pi);
}


// RadToDeg
// Converts radians to degrees.
FLOAT RadToDeg(FLOAT rad) {
  return (rad / pi * 180.0);
}

// ArcLengthOfMeridian
// Computes the ellipsoidal distance from the equator to a point at a
// given latitude.
// 
// Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
// GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
// 
// Inputs:
//     phi - Latitude of the point, in radians.
// 
// Globals:
//     sm_a - Ellipsoid model major axis.
//     sm_b - Ellipsoid model minor axis.
// 
// Returns:
//     The ellipsoidal distance of the point from the equator, in meters.
FLOAT ArcLengthOfMeridian (FLOAT phi) {
  FLOAT alpha, beta, gamma, delta, epsilon, n;
  FLOAT result;
  
  /* Precalculate n */
  n = (sm_a - sm_b) / (sm_a + sm_b);
  
  /* Precalculate alpha */
  alpha = ((sm_a + sm_b) / 2.0)
        * (1.0 + (POW(n, 2.0) / 4.0) + (POW(n, 4.0) / 64.0));
  
  /* Precalculate beta */
  beta = (-3.0 * n / 2.0) + (9.0 * POW(n, 3.0) / 16.0)
       + (-3.0 * POW(n, 5.0) / 32.0);
  
  /* Precalculate gamma */
  gamma = (15.0 * POW(n, 2.0) / 16.0)
        + (-15.0 * POW(n, 4.0) / 32.0);
  
  /* Precalculate delta */
  delta = (-35.0 * POW(n, 3.0) / 48.0)
      + (105.0 * POW(n, 5.0) / 256.0);
  
  /* Precalculate epsilon */
  epsilon = (315.0 * POW(n, 4.0) / 512.0);
  
  /* Now calculate the sum of the series and return */
  result = alpha
         * (phi + (beta * SIN(2.0 * phi))
         + (gamma * SIN(4.0 * phi))
         + (delta * SIN(6.0 * phi))
         + (epsilon * SIN(8.0 * phi)));
  
  return result;
}



// MapLatLonToXY
// Converts a latitude/longitude pair to x and y coordinates in the
// Transverse Mercator projection.  Note that Transverse Mercator is not
// the same as UTM; a scale factor is required to convert between them.
//
// Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
// GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
//
// Inputs:
//    phi - Latitude of the point, in radians.
//    lambda - Longitude of the point, in radians.
//    lambda0 - Longitude of the central meridian to be used, in radians.
//
// Outputs:
//    x - The x coordinate of the computed point.
//    y - The y coordinate of the computed point.
//
// Returns:
//    The function does not return a value.
void MapLatLonToXY (FLOAT phi, FLOAT lambda, FLOAT lambda0, FLOAT &x, FLOAT &y) {
    FLOAT N, nu2, ep2, t, t2, l;
    FLOAT l3coef, l4coef, l5coef, l6coef, l7coef, l8coef;
    //FLOAT tmp; // Unused

    /* Precalculate ep2 */
    ep2 = (POW(sm_a, 2.0) - POW(sm_b, 2.0)) / POW(sm_b, 2.0);

    /* Precalculate nu2 */
    nu2 = ep2 * POW(COS(phi), 2.0);

    /* Precalculate N */
    N = POW(sm_a, 2.0) / (sm_b * SQRT(1 + nu2));

    /* Precalculate t */
    t = TAN(phi);
    t2 = t * t;
    //tmp = (t2 * t2 * t2) - POW(t, 6.0); // Unused

    /* Precalculate l */
    l = lambda - lambda0;

    /* Precalculate coefficients for l**n in the equations below
       so a normal human being can read the expressions for easting
       and northing
       -- l**1 and l**2 have coefficients of 1.0 */
    l3coef = 1.0 - t2 + nu2;

    l4coef = 5.0 - t2 + 9 * nu2 + 4.0 * (nu2 * nu2);

    l5coef = 5.0 - 18.0 * t2 + (t2 * t2) + 14.0 * nu2
        - 58.0 * t2 * nu2;

    l6coef = 61.0 - 58.0 * t2 + (t2 * t2) + 270.0 * nu2
        - 330.0 * t2 * nu2;

    l7coef = 61.0 - 479.0 * t2 + 179.0 * (t2 * t2) - (t2 * t2 * t2);

    l8coef = 1385.0 - 3111.0 * t2 + 543.0 * (t2 * t2) - (t2 * t2 * t2);

    /* Calculate easting (x) */
    x = N * COS(phi) * l
        + (N / 6.0 * POW(COS(phi), 3.0) * l3coef * POW(l, 3.0))
        + (N / 120.0 * POW(COS(phi), 5.0) * l5coef * POW(l, 5.0))
        + (N / 5040.0 * POW(COS(phi), 7.0) * l7coef * POW(l, 7.0));

    /* Calculate northing (y) */
    y = ArcLengthOfMeridian (phi)
        + (t / 2.0 * N * POW(COS(phi), 2.0) * POW(l, 2.0))
        + (t / 24.0 * N * POW(COS(phi), 4.0) * l4coef * POW(l, 4.0))
        + (t / 720.0 * N * POW(COS(phi), 6.0) * l6coef * POW(l, 6.0))
        + (t / 40320.0 * N * POW(COS(phi), 8.0) * l8coef * POW(l, 8.0));

    return;
}



// UTMCentralMeridian
// Determines the central meridian for the given UTM zone.
//
// Inputs:
//     zone - An integer value designating the UTM zone, range [1,60].
//
// Returns:
//   The central meridian for the given UTM zone, in radians
//   Range of the central meridian is the radian equivalent of [-177,+177].
FLOAT UTMCentralMeridian(int zone) {
  FLOAT cmeridian;
  cmeridian = DegToRad(-183.0 + ((FLOAT)zone * 6.0));
  
  return cmeridian;
}



// LatLonToUTMXY
// Converts a latitude/longitude pair to x and y coordinates in the
// Universal Transverse Mercator projection.
//
// Inputs:
//   lat - Latitude of the point, in radians.
//   lon - Longitude of the point, in radians.
//   zone - UTM zone to be used for calculating values for x and y.
//          If zone is less than 1 or greater than 60, the routine
//          will determine the appropriate zone from the value of lon.
//
// Outputs:
//   x - The x coordinate (easting) of the computed point. (in meters)
//   y - The y coordinate (northing) of the computed point. (in meters)
//
// Returns:
//   The UTM zone used for calculating the values of x and y.
int LatLonToUTMXY (FLOAT lat, FLOAT lon, int zone, FLOAT& x, FLOAT& y) {
  if ( (zone < 1) || (zone > 60) )
    zone = FLOOR((lon + 180.0) / 6) + 1;
  
  MapLatLonToXY (DegToRad(lat), DegToRad(lon), UTMCentralMeridian(zone), x, y);
  
  /* Adjust easting and northing for UTM system. */
  x = x * UTMScaleFactor + 500000.0;
  y = y * UTMScaleFactor;
  if (y < 0.0)
    y = y + 10000000.0;
  
  return zone;
}



/// _________________________________________


// Setup procedure ------------------------
void setup() 
{
  Serial.begin(115200); // by JG bei mir immer zum Debuggen
 
  restoreEEprom();
  // by JG GPS-Port !?
  Serial1.begin (NtripSettings.baudOut, SERIAL_8N1, RX1, TX1); 
  //Serial1.begin(9600,SERIAL_8N1,RX1,TX1); 
  //Serial1.setRxBufferSize(2000);
  delay(100);

  Serial.println("*** START Setup IO ***");
  delay(200);
  wifi_connected = false;

  // Setup the button with an internal pull-up
#ifdef USE_M5STACK
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pinMode(BUTTON_C_PIN, INPUT_PULLUP);
#endif

 #ifdef USE_M5STICKC
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
#endif
  
  delay(100);

  Serial.println("---- GCP-Data from EEPROM ----");
  for (int i=0; i<10; i++)
  {
      Serial.println(NtripSettings.gcp[i]);
  }
  delay(1000);

#ifdef  USE_DISPLAY
  display_init();
  display_text(0,"BT-NTRIP M5STACK");
  display_display(); 
#endif
  
  delay(100);
 

  //------------------------------------------------------------------------------------------------------------  
  //create a task that will be executed in the Core1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Core1code, "Core1", 10000, NULL, 2, &Core1, 0); // test Priority 2
  delay(500); 
  //create a task that will be executed in the Core2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Core2code, "Core2", 10000, NULL, 1, &Core2, 1); 
  delay(500); 
  //------------------------------------------------------------------------------------------------------------

}

boolean b_first_run = true;
uint ui_GCP_count = 0;
char buttontext[18];
uint32_t txtcolor = TFT_RED;

void loop() 
{
#ifdef USE_DISPLAY  
		if (millis() - refreshDisplay >= 1200) // 1,2sec
		{ 
       refreshDisplay = millis(); 
       if (wifi_connected)
       {
        //display_clear();
        String fix = "--  ";
        //Serial.print("Fix-Mode:");
        //Serial.println(gps.cFixMode());
        if (gps.cFixMode() == 1)
        {
         fix = "GPS   ";
         txtcolor = TFT_ORANGE;
        }
        else
        if (gps.cFixMode()==2)
        {
         fix = "DGPS ";
         txtcolor = TFT_YELLOW;
        }
        else
        if (gps.cFixMode()==4)
        {
         fix = "RTK  ";
         txtcolor = TFT_GREEN;
        }
        if (gps.cFixMode()==5)
        {
         fix = "FLOAT"; 
         txtcolor = TFT_CYAN;
        }
         
        display_text(0, "SAT:"+ String(gps.satellites.value()) + " FIX:"+ fix, txtcolor); 
        display_text(1, "LA:"+ String(gps.location.lat(),7)); 
        display_text(2, "LO: "+ String(gps.location.lng(),7));
       
        
        //char TimeString[10];
        //sprintf_P(TimeString, PSTR("%02d:%02d:%02d"), gps.time.hour()+2, gps.time.minute(), gps.time.second());
        //display_text(3,PSTR(TimeString));
       
        String scon = "BT:--";
        if (bluetooth_connected)
        {
          scon = "BT:ON"; 
        }

        if (NtripSettings.enableNtrip==1)
        {
          scon = scon + "  NTRIP:ON " + NtripRunState ;
        }
        else
        {
          scon = scon +"  NTRIP:--";
        }
        display_text(4, scon);
        //display_display(); 
         if (b_first_run && wifi_connected)
        {
         display_text(6,"RESET GCP00 NTRIP");
        b_first_run = false;
        }

     #if defined(USE_M5STACK) || defined(USE_M5STICKC)
      M5.BtnA.read();
      delay(1);
      M5.BtnB.read();
      delay(1);
    #ifdef USE_M5STACK
      M5.BtnC.read();
    #endif
      delay(1);
      if (M5.BtnA.wasPressed())
      {
        display_clear();
        display_text(0, "RESET!");
        delay(1000);
        ESP.restart();
      }
      else
      if (M5.BtnB.wasPressed())
      {
        if (ui_GCP_count < 9)
        {
          ui_GCP_count++;
        }
        sprintf_P(buttontext, PSTR("RESET GCP%02d NTRIP"), ui_GCP_count);
         display_text(6,PSTR(buttontext));
         double x,y;
         String sx,sy;
       
         LatLonToUTMXY(gps.location.lat(), gps.location.lng(),0,x,y);
         sx = String(x);
         sy = String(y);
         Serial.print("Lat:"); Serial.println( String(gps.location.lat(),7));
         Serial.print("Lon:"); Serial.println( String(gps.location.lng(),7));
         Serial.print("x  :"); Serial.println(sx);
         Serial.print("y  :"); Serial.println(sy);
         Serial.print("h  :"); Serial.println(gps.altitude.meters());

         // für Standort "Büro Raibach
         /*
            Lat:49.8716320
            Lon:8.9595593

            UTM 32N
            x  :497094.03
            y  :5524359.10

            Test:
            http://rcn.montana.edu/Resources/Converter.aspx
            https://franzpc.com/apps/coordinate-converter-utm-to-geographic-latitude-longitude.html
            https://jjimenezshaw.github.io/Leaflet.UTM/examples/input.html
            

          Example GPC-File:
          first line:
          WGS84 UTM 32N
          or ??
          +proj=utm +zone=32 +north +ellps=WGS84 +datum=WGS84 +units=m +no_defs

          gcp01 529356.250827686 9251137.5643209 8.465
         */
         //sd_writefile(SD, "/gcp.log", "gcp" + String(ui_GCP_count)+ " " + sx +" " + sy + " " + gps.altitude.meters() + "\r\n");
         String gcpxx = "gcp" + String(ui_GCP_count)+ " " + sx +" " + sy + " " + gps.altitude.meters();
         gcpxx.toCharArray(NtripSettings.gcp[ui_GCP_count-1], 32);
         Serial.println(NtripSettings.gcp[ui_GCP_count-1]);
         EEprom_write_all();
      }
    #ifdef USE_M5STACK
      else
      if (M5.BtnC.wasPressed())
      {
         if (NtripSettings.enableNtrip==0)
         {
           NtripSettings.enableNtrip=1;
           sprintf_P(buttontext, PSTR("RESET GCP%02d NTRIP"), ui_GCP_count);
           display_text(6,PSTR(buttontext));
         }
        else
        {
          NtripSettings.enableNtrip=0;
          sprintf_P(buttontext, PSTR("RESET GCP%02d NTRIP"), ui_GCP_count);
           display_text(6,PSTR(buttontext));
        }
        delay(500);
        EEprom_write_all();
      }
      #endif
     }
    }
    delay(10);
    #endif
    
#endif
}
