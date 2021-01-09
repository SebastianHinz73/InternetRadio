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
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "NVSWebRadio.h"

extern const char* TAG;
char NVSWebRadio::mStaticBuffer[512];

///////////////////////////////////////////////////////////////////////////////
NVSWebRadio::NVSWebRadio()
{
}

///////////////////////////////////////////////////////////////////////////////
esp_err_t NVSWebRadio::Initialize()
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open
    ESP_LOGI(TAG, "[ NVS ] Opening Non-Volatile Storage (NVS) handle... ");
    err = nvs_open("storage", NVS_READWRITE, &mMyHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[ NVS ] Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else {
        // increase reset counter
        IncRestartNoRooter();

        if (GetRestartCount() > 3) {
            ESP_LOGI(TAG, "[ NVS ] Reset Router ssid and password... ");
            Credentials_t cr;
            SetCredentials(cr);
        }

        if (!ExistsValue(LYRAT_NET_ACTSTATION)) {
            ESP_LOGI(TAG, "[ NVS ] Set default values... ");

            Settings_t set;

            Station_t station0(DEFAULT_STATION0);
            Station_t station1(DEFAULT_STATION1);
            set.mStations.push_back(station0);
            set.mStations.push_back(station1);
            set.mActTune = station0;

            SetCredentials(set.mCredentials);
            SetBluetooth(set.mBluetooth);

            for (int i = 0; i < set.mStations.size(); i++) {
                SetStation(i, set.mStations[i], set.mStations.size());
            }
            SetStation(-1, set.mActTune, 2);
            std::string strName = "Lyrat Web Radio";
            SetName(strName);
            SetVolume(50);
            SetActStation(-1);

            std::string empty = "";
            SetValue(LYRAT_NVS_CHECKLIST, empty);
			AddCheckedStation(station0, CheckListResult::Valid);
			AddCheckedStation(station1, CheckListResult::Valid);
        }
    }

    return err;
}

///////////////////////////////////////////////////////////////////////////////
// increase variable to detect unreachable network
void NVSWebRadio::IncRestartNoRooter()
{
    SetValue(LYRAT_NVS_RESTART_NOROUTER, GetRestartCount() + 1);
}

///////////////////////////////////////////////////////////////////////////////
// return restart count
int NVSWebRadio::GetRestartCount()
{
    return ExistsValue(LYRAT_NVS_RESTART_NOROUTER) ? GetValue(LYRAT_NVS_RESTART_NOROUTER) : -1;
}

