/* 
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_peripherals.h"

#include "audio_element.h"
#include "i2s_stream.h"
#include "http_stream.h"
#include "mp3_decoder.h"
#include "aac_decoder.h"

extern const char* TAG;

audio_element_handle_t create_mp3_decoder()
{
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    return mp3_decoder_init(&mp3_cfg);
}

audio_element_handle_t create_aac_decoder()
{
    aac_decoder_cfg_t aac_cfg = DEFAULT_AAC_DECODER_CONFIG();
    return aac_decoder_init(&aac_cfg);
}

audio_element_handle_t create_i2s_stream(audio_stream_type_t type)
{
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = type;
    audio_element_handle_t i2s_stream = i2s_stream_init(&i2s_cfg);
    mem_assert(i2s_stream);
    return i2s_stream;
}

void esp_msg_debug(audio_event_iface_msg_t msg)
{
    /*
	typedef struct {
    int cmd;                //!< Command id 
    void *data;             //!< Data pointer 
    int data_len;           //!< Data length 
    void *source;           //!< Source event
    int source_type;        //!< Source type (To know where it came from) 
    bool need_free_data;    //!< Need to free data pointer after the event has been processed 
} audio_event_iface_msg_t;
*/

    char* pSourceType = "unknown";
    char* pMsgCommand = "unknown";

    switch (msg.source_type) {
    case AUDIO_ELEMENT_TYPE_UNKNOW:
        pSourceType = "AUDIO_ELEMENT_TYPE_UNKNOW";
        break;
    case AUDIO_ELEMENT_TYPE_ELEMENT:
        pSourceType = "AUDIO_ELEMENT_TYPE_ELEMENT";
        switch (msg.cmd) {
        case AEL_MSG_CMD_NONE:
            pMsgCommand = "AEL_MSG_CMD_NONE";
            break;
        case AEL_MSG_CMD_ERROR:
            pMsgCommand = "AEL_MSG_CMD_ERROR";
            break;
        case AEL_MSG_CMD_FINISH:
            pMsgCommand = "AEL_MSG_CMD_FINISH";
            break;
        case AEL_MSG_CMD_STOP:
            pMsgCommand = "AEL_MSG_CMD_STOP";
            break;
        case AEL_MSG_CMD_PAUSE:
            pMsgCommand = "AEL_MSG_CMD_PAUSE";
            break;
        case AEL_MSG_CMD_RESUME:
            pMsgCommand = "AEL_MSG_CMD_RESUME";
            break;
        case AEL_MSG_CMD_DESTROY:
            pMsgCommand = "AEL_MSG_CMD_DESTROY";
            break;
        case AEL_MSG_CMD_REPORT_STATUS:
            pMsgCommand = "AEL_MSG_CMD_REPORT_STATUS";
            break;
        case AEL_MSG_CMD_REPORT_MUSIC_INFO:
            pMsgCommand = "AEL_MSG_CMD_REPORT_MUSIC_INFO";
            break;
        case AEL_MSG_CMD_REPORT_CODEC_FMT:
            pMsgCommand = "AEL_MSG_CMD_REPORT_CODEC_FMT";
            break;
        case AEL_MSG_CMD_REPORT_POSITION:
            pMsgCommand = "AEL_MSG_CMD_REPORT_POSITION";
            break;
        }
        break;
    case AUDIO_ELEMENT_TYPE_PLAYER:
        pSourceType = "AUDIO_ELEMENT_TYPE_PLAYER";
        break;
    case AUDIO_ELEMENT_TYPE_SERVICE:
        pSourceType = "AUDIO_ELEMENT_TYPE_SERVICE";
        break;
    case AUDIO_ELEMENT_TYPE_PERIPH:
        pSourceType = "AUDIO_ELEMENT_TYPE_PERIPH";
        break;
    case PERIPH_ID_BUTTON:
        pSourceType = "PERIPH_ID_BUTTON";
        break;
    case PERIPH_ID_TOUCH:
        pSourceType = "PERIPH_ID_TOUCH";
        break;
    case PERIPH_ID_SDCARD:
        pSourceType = "PERIPH_ID_SDCARD";
        break;
    case PERIPH_ID_WIFI:
        pSourceType = "PERIPH_ID_WIFI";
        break;
    case PERIPH_ID_FLASH:
        pSourceType = "PERIPH_ID_FLASH";
        break;
    case PERIPH_ID_AUXIN:
        pSourceType = "PERIPH_ID_AUXIN";
        break;
    case PERIPH_ID_ADC:
        pSourceType = "PERIPH_ID_ADC";
        break;
    case PERIPH_ID_CONSOLE:
        pSourceType = "PERIPH_ID_CONSOLE";
        break;
    case PERIPH_ID_BLUETOOTH:
        pSourceType = "PERIPH_ID_BLUETOOTH";
        break;
    case PERIPH_ID_LED:
        pSourceType = "PERIPH_ID_LED";
        break;
    case PERIPH_ID_SPIFFS:
        pSourceType = "PERIPH_ID_SPIFFS";
        break;
    case PERIPH_ID_ADC_BTN:
        pSourceType = "PERIPH_ID_ADC_BTN";
        break;
    case PERIPH_ID_IS31FL3216:
        pSourceType = "PERIPH_ID_IS31FL3216";
        break;
    case PERIPH_ID_GPIO_ISR:
        pSourceType = "PERIPH_ID_GPIO_ISR";
        break;
    case PERIPH_ID_WS2812:
        pSourceType = "PERIPH_ID_WS2812";
        break;
    case PERIPH_ID_AW2013:
        pSourceType = "PERIPH_ID_AW2013";
        break;
    }

    ESP_LOGI(TAG, "[ * ] Receive msg source_type=%s, cmd=%s", pSourceType, pMsgCommand);
}
