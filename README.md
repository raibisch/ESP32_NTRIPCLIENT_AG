# ESP32_NTRIPCLIENT_AG
a ESP32 NTRIP Client 

based on the forked work from :
Example NTRIP client written with Arduino IDE for an espressif ESP32. (C) 2019- Wilhelm Eder
Enhanced by Matthias H.(MTZ) with Bluetooth, IMU (BNO055) and Roll Sensor MMA8452
Source: https://github.com/Coffeetrac/AG_NTRIP_ESP

Changes to original version
- change: complete neu source file structure
- change: forward serial NMEA aata to Bluetooth-serial
- remove: IMU part of original code
- add:  : lon/lat to UMT-Converter
- add:  : generate GCP (Ground-Control-Points) for ODM (opendronemap) https://docs.opendronemap.org/gcp/


