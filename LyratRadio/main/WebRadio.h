/****************************************************************************************
  WebRadio - Internet radio using Espressif's Lyrat board
  Written by Sebastian Hinz, http://radio-online.eu/
  Based on: Espressif ESP-ADF build environment and examples

  This code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
****************************************************************************************/

#ifndef _WEBRADIO_H_
#define _WEBRADIO_H_

#include <string>
#include "periph_wifi.h"
#include "audio_pipeline.h"
#include "periph_service.h"
#include "board.h"

#include "WifiWebRadio.h"
#include "DataWebRadio.h"
#include "mp3_decoder.h"

extern "C" {
audio_element_handle_t create_mp3_decoder();
audio_element_handle_t create_aac_decoder();
audio_element_handle_t create_i2s_stream(audio_stream_type_t type);
wifi_config_t* get_wifi_config_t();
void esp_msg_debug(audio_event_iface_msg_t msg);
void wifi_init_softap();
void wifi_init_client();
}

#define WEBRADIO_HTTP_STREAM_CFG_DEFAULT()          \
    {                                               \
        .type = AUDIO_STREAM_READER,                \
        .out_rb_size = HTTP_STREAM_RINGBUFFER_SIZE, \
        .task_stack = HTTP_STREAM_TASK_STACK,       \
        .task_core = HTTP_STREAM_TASK_CORE,         \
        .task_prio = HTTP_STREAM_TASK_PRIO,         \
        .stack_in_ext = false,                      \
        .event_handle = 0,                          \
        .user_data = 0,                             \
        .auto_connect_next_track = 0,               \
        .enable_playlist_parser = 0,                \
        .multi_out_num = 0,                         \
    \
}

extern const char* TAG;

//////////////////////////////////////////////////////////////////////
class IWebRadioCommands {
public:
    virtual void SetStation(int actStation) = 0;
    virtual void SetNextStation() = 0;
    virtual void SetPreviousStation() = 0;
    virtual void SetOnOff(bool bOn) = 0;
    virtual void SetVolume(int volume) = 0;
};

//////////////////////////////////////////////////////////////////////
class WebRadio : public IWebRadioCommands {
public:
    enum DecoderType_e {
        MP3,
        AAC,
    };

public:
    WebRadio();
    void Start();

    WifiWebRadio& GetWifiWebRadio() { return mWifi; }
    DataWebRadio& GetDataWebRadio() { return mData; }
    IWebRadioCommands& GetCommandInterface() { return *this; }

    // command interface
    void SetStation(int actStation);
    void SetNextStation();
    void SetPreviousStation();
    void SetOnOff(bool bOn);
    void SetVolume(int volume);

private:
    void AudioPipeline();
    bool key_handler(audio_event_iface_msg_t& msg);
    void AudioPipelineSwitchStation();

private:
    esp_periph_set_handle_t mSet;
    audio_pipeline_handle_t mPipeline;
    audio_board_handle_t mAudioBoardHandle;
    audio_element_handle_t mHttp_stream_reader;
    audio_element_handle_t mMp3_decoder;
    audio_element_handle_t mAac_decoder;
    audio_event_iface_handle_t mEvt;

    WifiWebRadio mWifi;
    DataWebRadio mData;
};

////////////////////////////////////////////////////////////////////////////////

#endif
