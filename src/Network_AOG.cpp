#include <Arduino.h>
#include <WiFi.h>
#include <base64.h>
#include <EEPROM.h>
#include "util_eeprom.h"
#include "Network_AOG.h"
#include "uti_dbg.h"
#include "global_vars.h"
#include "Display.h"



int action;

// Radiobutton Select your Position type
char position_type[3][26] = {"Position Off", "Position Fixed via String", "GGA Position from GPS"};

// Radiobutton Select the time between repetitions.
char repeatPos[3][8] = {"1 sec.", "5 sec.", "10 sec."};

// Radiobutton Baudrate of serial1
char baud_output[7][7] = {"  9600", " 14400", " 19200", " 38400", " 57600", "115200","460800"};

// Radiobutton Select if NMEA are transmitted via UDP.
char sendNmea[3][10] = {"OFF", "USB-Ser.", "Bluetooth"};

// Radiobutton Select if NTRIP Client is enabled. (Off to use only NMEA Transmission to AOG)
char ntripOn_type[3][9] = {"OFF", "WiFi-TCP", "AOG-UDP"};

// by JG
char gcp_WG84_32N[13] = {"WGS84 UTM32N"};
//---------------------------------------------------------------------

//Accesspoint name and password:
const char* ssid_ap     = "NTRIP";
const char* password_ap = "";


char HTML_String[7000];
char HTTP_Header[160];


char strmBuf[1024];         // rtcm Message Buffer by JG: old 512


//--------GPS Bridge public
unsigned long Ntrip_data_time = 0;
char lastSentence[100]="";
bool newSentence = false;
byte gpsBuffer[100];

// GPS-Bridge
char imuBuffer[20];

int intern_count = 0;

//---------------------------------------------------------------------
String _userAgent = "DLG NTRIPClient";

// Allgemeine Variablen
String _base64Authorization;
String _accept = "*/*";


WiFiClient client_page;
WiFiClient ntripCl;
//AsyncUDP udpNtrip;

// by JG NMEA over TCP/IP for u-center communication
char rxGPSBuf[510];        // by: JG ReceiveBuffer for Serial1 (GPS-Modul)
//WiFiServer tcpNMEAserver(81);
WiFiClient tcpNMEAclient;

// von main.cpp uebernommen
//static IP
IPAddress myip(192, 168, 2, 79);  // own WIFI-Client adress
IPAddress apip(192, 168, 4,  1);      // AP- adress
IPAddress gwip(192, 168, 2,  1);   // Gateway & Accesspoint IP
IPAddress mask(255, 255, 255, 0);
IPAddress myDNS(8, 8, 8, 8);      //optional


WiFiServer server(80);

// by JG von Core1Code übernommen
//###########################################################################################
void setAuthorization(const char * user, const char * password)
{
    if(user && password) {
        String auth = user;
        auth += ":";
        auth += password;
        _base64Authorization = base64::encode(auth);
    }
}

//###########################################################################################
bool connectCaster()
{
  DBG("*** connectCaster Start\n");
  setAuthorization(NtripSettings.ntripUser, NtripSettings.ntripPassword); // Create Auth-Code
  DBG("*** connectCaster Auth\n");
  bool ret = ntripCl.connect(NtripSettings.host, NtripSettings.port);
  DBG("*** connectCaster connect\n");
  return ret;
}

//###########################################################################################
char* readLine()
{
  int i=0;
  // Read a line of the reply from server and print them to Serial
  //Serial.println("start Line read: ");  
  while(ntripCl.available()) 
  {
    strmBuf[i] = ntripCl.read();
    //Serial.print(strmBuf[i]);
    if( strmBuf[i] == '\n' || i>=511) break;
    i++;  
    yield();         
  }
  strmBuf[i]= '\0';  //terminate string
  return (char*)strmBuf;
  
}
//###########################################################################################
void sendGGA()
{
    DBG("G");
    NtripRunState = 'G';
    // DBG("Sending GGA: ");
    // This will send the Position to the server, required by VRS - Virtual Reference Station - Systems
    if (NtripSettings.sendGGAsentence == 1){
       ntripCl.print(NtripSettings.GGAsentence);
       ntripCl.print("\r\n");
      }
    if (NtripSettings.sendGGAsentence > 1){
       ntripCl.print(lastSentence);
       //DBG(lastSentence, 1); // 
      }
}
   
//###########################################################################################
bool getSourcetable()
{
   DBG("*** getSourcetable 0\n");
   if (!connectCaster())
   {
     DBG("NTRIP Host connection failed\n");
     DBG("Can not connect to NTRIP Hoster\n");
     DBG("Check Network Name and Port\n");
     delay(2000);
     return false;
   }    
   DBG("*** getSourcetable 1\n");
    // This will send the request to the server
     ntripCl.print(String("GET /") + " HTTP/1.0\r\n" +
                    "User-Agent: " + _userAgent + "\r\n" +
                    "Accept: " + _accept + "\r\n" +
                    "Connection: close\r\n\r\n");
    
    unsigned long timeout = millis();
    DBG("*** getSourcetable 2\n");
    while (!ntripCl.available()) 
    {
        if (millis() - timeout > 5000) {
            DBG(">>> Client Timeout while requesting Sourcetable !!\n");
            DBG("Check Caster Name and Port !!\n");
            ntripCl.stop();
            return false;
        }
       delay(1); //wdt 
    }
   DBG("*** getSourcetable 3\n");
   String currentLine = readLine(); //reads to strmBuf
   if (currentLine.startsWith("SOURCETABLE 200 OK"))
   {
     DBG(currentLine, 1);
     delay(5);
     for(int as=0; as < 2; as++){
       while(ntripCl.available()) {
         currentLine = ntripCl.readStringUntil('\n');
         DBG(currentLine, 1);
         delay(1);
       }
       delay(100); // wait for additional Data
     }
     DBG("---------------------------------------------------------------------------------------------\n");
     ntripCl.stop();
     return true;
   }
   else{
     DBG(currentLine, 1);
     return false;
   }  
}

