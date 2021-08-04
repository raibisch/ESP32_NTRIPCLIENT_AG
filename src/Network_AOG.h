#ifndef NETWORK_AOG_H_
#define NETWORK_AOG_H_


#include <Arduino.h>

// all about Network
#include <WiFi.h>
#include <AsyncUDP.h>

#include <WiFiAP.h>
#include <WiFiClient.h>


// Wifi variables & definitions

#define MAX_PACKAGE_SIZE 2048
extern char HTML_String[];
extern char HTTP_Header[];



extern char strmBuf[];    
extern char lastSentence[100];
extern bool newSentence;
extern byte gpsBuffer[100];
//-------------------
extern unsigned long Ntrip_data_time;

//---------------------------------------------------------------------
extern String _userAgent;

// Allgemeine Variablen
extern String _base64Authorization;
extern String _accept;

extern WiFiClient ntripCl;
extern WiFiClient client_page;
extern AsyncUDP udpNtrip;

// by JG NMEA over TCP/IP for u-center communication
extern char rxGPSBuf[510];        // by: JG ReceiveBuffer for Serial1 (GPS-Modul)
extern WiFiServer tcpNMEAserver;
extern WiFiClient tcpNMEAclient;



#define ACTION_SET_SSID        1  
#define ACTION_SET_NTRIPCAST   2
#define ACTION_SET_SENDPOS     3
#define ACTION_SET_RESTART     4
#define ACTION_SET_GGA         5
#define ACTION_SET_NMEAOUT     6
#define ACTION_SET_AHRS        7
#define ACTION_UPDATE_TIME_AND_POS 8
#define ACTION_DELETE_GCP      9


extern void WiFi_Start_STA();
extern void WiFi_Start_AP();
extern void WiFi_Traffic();
extern void UDPReceiveNtrip();

extern bool getSourcetable();
extern bool startStream();
extern bool getRtcmData();

extern void tcpNMEAIO();
#endif
