#ifndef _secrets_h_
#define _secrets_h_

#define UDP_GENERIC_PORT 2240
#define UDP_REQUEST_PORT 2241
#define UDP_RESPONSE_PORT 2242
/*############################################################################ Classified information ############################################################################*/
//
#define MY_SSID "noInternet" // SSID of the access point
#define PASSWORD "changeMePlease" // Password of the access point
//
/*############################################################################ Classified information ############################################################################*/
IPAddress apIP(192, 168, 4, 1); // Access point IP
IPAddress receiverIP(192, 168, 4, 2);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 4, 1);
#endif