//###########################################################################################
bool startStream()
{
 
 // Reconnect for getting RTCM Stream
 if (!connectCaster())
 {  //reconnect for stream
   DBG("NTRIP Host connection failed\n");
   DBG("Can not connect to NTRIP Hoster\n");
   DBG("Check Network Name and Port\n");
   delay(2000);
   return false;
 }
 
 // This will send the request to the server
 String requestMtp =(String("GET /") + NtripSettings.mountpoint + " HTTP/1.0\r\n" +
                      "User-Agent: " + _userAgent + "\r\n");
 if (strlen(NtripSettings.ntripUser)==0){
   requestMtp += (String("Accept: ") + _accept + "\r\n");
   requestMtp += String("Connection: close\r\n"); 
  }                    
 else{
   requestMtp += String("Authorization: Basic "); 
   requestMtp += _base64Authorization;
   requestMtp += String("\r\n");
 }
 requestMtp += String("\r\n");
 
 ntripCl.print(requestMtp);
 // DBG(requestMtp);
 
 if (NtripSettings.sendGGAsentence > 0) {
    sendGGA(); // 
    repeatGGA = millis();
    DBG("", 1); //NL
  }
 
 delay(10);
 
 unsigned long timeout = millis();
 while (!ntripCl.available()) {
    if (millis() - timeout > 15000) {
         DBG("\n>>> Client Timeout - no response from host\n");
         ntripCl.stop();
         delay(2000);
         return false;
        }
     delay(1); //wdt
    } 
 delay(5);
 String currentLine;
 if (!(currentLine = readLine())) return false; //read answer 
 if (!(currentLine.startsWith("ICY 200 OK"))) {
  DBG("Received Error: ");
  DBG(currentLine, 1);
  return false;
 }
 //DBG("Connection established\n");
 intern_count=0; // reset counter
 return true;
}

//###########################################################################################
bool getRtcmData()
{
  long timeout = millis();
  while (ntripCl.available() == 0) 
  {
      if (millis() - timeout > 5000) 
      {
            DBG("\n>>> Client Timeout no RTCM Respond from Caster\n");
            if(sizeof(lastSentence) < 50 && (NtripSettings.sendGGAsentence >1)) DBG("Invalid NMEA String from GPS\n");
            if(NtripSettings.sendGGAsentence == 1) DBG("Maybe invalid fixed NMEA String\n");
            if(NtripSettings.sendGGAsentence == 0) DBG("Check if your Provider requires your Position\n");
            return false;
      }
      delay(1); // WDT prevent
  }
   
   // Read all the bytes of the reply from server and print them to Serial
   while(ntripCl.available()&& NtripSettings.enableNtrip == 1) 
   {
      char a = ntripCl.read();
      Serial1.print(a);
      if (NtripSettings.sendGGAsentence > 0)
      {
        if (millis() - repeatGGA > (NtripSettings.GGAfreq * 1000)) 
        {
          sendGGA(); //
          repeatGGA = millis();
        }
      }
      if (millis() - lifesign > 1000) 
      {
        DBG("*"); // Sectic - Data receiving
        if (intern_count % 2)
        {
          NtripRunState = '*';
        }
        else 
        {
          NtripRunState = '+';
        }
        if(intern_count++ >=59) {
          DBG("", 1); //New Line
          intern_count=0;
        }
        lifesign = millis();
        Ntrip_data_time = millis();  //LED WiFi Status timer
      }      
      delay(1);   
   }
  return true;
}

// End of NTIP-Client functions



//---------------------------------------------------------------------
char HexChar_to_NumChar( char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 55;
  return 0;
}
//---------------------------------------------------------------------
void exhibit(const char * tx, int v) {
  DBG(tx);
  DBG(v, 1);
}
//---------------------------------------------------------------------
void exhibit(const char * tx, unsigned int v) {
  DBG(tx);
  DBG((int)v, 1);
}
//---------------------------------------------------------------------
void exhibit(const char * tx, unsigned long v) {
  DBG(tx);
  DBG((long)v, 1);
}
//---------------------------------------------------------------------
void exhibit(const char * tx, const char * v) {
  DBG(tx);
  DBG(v, 1);
}



//---------------------------------------------------------------------
int Find_Start(const char * such, const char * str) {
  int tmp = -1;
  int ww = strlen(str) - strlen(such);
  int ll = strlen(such);

  for (int i = 0; i <= ww && tmp == -1; i++) {
    if (strncmp(such, &str[i], ll) == 0) tmp = i;
  }
  return tmp;
}
//---------------------------------------------------------------------
int Find_End(const char * such, const char * str) {
  int tmp = Find_Start(such, str);
  if (tmp >= 0)tmp += strlen(such);
  return tmp;
}


//---------------------------------------------------------------------
int Pick_Dec(const char * tx, int idx ) {
  int tmp = 0;

  for (int p = idx; p < idx + 5 && (tx[p] >= '0' && tx[p] <= '9') ; p++) {
    tmp = 10 * tmp + tx[p] - '0';
  }
  return tmp;
}

//---------------------------------------------------------------------
int Pick_Parameter_Zahl(const char * par, char * str) {
  int myIdx = Find_End(par, str);

  if (myIdx >= 0) return  Pick_Dec(str, myIdx);
  else return -1;
}



