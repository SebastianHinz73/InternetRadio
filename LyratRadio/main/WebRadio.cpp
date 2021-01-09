/****************************************************************************************
  WebRadio - Internet radio using Espressif's Lyrat board
  Written by Sebastian Hinz, http://radio-online.eu/
  Based on: Espressif ESP-ADF build environment and examples

  This code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
****************************************************************************************/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "audio_element.h"

#include "audio_event_iface.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "http_stream.h"
#include "mp3_decoder.h"
#include "aac_decoder.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"

#include "periph_touch.h"
#include "periph_adc_button.h"
#include "periph_button.h"

#include "input_key_service.h"
#include "board.h"

#include "DataWebRadio.h"
#include "WifiWebRadio.h"
#include "WebRadio.h"

const char* TAG = "WebRadio";

///////////////////////////////////////////////////////////////////////////////
WebRadio webRadio;

///////////////////////////////////////////////////////////////////////////////
WebRadio::WebRadio()
{
}

///////////////////////////////////////////////////////////////////////////////
void WebRadio::Start()
{
    tcpip_adapter_init();

    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    mSet = esp_periph_set_init(&periph_cfg);

    esp_err_t err = mData.Initialize(this);
    ESP_ERROR_CHECK(err);

    if (err == ESP_OK) {
        // start client mode
        if (!mData.EmptyCredentials()) {
            mWifi.StartClient(this, mSet);

            while (1) {
                AudioPipeline();

                printf("loop AudioPipeline -> error\n");
                vTaskDelay(4000 / portTICK_PERIOD_MS);
            }
        }
        else // start access point mode
        {
            mWifi.StartAccessPoint(this, mSet);

            printf("loop access point mode\n");
            while (1) // while loop in access point mode (no internet access)
            {
                vTaskDelay(4000 / portTICK_PERIOD_MS);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void WebRadio::AudioPipeline()
{
    audio_element_handle_t i2s_stream_writer;

    ESP_LOGI(TAG, "[ 1 ] Start audio codec chip");
    audio_board_key_init(mSet);
    mAudioBoardHandle = audio_board_init();
    audio_hal_ctrl_codec(mAudioBoardHandle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[2.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    mPipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(mPipeline);

    ESP_LOGI(TAG, "[2.1] Create http stream to read data");
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    mHttp_stream_reader = http_stream_init(&http_cfg);

    ESP_LOGI(TAG, "[2.2] Create i2s stream to write data to codec chip");
    i2s_stream_writer = create_i2s_stream(AUDIO_STREAM_WRITER);

    ESP_LOGI(TAG, "[2.3] Create mp3 decoder to decode mp3 file");
    mMp3_decoder = create_mp3_decoder();

    ESP_LOGI(TAG, "[2.3] Create aac decoder to decode aac file");
    mAac_decoder = create_aac_decoder();

    ESP_LOGI(TAG, "[2.4] Register all elements to audio pipeline");
    audio_pipeline_register(mPipeline, mHttp_stream_reader, "http");
    audio_pipeline_register(mPipeline, mMp3_decoder, "mp3");
    audio_pipeline_register(mPipeline, mAac_decoder, "aac");
    audio_pipeline_register(mPipeline, i2s_stream_writer, "i2s");

    Settings_t set;
    mData.GetSettings(set);
    //mData.Debug(set);
    Station_t& station = (set.mActStation == -1) ? set.mActTune : set.mStations[set.mActStation];

    // increment check-reset count
    mData.IncStationCheck(station, CheckListResult::Undefined);

    audio_hal_set_volume(mAudioBoardHandle->audio_hal, set.mVolume);

    ESP_LOGI(TAG, "[2.5] Link it together http_stream-->audio_decoder(%s)-->i2s_stream-->[codec_chip]", station.mDecoder.c_str());
    const char* link_tag[3] = { "http", station.mDecoder.c_str(), "i2s" };
    audio_pipeline_link(mPipeline, &link_tag[0], 3);

    ESP_LOGI(TAG, "[2.6] Set up  uri (http as http_stream, mp3 as mp3 decoder, and default output is i2s) '%s'", station.mUrl.c_str());
    audio_element_set_uri(mHttp_stream_reader, station.mUrl.c_str());

    ESP_LOGI(TAG, "[ 3 ] Start and wait for Wi-Fi network");

    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    mEvt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(mPipeline, mEvt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(mSet), mEvt);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(mPipeline);

    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(mEvt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }
        esp_msg_debug(msg);

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void*)mMp3_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info;
            audio_element_getinfo(mMp3_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);

            mData.GetSettings(set);
            Station_t& station = (set.mActStation == -1) ? set.mActTune : set.mStations[set.mActStation];

            // reset check-reset count,
            mData.IncStationCheck(station, CheckListResult::Valid);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void*)mAac_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info;
            audio_element_getinfo(mAac_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from aac decoder, sample_rates=%d, bits=%d, ch=%d",
                music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);

            mData.GetSettings(set);
            Station_t& station = (set.mActStation == -1) ? set.mActTune : set.mStations[set.mActStation];

            // reset check-reset count,
            mData.IncStationCheck(station, CheckListResult::Valid);
            continue;
        }

        if ((msg.source_type == PERIPH_ID_TOUCH || msg.source_type == PERIPH_ID_BUTTON || msg.source_type == PERIPH_ID_ADC_BTN)
            && (msg.cmd == PERIPH_TOUCH_TAP || msg.cmd == PERIPH_BUTTON_PRESSED || msg.cmd == PERIPH_ADC_BUTTON_PRESSED)) {
            key_handler(msg);
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*)i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_stop(mPipeline);
    audio_pipeline_wait_for_stop(mPipeline);
    audio_pipeline_terminate(mPipeline);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_unregister(mPipeline, mHttp_stream_reader);
    audio_pipeline_unregister(mPipeline, i2s_stream_writer);
    audio_pipeline_unregister(mPipeline, mMp3_decoder);
    audio_pipeline_unregister(mPipeline, mAac_decoder);

    audio_pipeline_remove_listener(mPipeline);

    /* Stop all peripherals before removing the listener */
    esp_periph_set_stop_all(mSet);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(mSet), mEvt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(mEvt);

    /* Release all resources */
    audio_pipeline_deinit(mPipeline);
    audio_element_deinit(mHttp_stream_reader);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(mMp3_decoder);
    audio_element_deinit(mAac_decoder);
    esp_periph_set_destroy(mSet);
}

///////////////////////////////////////////////////////////////////////////////
bool WebRadio::key_handler(audio_event_iface_msg_t& msg)
{
    IWebRadioCommands& command = GetCommandInterface();

    int player_volume;

    if ((int)msg.data == get_input_play_id()) {
        ESP_LOGI(TAG, "[ * ] [Play] touch tap event");
        command.SetPreviousStation();
        return true;
    }
    else if ((int)msg.data == get_input_set_id()) {
        ESP_LOGI(TAG, "[ * ] [Set] touch tap event");
        command.SetNextStation();
        return true;
    }
    else if ((int)msg.data == get_input_volup_id()) {
        audio_hal_get_volume(mAudioBoardHandle->audio_hal, &player_volume);

        ESP_LOGI(TAG, "[ * ] [Vol+] touch tap event");
        player_volume += 10;
        if (player_volume > 100) {
            player_volume = 100;
        }
        command.SetVolume(player_volume);
        ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
    }
    else if ((int)msg.data == get_input_voldown_id()) {
        audio_hal_get_volume(mAudioBoardHandle->audio_hal, &player_volume);

        ESP_LOGI(TAG, "[ * ] [Vol-] touch tap event");
        player_volume -= 10;
        if (player_volume < 0) {
            player_volume = 0;
        }
        command.SetVolume(player_volume);
        ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
void WebRadio::AudioPipelineSwitchStation()
{
    Settings_t set;
    mData.GetSettings(set);
    //mData.Debug(set);
    Station_t& station = (set.mActStation == -1) ? set.mActTune : set.mStations[set.mActStation];

    // increment check-reset count
    mData.IncStationCheck(station, CheckListResult::Undefined);

    esp_err_t err, err1, err2;

    err = audio_pipeline_stop(mPipeline);
    err1 = audio_pipeline_wait_for_stop(mPipeline);
    err2 = audio_pipeline_terminate(mPipeline);
    ESP_LOGI(TAG, "[ switch ] stop pipeline => %s, %s, %s", esp_err_to_name(err), esp_err_to_name(err1), esp_err_to_name(err2));

    const char* link_tag[3] = { "http", station.mDecoder.c_str(), "i2s" };

    err = audio_pipeline_breakup_elements(mPipeline, mMp3_decoder);
    err1 = audio_pipeline_breakup_elements(mPipeline, mAac_decoder);
    ESP_LOGI(TAG, "[ switch ] pipeline breakup elements => %s, %s", esp_err_to_name(err), esp_err_to_name(err1));

    err = audio_pipeline_relink(mPipeline, &link_tag[0], 3);
    ESP_LOGI(TAG, "[ switch ] Relink it together http_stream-->audio_decoder(%s)-->i2s_stream-->[codec_chip] => %s", station.mDecoder.c_str(), esp_err_to_name(err));

    err = audio_pipeline_set_listener(mPipeline, mEvt);
    err1 = audio_element_set_uri(mHttp_stream_reader, station.mUrl.c_str());
    ESP_LOGI(TAG, "[ switch ] Set up  uri '%s' => %s, %s", station.mUrl.c_str(), esp_err_to_name(err), esp_err_to_name(err1));

    err = audio_pipeline_reset_ringbuffer(mPipeline);
    err1 = audio_pipeline_reset_elements(mPipeline);
    err2 = audio_pipeline_run(mPipeline);

    ESP_LOGI(TAG, "[ switch ] reset and run %s, %s, %s", esp_err_to_name(err), esp_err_to_name(err1), esp_err_to_name(err2));
}

///////////////////////////////////////////////////////////////////////////////
// Command Interface
///////////////////////////////////////////////////////////////////////////////
void WebRadio::SetStation(int actStation)
{
    ESP_LOGI(TAG, "[ * ] SetStation %d", actStation);
    mData.SetActStation(actStation);
    AudioPipelineSwitchStation();
}

///////////////////////////////////////////////////////////////////////////////
void WebRadio::SetNextStation()
{
    Settings_t settings;
    mData.GetSettings(settings);

    int next = settings.mActStation + 1;
    if (next == settings.mStations.size()) {
        next = 0;
    }

    mData.SetActStation(next);
    AudioPipelineSwitchStation();
}

///////////////////////////////////////////////////////////////////////////////
void WebRadio::SetPreviousStation()
{
    Settings_t settings;
    mData.GetSettings(settings);

    int prev = settings.mActStation - 1;

    if (prev < 0) {
        prev = settings.mStations.size() - 1;
    }

    mData.SetActStation(prev);
    AudioPipelineSwitchStation();
}

///////////////////////////////////////////////////////////////////////////////
void WebRadio::SetOnOff(bool bOn)
{
}

///////////////////////////////////////////////////////////////////////////////
void WebRadio::SetVolume(int volume)
{
    mData.SetVolume(volume);
    audio_hal_set_volume(mAudioBoardHandle->audio_hal, volume);
}

///////////////////////////////////////////////////////////////////////////////
// entry point for esp-idf framework
extern "C" void app_main()
{
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    webRadio.Start();

    printf("loop app_main -> error\n");

    while (1) // while loop on error
    {
        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
}
