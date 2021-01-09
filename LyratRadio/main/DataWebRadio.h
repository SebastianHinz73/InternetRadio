/****************************************************************************************
  WebRadio - Internet radio using Espressif's Lyrat board
  Written by Sebastian Hinz, http://radio-online.eu/
  Based on: Espressif ESP-ADF build environment and examples

  This code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
****************************************************************************************/

#ifndef _DATAWEBBRADIO_H_
#define _DATAWEBBRADIO_H_

////////////////////////////////////////////////////////////////////////////////
#include "NVSWebRadio.h"
#include "string"

class WebRadio;

//////////////////////////////////////////////////////////////////////
class DataWebRadio : public NVSWebRadio {
public:
    enum MessageType_e {
        NoWebRadioRequest,
        FindBoard,
        Configuration,
        PlayIds,
    };

public:
    DataWebRadio();
    esp_err_t Initialize(WebRadio* webRadio);

    // functions for udp and tcp message parsing
    MessageType_e IsWebRadioRequest(char* pRequest);
    void HandleMessage(MessageType_e msg, char* buffer, size_t size);
    bool CreateMessageResponse(MessageType_e msg, char* buffer, size_t size);
    void Debug(Settings_t& set);

    // functions
private:
    // variable
private:
    WebRadio* mWebRadio;
};

////////////////////////////////////////////////////////////////////////////////

#endif