//---------------------------------------------------------------------
void strcatf(char* tx, float f) {
  char tmp[8];

  dtostrf(f, 6, 2, tmp);
  strcat (tx, tmp);
}
//---------------------------------------------------------------------
//void strcatl(char* tx, long l) {
  //char tmp[sizeof l];
  //memcpy(tmp, l, sizeof l);
  //strcat (tx, tmp);
//}

//---------------------------------------------------------------------
void strcati(char* tx, int i) {
  char tmp[8];

  itoa(i, tmp, 10);
  strcat (tx, tmp);
}

//---------------------------------------------------------------------
void strcati2(char* tx, int i) {
  char tmp[8];

  itoa(i, tmp, 10);
  if (strlen(tmp) < 2) strcat (tx, "0");
  strcat (tx, tmp);
}

//------------------------------------------------------------------------------------------
void set_colgroup1(int ww) {
  if (ww == 0) return;
  strcat( HTML_String, "<col width=\"");
  strcati( HTML_String, ww);
  strcat( HTML_String, "\">");
}

//----------------------------------------------------------------------------------------------
void set_colgroup(int w1, int w2, int w3, int w4, int w5) {
  strcat( HTML_String, "<colgroup>");
  set_colgroup1(w1);
  set_colgroup1(w2);
  set_colgroup1(w3);
  set_colgroup1(w4);
  set_colgroup1(w5);
  strcat( HTML_String, "</colgroup>");

}

//----------------------------------------------------------------------------
int Pick_N_Zahl(const char * tx, char separator, byte n) {

  int ll = strlen(tx);
  //int tmp = -1; by JG unused variable
  byte anz = 1;
  byte i = 0;
  while (i < ll && anz < n) {
    if (tx[i] == separator)anz++;
    i++;
  }
  if (i < ll) return Pick_Dec(tx, i);
  else return -1;
}

//---------------------------------------------------------------------
int Pick_Hex(const char * tx, int idx ) {
  int tmp = 0;

  for (int p = idx; p < idx + 5 && ( (tx[p] >= '0' && tx[p] <= '9') || (tx[p] >= 'A' && tx[p] <= 'F')) ; p++) {
    if (tx[p] <= '9')tmp = 16 * tmp + tx[p] - '0';
    else tmp = 16 * tmp + tx[p] - 55;
  }

  return tmp;
}

//---------------------------------------------------------------------
void Pick_Text(char * tx_ziel, char  * tx_quelle, int max_ziel) {

  int p_ziel = 0;
  int p_quelle = 0;
  int len_quelle = strlen(tx_quelle);

  while (p_ziel < max_ziel && p_quelle < len_quelle && tx_quelle[p_quelle] && tx_quelle[p_quelle] != ' ' && tx_quelle[p_quelle] !=  '&') {
    if (tx_quelle[p_quelle] == '%') {
      tx_ziel[p_ziel] = (HexChar_to_NumChar( tx_quelle[p_quelle + 1]) << 4) + HexChar_to_NumChar(tx_quelle[p_quelle + 2]);
      p_quelle += 2;
    } else if (tx_quelle[p_quelle] == '+') {
      tx_ziel[p_ziel] = ' ';
    }
    else {
      tx_ziel[p_ziel] = tx_quelle[p_quelle];
    }
    p_ziel++;
    p_quelle++;
  }

  tx_ziel[p_ziel] = 0;
}

//---------------------------------------------------------------------
void WiFi_Start_AP()
{
  WiFi.mode(WIFI_AP); // Accesspoint
  WiFi.softAP(ssid_ap);
  while (!SYSTEM_EVENT_AP_START) // wait until AP has started
  {
    delay(100);
    DBG(".");
  }

  WiFi.softAPConfig(apip, apip, mask); // set fix IP for AP
  IPAddress getmyIP = WiFi.softAPIP();

  server.begin();
  // tcpNMEAserver.begin();  by JG
  my_WiFi_Mode = WIFI_AP;
  DBG("Accesspoint started - Name : ");
  DBG(ssid_ap);
  DBG(" IP address: ");
  DBG(getmyIP, 1);
  display_clear();
  display_text(0, "WiFI connected to:");
  display_text(1, ssid_ap);
  display_text(2, WiFi.localIP().toString());
  display_display();
  wifi_connected = true;
}

//---------------------------------------------------------------------
void WiFi_Start_STA() 
{
  unsigned long timeout;
  delay(100);
  WiFi.disconnect();
  delay(500);
  WiFi.mode(WIFI_STA);   //  Workstation
  
  // für statische IP:
  if (!WiFi.config(myip, gwip, mask, myDNS)) 
  {
    DBG("STA Failed to configure\n");
  }
  
  WiFi.begin(NtripSettings.ssid, NtripSettings.password);
  //timeout = millis() + (NtripSettings.timeoutRouter * 1000);
  timeout = millis() + (5 * 1000); // 4 sec.
  while (WiFi.status() != WL_CONNECTED && millis() < timeout) 
  {
    delay(400);
    DBG(".");
    //WIFI LED blink in double time while connecting
    
    if (!LED_WIFI_ON) 
    {
        if (millis() > (LED_WIFI_time + (LED_WIFI_pause >> 2))) 
          {
           LED_WIFI_time = millis();
           LED_WIFI_ON = true;
           //digitalWrite(LED_PIN_WIFI, HIGH);
          }
    }
    if (LED_WIFI_ON) 
    {
      if (millis() > (LED_WIFI_time + (LED_WIFI_pulse >> 2))) {
        LED_WIFI_time = millis();
        LED_WIFI_ON = false;
        //digitalWrite(LED_PIN_WIFI, LOW);
      }
      display_clear();
      display_text(0,"..connect WiFI");
      display_display();
      delay(200);
    }
  }    
  
  DBG("", 1); //NL
  if (WiFi.status() == WL_CONNECTED) 
  {
    server.begin();
    //tcpNMEAserver.begin(); // by JG z.Z. nicht genutzt
    my_WiFi_Mode = WIFI_STA;
    display_clear();
    display_text(0,"WiFI connected to:");
    display_text(1, NtripSettings.ssid);
    display_text(2,WiFi.localIP().toString());
    display_display();
    DBG("WiFi Client connected to : ");
   
    DBG(NtripSettings.ssid, 1);
    DBG("Connected IP - Address : ");
    DBG( WiFi.localIP(), 1);
    wifi_connected = true;
  } 
  else 
  {
    //WiFi.mode(WIFI_OFF);
    DBG("WLAN-Client-Connection failed\n");
    
    //ESP.restart(); // by JG
    //wifi_connected = false;

    DBG("..try to create local AP 'NTRIP'");
    WiFi_Start_AP();
    wifi_connected = true;
  }
  
}


