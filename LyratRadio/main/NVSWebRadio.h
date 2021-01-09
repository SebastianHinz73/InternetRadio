/****************************************************************************************
  WebRadio - Internet radio using Espressif's Lyrat board
  Written by Sebastian Hinz, http://radio-online.eu/
  Based on: Espressif ESP-ADF build environment and examples

  This code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
****************************************************************************************/

#ifndef _NVSWEBRADIO_H_
#define _NVSWEBRADIO_H_

#include "data_json_interface.h"

// max 15 characters for id's        123456789012345
#define LYRAT_NVS_RESTART_NOROUTER "norouter"
#define LYRAT_NVS_MAXSTATION "max"
#define LYRAT_NVS_CHECKLIST "checklist"

#define DEFAULT_STATION0 	"9605ae29-0601-11e8-ae97-52543be04c81", "http://stream.lohro.de:8000/lohro", "MP3"
#define DEFAULT_STATION1 	"960c5b08-0601-11e8-ae97-52543be04c81", "http://swr-swr1-bw.cast.addradio.de/swr/swr1/bw/mp3/128/stream.mp3", "MP3"

//////////////////////////////////////////////////////////////////////
enum CheckListResult {
    Undefined,
    Invalid,
    Valid
};

//////////////////////////////////////////////////////////////////////
typedef struct
    {
    std::string mId;
    CheckListResult mResult;

    const char* ResultString()
    {
        switch (mResult) {
        case CheckListResult::Undefined:
            return "Undefined";
        case CheckListResult::Invalid:
            return "Invalid";
        case CheckListResult::Valid:
            return "Valid";
        default:
            return "Unknown";
        }
    }
} CheckListEntry_t;

//////////////////////////////////////////////////////////////////////
class NVSWebRadio {
public:
    NVSWebRadio();

    esp_err_t Initialize();
    void ResetRestartNoRooter(); // reset no-network counter
    int GetRestartCount(); //

    void SetCredentials(Credentials_t& cr);
    void SetBluetooth(Bluetooth_t& bt);
    void SetStation(int index, Station_t& st, int maxStation = 0);
    void SetName(std::string& name);
    void SetVolume(int volume);
    void SetActStation(int actStation);

    void IncStationCheck(Station_t& station, CheckListResult result); // increment reset counter, set station status
    void GetCheckedStations(std::vector<CheckListEntry_t>& retList);

    bool GetStation(int i, Station_t& station);
    void GetSettings(Settings_t& settings);
    void GetCredentials(Credentials_t& credentials);
    bool EmptyCredentials();

    // functions
private:
    void IncRestartNoRooter(); // increase variable to detect unreachable network
    void AddCheckedStation(Station_t& st, CheckListResult result);

private:
    bool ExistsValue(const char* pKey);
    int GetValue(const char* pKey);
    void SetValue(const char* pKey, int i32Value);
    bool GetValue(const char* pKey, std::string& retValue);
    void SetValue(const char* pKey, std::string& Value);

    // variables
private:
    static char mStaticBuffer[512];
    nvs_handle mMyHandle;
};

////////////////////////////////////////////////////////////////////////////////

#endif
