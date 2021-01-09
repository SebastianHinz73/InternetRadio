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
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "cJSON.h"
#include "WebRadio.h"
#include "DataWebRadio.h"

///////////////////////////////////////////////////////////////////////////////
DataWebRadio::DataWebRadio()
{
}

///////////////////////////////////////////////////////////////////////////////
esp_err_t DataWebRadio::Initialize(WebRadio* webRadio)
{
    mWebRadio = webRadio;

    esp_err_t err = NVSWebRadio::Initialize();
    if (err == ESP_OK) {
        Settings_t set;
        GetSettings(set);
        Debug(set);
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////
void DataWebRadio::Debug(Settings_t& set)
{
    ESP_LOGI(TAG, "[ DATA ] mCredentials %s, %s", set.mCredentials.mSSID.c_str(), /*set.mCredentials.mPassword.c_str()*/ "*****");
    ESP_LOGI(TAG, "[ DATA ] mBluetooth %d, %s", set.mBluetooth.mbEnabled, set.mBluetooth.mPair.c_str());
    ESP_LOGI(TAG, "[ DATA ] mActTune %s, %s, %s", set.mActTune.mUrl.c_str(), set.mActTune.mDecoder.c_str(), set.mActTune.mId.c_str());

    // Station_t presets
    for (int i = 0; i < set.mStations.size(); i++) {
        ESP_LOGI(TAG, "[ DATA ] mStations[%d] %s, %s, %s", i, set.mStations[i].mId.c_str(), set.mStations[i].mUrl.c_str(), set.mStations[i].mDecoder.c_str());
    }
    ESP_LOGI(TAG, "[ DATA ] mRadioName %s", set.mRadioName.c_str());
    ESP_LOGI(TAG, "[ DATA ] mVolume %d", set.mVolume);
    ESP_LOGI(TAG, "[ DATA ] mActStation %d", set.mActStation);
    ESP_LOGI(TAG, "[ DATA ] mRestartCount %d", GetRestartCount());
}

///////////////////////////////////////////////////////////////////////////////
void DataWebRadio::HandleMessage(MessageType_e msg, char* buffer, size_t size)
{
    Settings_t set;

    cJSON* root = cJSON_Parse(buffer); // message is valid json message string (alredy checked in IsWebRadioRequest
    cJSON* webradio = cJSON_GetObjectItem(root, LYRAT_NET_WEBRADIO);

    IWebRadioCommands& command = mWebRadio->GetCommandInterface();

    switch (msg) {
    case DataWebRadio::Configuration: {
        cJSON* configuration = cJSON_GetObjectItem(webradio, LYRAT_NET_CONFIGURATION);

        if (cJSON_HasObjectItem(configuration, LYRAT_NET_BLUETOOTH)) {
            cJSON* bluetooth = cJSON_GetObjectItem(configuration, LYRAT_NET_BLUETOOTH);
            cJSON* jsonEnabled = cJSON_GetObjectItem(bluetooth, LYRAT_NET_BT_ENABLED);
            if (jsonEnabled != NULL) {
                set.mBluetooth.mbEnabled = jsonEnabled->valueint;
            }
            set.mBluetooth.mPair = cJSON_GetStringValue(cJSON_GetObjectItem(bluetooth, LYRAT_NET_BT_PAIR));
            SetBluetooth(set.mBluetooth);
        }
        if (cJSON_HasObjectItem(configuration, LYRAT_NET_STATIONLIST)) {
            cJSON* stationList = cJSON_GetObjectItem(configuration, LYRAT_NET_STATIONLIST);
            int sizeArr = cJSON_GetArraySize(stationList);
            for (int i = 0; i < sizeArr; i++) {
                Station_t station;
                cJSON* jsonStation = cJSON_GetArrayItem(stationList, i);

                station.mId = cJSON_GetStringValue(cJSON_GetObjectItem(jsonStation, LYRAT_NET_ST_ID));
                station.mUrl = cJSON_GetStringValue(cJSON_GetObjectItem(jsonStation, LYRAT_NET_ST_URL));
                station.mDecoder = cJSON_GetStringValue(cJSON_GetObjectItem(jsonStation, LYRAT_NET_ST_DECODER));
                set.mStations.push_back(station);

                SetStation(i, set.mStations[i], set.mStations.size());
            }
            command.SetStation(0); // reset counter to 0
        }
        if (cJSON_HasObjectItem(configuration, LYRAT_NET_ACTTUNE)) {
            cJSON* station = cJSON_GetObjectItem(configuration, LYRAT_NET_ACTTUNE);

            set.mActTune.mId = cJSON_GetStringValue(cJSON_GetObjectItem(station, LYRAT_NET_ST_ID));
            set.mActTune.mUrl = cJSON_GetStringValue(cJSON_GetObjectItem(station, LYRAT_NET_ST_URL));
            set.mActTune.mDecoder = cJSON_GetStringValue(cJSON_GetObjectItem(station, LYRAT_NET_ST_DECODER));
            SetStation(-1, set.mActTune);
            SetActStation(-1);
            command.SetStation(set.mActStation);
        }
        if (cJSON_HasObjectItem(configuration, LYRAT_NET_RADIO)) {
            std::string newName = cJSON_GetStringValue(cJSON_GetObjectItem(configuration, LYRAT_NET_RADIO));
            if (!newName.empty()) {
                set.mRadioName = newName;
                //printf("###### set.mRadioName %s", set.mRadioName.c_str());
                SetName(set.mRadioName);
            }
        }
        if (cJSON_HasObjectItem(configuration, LYRAT_NET_VOLUME)) {
            cJSON* volume = cJSON_GetObjectItem(configuration, LYRAT_NET_VOLUME);
            if (volume != NULL) {
                set.mVolume = volume->valueint;
            }
            SetVolume(set.mVolume);

            command.SetVolume(set.mVolume);
        }
        if (cJSON_HasObjectItem(configuration, LYRAT_NET_CREDENTIALS)) {
            GetCredentials(set.mCredentials); // default init

            cJSON* credentials = cJSON_GetObjectItem(configuration, LYRAT_NET_CREDENTIALS);
            set.mCredentials.mSSID = cJSON_GetStringValue(cJSON_GetObjectItem(credentials, LYRAT_NET_RADIOSSID));
            if (cJSON_GetObjectItem(credentials, LYRAT_NET_RADIOPASSWD) != NULL) {
                set.mCredentials.mPassword = cJSON_GetStringValue(cJSON_GetObjectItem(credentials, LYRAT_NET_RADIOPASSWD));
            }
            SetCredentials(set.mCredentials);

            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        }
        Settings_t set;
        GetSettings(set);
        Debug(set);
    } break;

    default:
        break;
    };

    cJSON_Delete(root);
}

///////////////////////////////////////////////////////////////////////////////
bool DataWebRadio::CreateMessageResponse(MessageType_e msg, char* buffer, size_t size)
{
    bool bSendResponse = false;

    cJSON* root = cJSON_CreateObject();
    cJSON* json;

    switch (msg) {
    case DataWebRadio::FindBoard: {
        cJSON* alive;
        Settings_t settings;
        GetSettings(settings);

        cJSON_AddItemToObject(root, LYRAT_NET_WEBRADIO, alive = cJSON_CreateObject());
        cJSON_AddItemToObject(alive, LYRAT_NET_BOARDALIVE, json = cJSON_CreateObject());
        cJSON_AddStringToObject(json, LYRAT_NET_NAME, settings.mRadioName.c_str());
        cJSON_AddStringToObject(json, "id", mWebRadio->GetWifiWebRadio().getId().c_str());
        cJSON_AddStringToObject(json, LYRAT_NET_IP, mWebRadio->GetWifiWebRadio().getIp().c_str());
        bSendResponse = true;
    } break;

    case DataWebRadio::Configuration: {
        cJSON* stations;
        cJSON* station;
        cJSON* credentials;
        cJSON* bluetooth;
        Settings_t set;
        GetSettings(set);

        cJSON_AddItemToObject(root, LYRAT_NET_WEBRADIO, json = cJSON_CreateObject());

        cJSON_AddItemToObject(json, LYRAT_NET_CREDENTIALS, credentials = cJSON_CreateObject());
        cJSON_AddStringToObject(credentials, LYRAT_NET_RADIOSSID, set.mCredentials.mSSID.c_str());
        cJSON_AddStringToObject(credentials, LYRAT_NET_RADIOPASSWD, "****");

        cJSON_AddItemToObject(json, LYRAT_NET_BLUETOOTH, bluetooth = cJSON_CreateObject());
        cJSON_AddNumberToObject(bluetooth, LYRAT_NET_BT_ENABLED, set.mBluetooth.mbEnabled);
        cJSON_AddStringToObject(bluetooth, LYRAT_NET_BT_PAIR, set.mBluetooth.mPair.c_str());

        cJSON_AddItemToObject(json, LYRAT_NET_STATIONLIST, stations = cJSON_CreateArray());
        for (auto st : set.mStations) {
            cJSON_AddItemToArray(stations, station = cJSON_CreateObject());
            cJSON_AddStringToObject(station, LYRAT_NET_ST_ID, st.mId.c_str());
            cJSON_AddStringToObject(station, LYRAT_NET_ST_URL, st.mUrl.c_str());
            cJSON_AddStringToObject(station, LYRAT_NET_ST_DECODER, st.mDecoder.c_str());
        }
        cJSON_AddItemToObject(json, LYRAT_NET_ACTTUNE, station = cJSON_CreateObject());
        cJSON_AddStringToObject(station, LYRAT_NET_ST_ID, set.mActTune.mId.c_str());
        cJSON_AddStringToObject(station, LYRAT_NET_ST_URL, set.mActTune.mUrl.c_str());
        cJSON_AddStringToObject(station, LYRAT_NET_ST_DECODER, set.mActTune.mDecoder.c_str());

        cJSON_AddStringToObject(json, LYRAT_NET_RADIO, set.mRadioName.c_str());
        cJSON_AddNumberToObject(json, LYRAT_NET_VOLUME, set.mVolume);
        cJSON_AddNumberToObject(json, LYRAT_NET_ACTSTATION, set.mActStation);

        bSendResponse = true;
    } break;

    case DataWebRadio::PlayIds: {
        cJSON* playids;
        cJSON* stations;
        cJSON* station;
        Settings_t set;
        GetSettings(set);

        cJSON_AddItemToObject(root, LYRAT_NET_WEBRADIO, playids = cJSON_CreateObject());
        cJSON_AddItemToObject(playids, LYRAT_NET_PLAYIDS, stations = cJSON_CreateArray());

        std::vector<CheckListEntry_t> entryList;

        GetCheckedStations(entryList);
        for (std::size_t i = 0; i < entryList.size(); ++i) {
            cJSON_AddItemToArray(stations, station = cJSON_CreateObject());
            cJSON_AddStringToObject(station, LYRAT_NET_ST_ID, entryList[i].mId.c_str());
            cJSON_AddStringToObject(station, LYRAT_NET_CHECK, entryList[i].ResultString());
        }
        bSendResponse = true;
    } break;

    default:
        break;
    };

    if (bSendResponse) {
        bSendResponse = cJSON_PrintPreallocated(root, buffer, size, 0);
    }
    cJSON_Delete(root);

    return bSendResponse;
}

///////////////////////////////////////////////////////////////////////////////
DataWebRadio::MessageType_e DataWebRadio::IsWebRadioRequest(char* pRequest)
{
    MessageType_e reqType = NoWebRadioRequest;

    cJSON* root = cJSON_Parse(pRequest);
    if (root != NULL) {
        cJSON* webradio = cJSON_GetObjectItem(root, LYRAT_NET_WEBRADIO);
        if (webradio != NULL) {
            if (cJSON_HasObjectItem(webradio, LYRAT_NET_FINDBOARD)) {
                reqType = FindBoard;
            }
            else if (cJSON_HasObjectItem(webradio, LYRAT_NET_CONFIGURATION)) {
                reqType = Configuration;
            }
            else if (cJSON_HasObjectItem(webradio, LYRAT_NET_PLAYIDS)) {
                reqType = PlayIds;
            }
        }
        cJSON_Delete(root);
    }

    return reqType;
}