//------------------------------------------------------------------------------------------
// Subs --------------------------------------
void udpNtripRecv()
{ /*
  //callback when received packets
  udpNtrip.onPacket([](AsyncUDPPacket packet) 
   {Serial.print("."); 
    for (int i = 0; i < packet.length(); i++) 
        {
          if(NtripSettings.enableNtrip == 2) {
            //Serial.print(packet.data()[i],HEX); 
            Serial1.write(packet.data()[i]); 
          }
        }
   });  // end of onPacket call
   */
}

// by JG
// soll späeter NMEA senden und empfangen
void tcpNMEAIO()
{
  /*
   // HANDLE_SERVER();
  if (server.hasClient())
  {
    WiFiClient client = server.available();
    client.setNoDelay(true); // by JG
    Serial.print("* client connected...");
    ACT_TCPCONNECT = true;

    while (GPSRaw.available())
    {
      char c = GPSRaw.read();
      if (c == 0x0A)
      {
        break;
      }
      //yield();
    }

    //digitalWrite(LED,HIGH);

    while (client.connected())
    {
      // Lese von u-blox Seriell Port
      if (GPSRaw.available())
      {
        int cnt_ser2tcp = GPSR exhibit ("NMEA: ", NtripSettings.enableNtrip); 
      }
      // Lese Input von TCP
      if (client.available())
      {
        GPSRaw.write(client.read());
        
        //char c = client.read();
        //GPSRaw.write(c);
        //Serial.printf(",%0X ",c);
      }
      //yield();
    } // ... client.Connected()

    delay(100);
    ACT_TCPCONNECT = false;
    Serial.println("client disonnected");
  } //... server.hasClient()


  if (tcpNMEA.hasClient())
  {  

    WiFiClient client = server.available();

    client.setNoDelay(true); // by JG
    Serial.print("* client connected...");
     if (!_NMEA_connected)
     {
       _NMEA_connected = true;
       Serial.print("NMEA-connected..");
     }
     if (client.connected())
     {

     }
     delay(1);
  }
  else
  {
    if (_NMEA_connected)
    {
      _NMEA_connected = false;
      Serial.println("..NMEA-DISconnected");
     }
  }
  */
}

//---------------------------------------------------------------------
void UDPReceiveNtrip()
{
  tcpNMEAIO();
  /*
  if(udpNtrip.listen(portMyNtrip)) 
    {
     Serial.print("NTRIP UDP Listening on IP: ");
     Serial.println(WiFi.localIP());
     udpNtripRecv();
    } 
    */
}


//---------------------------------------------------------------------
// HTML Seite 01 aufbauen
//---------------------------------------------------------------------
void make_HTML01() {

  strcpy( HTML_String, "<!DOCTYPE html>");
  strcat( HTML_String, "<html>");
  strcat( HTML_String, "<head>");
  strcat( HTML_String, "<title>AG NTRIP Client Config Page</title>");
  strcat( HTML_String, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0;\" />\r\n");
  //strcat( HTML_String, "<meta http-equiv=\"refresh\" content=\"10\">");
  strcat( HTML_String, "<style>divbox {background-color: lightgrey;width: 200px;border: 5px solid red;padding:10px;margin: 10px;}</style>");
  strcat( HTML_String, "</head>");
  strcat( HTML_String, "<body bgcolor=\"#A2C482\">");
  strcat( HTML_String, "<font color=\"#000000\" face=\"VERDANA,ARIAL,HELVETICA\">");
 
  strcat( HTML_String, "<h1>NTRIP-Client ESP32</h1>");

  //-----------------------------------------------------------------------------------------
  
  char TimeString[20];
  sprintf_P(TimeString, PSTR("%02d:%02d:%02d <br>"), gps.time.hour()+2, gps.time.minute(), gps.time.second()); // by JG
  //strcat( HTML_String, TimeString);
  //strcat( HTML_String, "</br>");
  // by JG Ground-Control-Points ausgeben
  strcat( HTML_String, "<h2>Ground-Control-Points:</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);
  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Delete GCP:</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati( HTML_String, ACTION_DELETE_GCP);
  strcat( HTML_String, "\">Submit</button></td>"); 
  strcat( HTML_String, "</tr>");
  strcat( HTML_String, "</table>");  
  strcat( HTML_String, "</form>");
  strcat(HTML_String, "</br>");
  strcat( HTML_String, gcp_WG84_32N);
  strcat( HTML_String, "</br>");
  for(int i=0; i<10; i++)
  { 
      strcat( HTML_String, NtripSettings.gcp[i]);
      strcat(HTML_String, "</br>");

  }
  strcat( HTML_String, "<br><hr>");

  strcat( HTML_String, "<hr><h2>WiFi Network Client Access Data</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "This NTRIP Client requires access to an Internet enabled Network!!<br><br>");
  strcat( HTML_String, "If access fails, an accesspoint will be created<br>");
  strcat( HTML_String, "(NTRIP_Client_ESP_Net PW:passport)<br><br>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Address:</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"SSID_MY\" maxlength=\"22\" Value =\"");
  strcat( HTML_String, NtripSettings.ssid);
  strcat( HTML_String, "\"></td>");
  
  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_SSID);
  strcat( HTML_String, "\">Submit</button></td>");
  strcat( HTML_String, "</tr>");

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Password:</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"Password_MY\" maxlength=\"22\" Value =\"");
  strcat( HTML_String, NtripSettings.password);
  strcat( HTML_String, "\"></td>");
  strcat( HTML_String, "</tr>");
  strcat( HTML_String, "<tr> <td colspan=\"3\">&nbsp;</td> </tr>");
  strcat( HTML_String, "<tr><td colspan=\"2\"><b>Restart NTRIP client for changes to take effect</b></td>");
  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_RESTART);
  strcat( HTML_String, "\">Restart</button></td>");
  strcat( HTML_String, "</tr>");
  
  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br><hr>");

