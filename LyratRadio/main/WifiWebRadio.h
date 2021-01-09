/****************************************************************************************
  WebRadio - Internet radio using Espressif's Lyrat board
  Written by Sebastian Hinz, http://radio-online.eu/
  Based on: Espressif ESP-ADF build environment and examples

  This code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
****************************************************************************************/

#ifndef _WIFIWEBRADIO_H_
#define _WIFIWEBRADIO_H_

#include "periph_wifi.h"

class WebRadio;

//////////////////////////////////////////////////////////////////////
class WifiWebRadio {
public:
    WifiWebRadio();
    esp_err_t StartClient(WebRadio* webRadio, esp_periph_set_handle_t& set);
    esp_err_t StartAccessPoint(WebRadio* webRadio, esp_periph_set_handle_t& set);
    esp_err_t Initialize(WebRadio* webRadio, esp_periph_set_handle_t& set);
    const std::string& getIp() { return mIp; }
    const std::string& getId() { return mId; }
    static esp_err_t event_handler(void* ctx, system_event_t* event);

private:
    static void udp_server_task(void* pvParameters);

private:
    /* FreeRTOS event group to signal when we are connected*/
    // EventGroupHandle_t s_wifi_event_group;
    static WebRadio* mWebRadio;
    std::string mId;
    std::string mIp;
};

////////////////////////////////////////////////////////////////////////////////

#endif
