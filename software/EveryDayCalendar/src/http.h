
#ifndef __HTTP_H__
#define __HTTP_H__

#include "ESPAsyncWebServer.h" // Used for displaying webpages

void httpIndex(AsyncWebServerRequest *request);
void httpDatabase(AsyncWebServerRequest *request);
void httpSet(AsyncWebServerRequest *request);
void httpResetWifi(AsyncWebServerRequest *request);
void httpReset(AsyncWebServerRequest *request);

#endif // __HTTP_H__