//-------------------------------------------------------------  
// NTRIP Caster
  strcat( HTML_String, "<h2>NTRIP Caster Settings</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);
  
  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Network Name:(IP only)</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"CASTER\" maxlength=\"40\" Value =\"");
  strcat( HTML_String, NtripSettings.host);
  strcat( HTML_String, "\"></td>");
  
  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_NTRIPCAST);
  strcat( HTML_String, "\">Submit</button></td>");
  strcat( HTML_String, "</tr>");

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Port:</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"CASTERPORT\" maxlength=\"4\" Value =\"");
  strcati( HTML_String, NtripSettings.port);
  strcat( HTML_String, "\"></td>");
  strcat( HTML_String, "</tr>");
  
  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Mountpoint:</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"MOUNTPOINT\" maxlength=\"40\" Value =\"");
  strcat( HTML_String, NtripSettings.mountpoint);
  strcat( HTML_String, "\"></td>");
  strcat( HTML_String, "</tr>");

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Username:</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"CASTERUSER\" maxlength=\"40\" Value =\"");
  strcat( HTML_String, NtripSettings.ntripUser);
  strcat( HTML_String, "\"></td>");
  strcat( HTML_String, "</tr>");
  
  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Password:</b></td>");
  strcat( HTML_String, "<td>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:200px\" name=\"CASTERPWD\" maxlength=\"40\" Value =\"");
  strcat( HTML_String, NtripSettings.ntripPassword);
  strcat( HTML_String, "\"></td>");
  strcat( HTML_String, "</tr>");
  strcat( HTML_String, "<tr> <td colspan=\"3\">&nbsp;</td> </tr>");
  strcat( HTML_String, "<tr><td colspan=\"2\"><b>Restart NTRIP client for changes to take effect</b></td>");
  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_RESTART);
  strcat( HTML_String, "\">Restart</button></td>");
  strcat( HTML_String, "</tr>");
  
  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br><hr>");