///////////////////////////////////////////////////////////////////////////////
// reset no-network counter
void NVSWebRadio::ResetRestartNoRooter()
{
    SetValue(LYRAT_NVS_RESTART_NOROUTER, 0);
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::SetCredentials(Credentials_t& cr)
{
    esp_err_t err = nvs_set_str(mMyHandle, LYRAT_NET_RADIOSSID, cr.mSSID.c_str());
    ESP_ERROR_CHECK(err);

    err = nvs_set_str(mMyHandle, LYRAT_NET_RADIOPASSWD, cr.mPassword.c_str());
    ESP_ERROR_CHECK(err);

    // Commit written value.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);

    ResetRestartNoRooter();
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::SetBluetooth(Bluetooth_t& bt)
{
    esp_err_t err = nvs_set_i32(mMyHandle, LYRAT_NET_BT_ENABLED, bt.mbEnabled);
    ESP_ERROR_CHECK(err);

    err = nvs_set_str(mMyHandle, LYRAT_NET_BT_PAIR, bt.mPair.c_str());
    ESP_ERROR_CHECK(err);

    // Commit written value.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::SetStation(int index, Station_t& st, int maxStation)
{
    char key[32];

    sprintf(key, "%s%d", LYRAT_NET_ST_ID, index);
    esp_err_t err = nvs_set_str(mMyHandle, key, st.mId.c_str());
    ESP_ERROR_CHECK(err);

    sprintf(key, "%s%d", LYRAT_NET_ST_URL, index);
    err = nvs_set_str(mMyHandle, key, st.mUrl.c_str());
    ESP_ERROR_CHECK(err);

    sprintf(key, "%s%d", LYRAT_NET_ST_DECODER, index);
    err = nvs_set_str(mMyHandle, key, st.mDecoder.c_str());
    ESP_ERROR_CHECK(err);

    //sprintf(key, "%s%d", LYRAT_NVS_STATION_PLAY_OK, index);
    //err = nvs_set_i32(mMyHandle, key, 0);
    //ESP_ERROR_CHECK(err);

    if (index >= 0) {
        err = nvs_set_i32(mMyHandle, LYRAT_NVS_MAXSTATION, maxStation);
        ESP_ERROR_CHECK(err);
    }

    // Commit written value.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::SetName(std::string& name)
{
    esp_err_t err = nvs_set_str(mMyHandle, LYRAT_NET_RADIO, name.c_str());
    ESP_ERROR_CHECK(err);

    // Commit written value.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::SetVolume(int volume)
{
    esp_err_t err = nvs_set_i32(mMyHandle, LYRAT_NET_VOLUME, volume);
    ESP_ERROR_CHECK(err);

    // Commit written value.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::SetActStation(int actStation)
{
    esp_err_t err = nvs_set_i32(mMyHandle, LYRAT_NET_ACTSTATION, actStation);
    ESP_ERROR_CHECK(err);

    // Commit written value.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::IncStationCheck(Station_t& station, CheckListResult result)
{
    int actCnt = 0;
    AddCheckedStation(station, result);

    switch (result) {
    case CheckListResult::Undefined:
    case CheckListResult::Invalid:
        actCnt = GetValue(LYRAT_NET_CHECK);
        if (actCnt > 2) {
            printf("Reset actual station... \n");
            AddCheckedStation(station, CheckListResult::Invalid);

            Station_t station0(DEFAULT_STATION0);
            SetStation(-1, station0, 0);
        }
        break;
    case CheckListResult::Valid:
        break;
    };

    ESP_LOGI(TAG, "[ NVS ] Reset count = %d\n", actCnt);

    esp_err_t err = nvs_set_i32(mMyHandle, LYRAT_NET_CHECK, actCnt + 1);
    ESP_ERROR_CHECK(err);

    // Commit written value.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::AddCheckedStation(Station_t& st, CheckListResult result)
{
    bool bFound = false;
    std::vector<CheckListEntry_t> stationList;
    GetCheckedStations(stationList);

    //printf("## LYRAT_NVS_CHECKLIST, %d\n", stationList.size());

    // station already in list?
    for (std::size_t i = 0; i < stationList.size(); ++i) {
        if (stationList[i].mId == st.mId) {
            stationList[i].mId = st.mId;
            // Undefined (0), Invalid (1), Valid(2) --> write only bigger values
            if ((int)stationList[i].mResult < (int)result) {
                stationList[i].mResult = result;
            }
            bFound = true;
            break;
        }
    }

    if (!bFound) {
        // remove last entry if not fit, // e.g. '78012206-1aa1-11e9-a80b-52543be04c81,2;'
        if ((stationList.size() + 1) * 40 >= sizeof(mStaticBuffer)) {
            stationList.pop_back();
        }
        stationList.insert(stationList.begin(), CheckListEntry_t());

        stationList[0].mId = st.mId;
        stationList[0].mResult = result;
    }

    std::string write;
    for (std::size_t i = 0; i < stationList.size(); ++i) {
        char chResult[2];
        sprintf(chResult, "%d", stationList[i].mResult);
        write += stationList[i].mId + "," + chResult + ";";
        //printf("# i %d %s\n", i, chResult);
    }
    SetValue(LYRAT_NVS_CHECKLIST, write);
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::GetCheckedStations(std::vector<CheckListEntry_t>& retList)
{
    retList.clear();

    std::string stations;

    if (GetValue(LYRAT_NVS_CHECKLIST, stations)) {
        std::string::size_type prev_pos = 0, pos = 0, pos2 = 0;

        while ((pos = stations.find(';', pos)) != std::string::npos) {
            std::string entry(stations.substr(prev_pos, pos - prev_pos));

            if ((pos2 = entry.find(',')) != std::string::npos) {
                std::string id = entry.substr(0, pos2);
                std::string resString = entry.substr(pos2 + 1);
                CheckListResult res = CheckListResult::Undefined;
                if (resString == "1") {
                    res = CheckListResult::Invalid;
                }
                else if (resString == "2") {
                    res = CheckListResult::Valid;
                }

                retList.push_back(CheckListEntry_t());
                retList[retList.size() - 1].mId = id;
                retList[retList.size() - 1].mResult = res;
            }

            prev_pos = ++pos;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::GetSettings(Settings_t& set)
{
    // credentials
    GetValue(LYRAT_NET_RADIOSSID, set.mCredentials.mSSID);
    GetValue(LYRAT_NET_RADIOPASSWD, set.mCredentials.mPassword);

    // bluetooth
    set.mBluetooth.mbEnabled = GetValue(LYRAT_NET_BT_ENABLED);
    GetValue(LYRAT_NET_BT_PAIR, set.mBluetooth.mPair);

    int i = -1;
    Station_t station;

    while (GetStation(i, station)) {
        if (i == -1) {
            set.mActTune = station;
        }
        else {
            set.mStations.push_back(station);
        }
        i++;
    }

    GetValue(LYRAT_NET_RADIO, set.mRadioName);
    set.mVolume = GetValue(LYRAT_NET_VOLUME);
    set.mActStation = GetValue(LYRAT_NET_ACTSTATION);
}

///////////////////////////////////////////////////////////////////////////////
bool NVSWebRadio::GetStation(int i, Station_t& station)
{
    char key[32];
    sprintf(key, "%s%d", LYRAT_NET_ST_ID, i);
    bool bRc = ExistsValue(key);
    bRc &= i < GetValue(LYRAT_NVS_MAXSTATION);

    if (bRc) {
        GetValue(key, station.mId);
        sprintf(key, "%s%d", LYRAT_NET_ST_URL, i);
        GetValue(key, station.mUrl);

        sprintf(key, "%s%d", LYRAT_NET_ST_DECODER, i);
        GetValue(key, station.mDecoder);
    }

    return bRc;
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::GetCredentials(Credentials_t& credentials)
{
    // credentials
    GetValue(LYRAT_NET_RADIOSSID, credentials.mSSID);
    GetValue(LYRAT_NET_RADIOPASSWD, credentials.mPassword);
}

///////////////////////////////////////////////////////////////////////////////
bool NVSWebRadio::EmptyCredentials()
{
    Credentials_t credentials;
    GetCredentials(credentials);
    return credentials.mSSID.length() == 0;
}

///////////////////////////////////////////////////////////////////////////////
bool NVSWebRadio::ExistsValue(const char* pKey)
{
    int32_t defValue = 0;
    esp_err_t err = nvs_get_i32(mMyHandle, pKey, &defValue);

    if (err != ESP_OK) {
        size_t length = sizeof(mStaticBuffer);
        err = nvs_get_str(mMyHandle, pKey, mStaticBuffer, &length);
    }

    switch (err) {
    case ESP_OK:
        return true;
    case ESP_ERR_NVS_NOT_FOUND:
        return false;
    default:
        printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
int NVSWebRadio::GetValue(const char* pKey)
{
    int32_t value = 0;
    esp_err_t err = nvs_get_i32(mMyHandle, pKey, &value);

    switch (err) {
    case ESP_OK:
    case ESP_ERR_NVS_NOT_FOUND:
        break;
    default:
        ESP_ERROR_CHECK(err);
        break;
    }
    return value;
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::SetValue(const char* pKey, int i32Value)
{
    esp_err_t err = nvs_set_i32(mMyHandle, pKey, i32Value);
    ESP_ERROR_CHECK(err);

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);
}

///////////////////////////////////////////////////////////////////////////////
bool NVSWebRadio::GetValue(const char* pKey, std::string& retValue)
{
    size_t length = sizeof(mStaticBuffer);
    esp_err_t err = nvs_get_str(mMyHandle, pKey, mStaticBuffer, &length);

    switch (err) {
    case ESP_OK:
        retValue = mStaticBuffer;
        return true;
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        printf("The value is not initialized yet! %s\n", pKey);
        retValue = "";
        break;
    default:
        ESP_ERROR_CHECK(err);
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
void NVSWebRadio::SetValue(const char* pKey, std::string& Value)
{
    esp_err_t err = nvs_set_str(mMyHandle, pKey, Value.c_str());
    ESP_ERROR_CHECK(err);

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(mMyHandle);
    ESP_ERROR_CHECK(err);
}