//-------------------------------------------------------------  
// GGA processing
  strcat( HTML_String, "<h2>Send my Position</h2>");
  strcat( HTML_String, "(Required if your Caster provides VRS (Virtual Reference Station)<br>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);

  strcat( HTML_String, "<br>");
  for (int i = 0; i < 3; i++) 
  {
    strcat( HTML_String, "<tr>");
    if (i == 0)  strcat( HTML_String, "<td><b>Select Mode</b></td>");
    else strcat( HTML_String, "<td> </td>");
    strcat( HTML_String, "<td><input type = \"radio\" name=\"POSITION_TYPE\" id=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value=\"");
    strcati( HTML_String, i);
    strcat( HTML_String, "\"");
    if (NtripSettings.sendGGAsentence == i)strcat( HTML_String, " CHECKED");
    strcat( HTML_String, "><label for=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, position_type[i]);
    strcat( HTML_String, "</label></td>");
    if (i == 0)
    {
      strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
      strcati(HTML_String, ACTION_SET_SENDPOS);
      strcat( HTML_String, "\">Submit</button></td>");
      strcat( HTML_String, "</tr>");
     }
  }
  strcat( HTML_String, "<tr> <td colspan=\"3\">&nbsp;</td> </tr>");
  
  
  for (int i = 0; i < 3; i++) 
  {
    strcat( HTML_String, "<tr>");
    if (i == 0) {
      //strcat( HTML_String, "<td colspan=\"6\">&nbsp;</td><br>");
      strcat( HTML_String, "<td><b>Repeat time</b></td>");
     }
    else strcat( HTML_String, "<td>&nbsp;</td>");
    strcat( HTML_String, "<td><input type = \"radio\" name=\"REPEATTIME\" id=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value=\"");
    strcati( HTML_String, i);
    strcat( HTML_String, "\"");
    if (NtripSettings.GGAfreq == 1 && i==0)strcat( HTML_String, " CHECKED");
    if (NtripSettings.GGAfreq == 5 && i==1)strcat( HTML_String, " CHECKED");
    if (NtripSettings.GGAfreq == 10 && i==2)strcat( HTML_String, " CHECKED");
    strcat( HTML_String, "><label for=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, repeatPos[i]);
    strcat( HTML_String, "</label></td>");
  }
  strcat( HTML_String, "<tr> <td colspan=\"3\">&nbsp;</td> </tr>");
  
  for (int i = 0; i < 7; i++) 
  {
    strcat( HTML_String, "<tr>");
    if (i == 0) strcat( HTML_String, "<td><b>Baudrate</b></td>");
    else strcat( HTML_String, "<td> </td>");
    strcat( HTML_String, "<td><input type = \"radio\" name=\"BAUDRATESET\" id=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value=\"");
    strcati( HTML_String, i);
    strcat( HTML_String, "\"");
    if ((NtripSettings.baudOut == 9600)   && i==0)strcat( HTML_String, " CHECKED");
    if ((NtripSettings.baudOut == 14400)  && i==1)strcat( HTML_String, " CHECKED");
    if ((NtripSettings.baudOut == 19200)  && i==2)strcat( HTML_String, " CHECKED");
    if ((NtripSettings.baudOut == 38400)  && i==3)strcat( HTML_String, " CHECKED");
    if ((NtripSettings.baudOut == 57600)  && i==4)strcat( HTML_String, " CHECKED");
    if ((NtripSettings.baudOut == 115200) && i==5)strcat( HTML_String, " CHECKED");
    if ((NtripSettings.baudOut == 460800) && i==6)strcat( HTML_String, " CHECKED");
    strcat( HTML_String, "><label for=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, baud_output[i]);
    strcat( HTML_String, "</label></td>");
  }
  
  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br><hr>");

//-------------------------------------------------------------  
// NMEA Sentence
  strcat( HTML_String, "<form>");
  set_colgroup(150, 270, 150, 0, 0);

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<h2>Fixed GGA or RMC Sentence of your Location:</h2>");
  strcat( HTML_String, "You can create your own from  <a href=\"https://www.nmeagen.org/\" target=\"_blank\">www.nmeagen.org</a><br><br>");
  strcat( HTML_String, "<input type=\"text\" style= \"width:650px\" name=\"GGA_MY\" maxlength=\"100\" Value =\"");
  strcat( HTML_String, NtripSettings.GGAsentence);
  strcat( HTML_String, "\"><br><br>");
  
  strcat( HTML_String, "<button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_GGA);
  strcat( HTML_String, "\">Submit</button>");
  strcat( HTML_String, "</tr>");

  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br><hr>");
//-------------------------------------------------------------  
// NMEA transmission
  strcat( HTML_String, "<h2>Component Config</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);

  //for (int i = 0; i < 3; i++) // by JG nur noch OFF und WIFI-TCP
  for (int i = 0; i < 2; i++) 
  {
    strcat( HTML_String, "<tr>");
    if (i == 0)  
     strcat( HTML_String, "<td><b>NTRIP Client</b></td>");
    else 
     strcat( HTML_String, "<td> </td>");

    strcat( HTML_String, "<td><input type = \"radio\" name=\"ENABLENTRIP\" id=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value=\"");
    strcati( HTML_String, i);
    strcat( HTML_String, "\"");

    if (NtripSettings.enableNtrip == i)strcat( HTML_String, " CHECKED");
    strcat( HTML_String, "><label for=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, ntripOn_type[i]);
    strcat( HTML_String, "</label></td>");

    if (i == 0)
    {
      strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
      strcati(HTML_String, ACTION_SET_NMEAOUT);
      strcat( HTML_String, "\">Submit</button></td>");
      strcat( HTML_String, "</tr>");
     } 
  }
  strcat( HTML_String, "<tr> <td colspan=\"3\">&nbsp;</td> </tr>");

  #ifndef USE_BLUETOOTH
    if (NtripSettings.send_UDP_AOG == 2) NtripSettings.send_UDP_AOG =0;
  #endif
  for (int i = 0; i < 3; i++) 
  {
    strcat( HTML_String, "<tr>");
    if (i == 0)  strcat( HTML_String, "<td><b>Transmission Mode</b></td>");
    else strcat( HTML_String, "<td> </td>");
    strcat( HTML_String, "<td><input type = \"radio\" name=\"SENDNMEA_TYPE\" id=\"JZ"); 
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value=\"");
    strcati( HTML_String, i);
    strcat( HTML_String, "\"");
    if (NtripSettings.send_NMEA== i) 
    {strcat( HTML_String, " CHECKED");}

    #ifndef USE_BLUETOOTH
      if ( i == 2) {strcat( HTML_String, " disabled"); //if BT is Disabled by code
    #endif
    strcat( HTML_String, "><label for=\"JZ");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, sendNmea[i]);
    #ifndef USE_BLUETOOTH
      if ( i == 2) strcat( HTML_String, " (disabled by code)"); //if BT is Disabled by code
    #endif
    strcat( HTML_String, "</label></td>");
    
  }
  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br><hr>");
 
  //-------------------------------------------------------------  
  // Checkboxes AHRS
  /*
  strcat( HTML_String, "<h2>Config AHRS Functions</h2>");
  strcat( HTML_String, "<form>");
  strcat( HTML_String, "<table>");
  set_colgroup(150, 270, 150, 0, 0);

  strcat( HTML_String, "<tr>");
  strcat( HTML_String, "<td><b>Select installed</b></td>");
  strcat( HTML_String, "<td>");

  for (int i = 0; i < 2; i++) 
  {
    if (i == 1)strcat( HTML_String, "<br>");
    strcat( HTML_String, "<input type=\"checkbox\" name=\"AHRS_TAG");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" id = \"Part");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value = \"1\" ");
    if (NtripSettings.AHRSbyte & 1 << i) strcat( HTML_String, "checked ");
    strcat( HTML_String, "> ");
    strcat( HTML_String, "<label for =\"Part");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, AHRS_tab[i]);
    strcat( HTML_String, "</label>");
  }
  strcat( HTML_String, "</td>");
  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_SET_AHRS);
  strcat( HTML_String, "\">Submit</button></td>");
  strcat( HTML_String, "</tr>"); 
 
  strcat( HTML_String, "</table>");
  strcat( HTML_String, "</form>");
  strcat( HTML_String, "<br><hr>");
  */
  
//-------------------------------------------------------------  
  strcat( HTML_String, "</font>");
  strcat( HTML_String, "</font>");
  strcat( HTML_String, "</body>");
  strcat( HTML_String, "</html>");
}


//--------------------------------------------------------------------------
void send_HTML() 
{
  char my_char;
  int  my_len = strlen(HTML_String);
  int  my_ptr = 0;
  int  my_send = 0;

  //--------------------------------------------------------------------------
  // in Portionen senden
  while ((my_len - my_send) > 0) 
  {
    my_send = my_ptr + MAX_PACKAGE_SIZE;
    if (my_send > my_len) 
    {
      client_page.print(&HTML_String[my_ptr]);
      delay(22);

      //Serial.println(&HTML_String[my_ptr]);

      my_send = my_len;
    } 
    else 
    {
      my_char = HTML_String[my_send];
      // Auf Anfang eines Tags positionieren
      while ( my_char != '<') my_char = HTML_String[--my_send];
      HTML_String[my_send] = 0;
      client_page.print(&HTML_String[my_ptr]);
      delay(22);
      
      //Serial.println(&HTML_String[my_ptr]);

      HTML_String[my_send] =  my_char;
      my_ptr = my_send;
    }
  }
  //client_page.stop();
}

//--------------------------------------------------------------------------
void send_not_found() 
{

  DBG("\nSend Not Found\n");

  client_page.print("HTTP/1.1 404 Not Found\r\n\r\n");
  delay(20);
  //client_page.stop();
}

//---------------------------------------------------------------------
// Process given values
//---------------------------------------------------------------------
void process_Request()
{ 
  int myIndex;

  if (Find_Start ("/?", HTML_String) < 0 && Find_Start ("GET / HTTP", HTML_String) < 0 )
    {
      //nothing to process
      return;
    }
  action = Pick_Parameter_Zahl("ACTION=", HTML_String);

  // WiFi access data
  if ( action == ACTION_SET_SSID) {

    myIndex = Find_End("SSID_MY=", HTML_String);
    if (myIndex >= 0) {
       for (int i=0;i<24;i++) NtripSettings.ssid[i]=0x00;
       Pick_Text(NtripSettings.ssid, &HTML_String[myIndex], 24);
       //exhibit ("SSID  : ", NtripSettings.ssid);
      }
    myIndex = Find_End("Password_MY=", HTML_String);
    if (myIndex >= 0) {
       for (int i=0;i<24;i++) NtripSettings.password[i]=0x00;
       Pick_Text(NtripSettings.password, &HTML_String[myIndex], 24);
       exhibit ("Password  : ", NtripSettings.password);
       EEprom_write_all();
      }
  }


  if ( action == ACTION_SET_NTRIPCAST) {
    myIndex = Find_End("CASTER=", HTML_String);
    if (myIndex >= 0) {
       for (int i=0;i<40;i++) NtripSettings.host[i]=0x00;
       Pick_Text(NtripSettings.host, &HTML_String[myIndex], 40);
       exhibit ("Caster:   ", NtripSettings.host);
      }
    
    NtripSettings.port = Pick_Parameter_Zahl("CASTERPORT=", HTML_String);
    //exhibit ("Port  : ", NtripSettings.port);
    
    myIndex = Find_End("MOUNTPOINT=", HTML_String);
    if (myIndex >= 0) {
       for (int i=0;i<40;i++) NtripSettings.mountpoint[i]=0x00;
       Pick_Text(NtripSettings.mountpoint, &HTML_String[myIndex], 40);
       //exhibit ("Caster:   ", NtripSettings.mountpoint);
      }
        
    myIndex = Find_End("CASTERUSER=", HTML_String);
    if (myIndex >= 0) {
       for (int i=0;i<40;i++) NtripSettings.ntripUser[i]=0x00;
       Pick_Text(NtripSettings.ntripUser, &HTML_String[myIndex], 40);
       //exhibit ("Username : ", NtripSettings.ntripUser);
      }
    myIndex = Find_End("CASTERPWD=", HTML_String);
    if (myIndex >= 0) {
       for (int i=0;i<40;i++) NtripSettings.ntripPassword[i]=0x00;
       Pick_Text(NtripSettings.ntripPassword, &HTML_String[myIndex], 40);
       //exhibit ("Password  : ", NtripSettings.ntripPassword);
       EEprom_write_all();
       connectCaster(); //reconnect Caster + calc _Auth
      }     
   }  

  if ( action == ACTION_SET_SENDPOS) 
  {
    NtripSettings.sendGGAsentence = Pick_Parameter_Zahl("POSITION_TYPE=", HTML_String);
    if (Pick_Parameter_Zahl("REPEATTIME=", HTML_String)==0) NtripSettings.GGAfreq =1;
    if (Pick_Parameter_Zahl("REPEATTIME=", HTML_String)==1) NtripSettings.GGAfreq =5;
    if (Pick_Parameter_Zahl("REPEATTIME=", HTML_String)==2) NtripSettings.GGAfreq =10;
    
    if (Pick_Parameter_Zahl("BAUDRATESET=", HTML_String)==0) NtripSettings.baudOut = 9600;
    if (Pick_Parameter_Zahl("BAUDRATESET=", HTML_String)==1) NtripSettings.baudOut = 14400;
    if (Pick_Parameter_Zahl("BAUDRATESET=", HTML_String)==2) NtripSettings.baudOut = 19200;
    if (Pick_Parameter_Zahl("BAUDRATESET=", HTML_String)==3) NtripSettings.baudOut = 38400;
    if (Pick_Parameter_Zahl("BAUDRATESET=", HTML_String)==4) NtripSettings.baudOut = 57600;
    if (Pick_Parameter_Zahl("BAUDRATESET=", HTML_String)==5) NtripSettings.baudOut = 115200;   
    if (Pick_Parameter_Zahl("BAUDRATESET=", HTML_String)==6) NtripSettings.baudOut = 460800;   // by JG
    
    //Serial.flush(); // wait for last transmitted data to be sent 
    Serial1.flush(); // wait for last transmitted data to be sent 
    Serial1.end(); // by JG
    delay(100); // by JG

	  Serial1.begin(NtripSettings.baudOut, SERIAL_8N1, RX1, TX1);  //set new Baudrate
    DBG("\nRTCM/NMEA Baudrate: ");
    DBG(NtripSettings.baudOut, 1);
    EEprom_write_all();
   }
  if ( action == ACTION_SET_RESTART) 
  {
    if (EEPROM.read(2)==0)
    {
       EEPROM.write(2,1);
       EEPROM.commit();
       delay(2000);
       //wifi_connected = false;
       ESP.restart();
       
     } 
   }

  if ( action == ACTION_SET_GGA) 
  {
    myIndex = Find_End("GGA_MY=", HTML_String);
    if (myIndex >= 0) {
       for (int i=0;i<100;i++) NtripSettings.GGAsentence[i]=0x00;
       Pick_Text(NtripSettings.GGAsentence, &HTML_String[myIndex], 100);
       exhibit ("NMEA: ", NtripSettings.GGAsentence);
      }
    EEprom_write_all();
  }

    if ( action == ACTION_SET_NMEAOUT) 
    {
       NtripSettings.send_NMEA = Pick_Parameter_Zahl("SENDNMEA_TYPE=", HTML_String);
       byte old = NtripSettings.enableNtrip;
       NtripSettings.enableNtrip = (byte)Pick_Parameter_Zahl("ENABLENTRIP=", HTML_String);

       if (NtripSettings.enableNtrip == 1 && old == 0 ) 
       {
         restart = 0; 
       } 
       EEprom_write_all();   
       exhibit ("NMEA: ", NtripSettings.enableNtrip); 
     } 

    /* by JG
   if ( action == ACTION_SET_AHRS) 
   {
    NtripSettings.AHRSbyte = 0;
    char tmp_string[20];
    for (int i = 0; i < 2; i++) {
      strcpy( tmp_string, "AHRS_TAG");
      strcati( tmp_string, i);
      strcat( tmp_string, "=");
      if (Pick_Parameter_Zahl(tmp_string, HTML_String) == 1)NtripSettings.AHRSbyte |= 1 << i;
     }

    EEprom_write_all();
   }
   */
   if ( action == ACTION_DELETE_GCP) 
    {
       Serial.println("Action DELETE GCP");
       
       for (int i=0; i<10; i++)
       {
         String s= "gpx";
         s.toCharArray(NtripSettings.gcp[i],13);
       }
       EEprom_write_all(); 
       delay(1);
       exhibit ("Ground-Control-Point Delete", "gpx"); 
       
     } 



   // by JG
   if (action == ACTION_UPDATE_TIME_AND_POS)
   {
     exhibit("Time-Second", gps.time.second());
   }
}  
//---------------------------------------------------------------------
void WiFi_Traffic() 
{
  /* by JG .... wurden auch vorher nicht gebraucht
  char my_char;
  int htmlPtr = 0;
  int myIdx;
  int myIndex;
  */
  unsigned long my_timeout;

   
  // Check if a client has connected
  client_page = server.available();
  
  if (!client_page)  return;

  //DBG("New Client:\n");           // print a message out the serial port
  
  my_timeout = millis() + 250L;
  while (!client_page.available() && (millis() < my_timeout) ) delay(10);
  delay(10);
  if (millis() > my_timeout)  
  {
      //DBG("Client connection timeout!\n");
      return;
  }
  //---------------------------------------------------------------------
  //htmlPtr = 0;
  char c; 
  if (client_page) 
  {                        // if you get a client,
    //DBG("New Client.\n");                   // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client_page.connected()) 
    {       // loop while the client's connected
      if (client_page.available()) 
      {        // if there's bytes to read from the client,
        c = client_page.read();        // read a byte, then
        //DBG(c);                             // print it out the serial monitor
        if (c == '\n') 
        {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) 
          {
           
            make_HTML01();  // create Page array
           //---------------------------------------------------------------------
           // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
           // and a content-type so the client knows what's coming, then a blank line:
           strcpy(HTTP_Header , "HTTP/1.1 200 OK\r\n");
           strcat(HTTP_Header, "Content-Length: ");
           strcati(HTTP_Header, strlen(HTML_String));
           strcat(HTTP_Header, "\r\n");
           strcat(HTTP_Header, "Content-Type: text/html\r\n");
           strcat(HTTP_Header, "Connection: close\r\n");
           strcat(HTTP_Header, "\r\n");

           client_page.print(HTTP_Header);
           delay(20);
           send_HTML();
           
            // break out of the while loop:
            break;
          }
          else 
          {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } 
        else if (c != '\r') 
        { // if you got anything else but a carriage return character,
             currentLine += c;      // add it to the end of the currentLine
             if (currentLine.endsWith("HTTP")) 
               {
                if (currentLine.startsWith("GET ")) 
                 {
                  currentLine.toCharArray(HTML_String,currentLine.length());
                  //DBG("", 1); //NL
                  exhibit ("Request : ", HTML_String);
                  process_Request();
                 }  
               }
        }//end else
      } //end client available
    } //end while client.connected
    // close the connection:
    client_page.stop();
    //DBG("Pagelength : ");
    //DBG((long)strlen(HTML_String));
    //DBG("   --> Client Disconnected\n");
 }// end if client 
}


