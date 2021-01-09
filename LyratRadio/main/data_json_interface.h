/****************************************************************************************
  WebRadio - Internet radio using Espressif's Lyrat board
  Written by Sebastian Hinz, http://radio-online.eu/
  Based on: Espressif ESP-ADF build environment and examples

  This code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
****************************************************************************************/

#ifndef DATA_JSON_INTERFACE_H
#define DATA_JSON_INTERFACE_H

#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define LYRAT_NET_WEBRADIO "webradio"
#define LYRAT_NET_FINDBOARD "find_board"
#define LYRAT_NET_BOARDALIVE "board_alive"
#define LYRAT_NET_NAME "name"
#define LYRAT_NET_IP "ip"

#define LYRAT_NET_CONFIGURATION "configuration"
#define LYRAT_NET_CREDENTIALS "credentials"
#define LYRAT_NET_RADIOSSID "radio_ssid"
#define LYRAT_NET_RADIOPASSWD "radio_passwd"
#define LYRAT_NET_BLUETOOTH "bluetooth"
#define LYRAT_NET_BT_ENABLED "bt_enabled"
#define LYRAT_NET_BT_PAIR "bt_pair"
#define LYRAT_NET_STATIONLIST "station_list"
#define LYRAT_NET_ST_ID "st_id"
#define LYRAT_NET_ST_URL "st_url"
#define LYRAT_NET_ST_DECODER "st_decoder"
#define LYRAT_NET_ACTTUNE "act_tune"
#define LYRAT_NET_RADIO "radio"
#define LYRAT_NET_VOLUME "volume"
#define LYRAT_NET_ACTSTATION "act"

#define LYRAT_NET_PLAYIDS "playids"
#define LYRAT_NET_CHECK "check"

///////////////////////////////////////////////////////////////////////////////
typedef struct Station {
    Station()
        : mId("")
        , mUrl("")
        , mDecoder("")
    {
    }
    Station(std::string id, std::string url, std::string decoder)
        : mId(id)
        , mUrl(url)
        , mDecoder(decoder)
    {
    }
    Station& operator=(const Station& src)
    {
        if (this != &src) {
            mId = src.mId;
            mUrl = src.mUrl;
            mDecoder = src.mDecoder;
        }
        return *this;
    }
    inline bool operator==(const Station& rhs) const
    {
        return (mId == rhs.mId) && (mUrl == rhs.mUrl) && (mDecoder == rhs.mDecoder);
    }
    inline bool operator!=(const Station& rhs) const { return !(*this == rhs); }
    std::string mId;
    std::string mUrl;
    std::string mDecoder;
} Station_t;

///////////////////////////////////////////////////////////////////////////////
typedef struct Credentials {
    Credentials()
        : mSSID("")
        , mPassword("")
    {
    }
    Credentials& operator=(const Credentials& src)
    {
        if (this != &src) {
            mSSID = src.mSSID;
            mPassword = src.mPassword;
        }
        return *this;
    }
    inline bool operator==(const Credentials& rhs) const
    {
        return (mSSID == rhs.mSSID) && (mPassword == rhs.mPassword);
    }
    inline bool operator!=(const Credentials& rhs) const { return !(*this == rhs); }
    std::string mSSID;
    std::string mPassword;
} Credentials_t;

///////////////////////////////////////////////////////////////////////////////
typedef struct Bluetooth {
    Bluetooth()
        : mbEnabled(false)
        , mPair("")
    {
    }
    Bluetooth& operator=(const Bluetooth& src)
    {
        if (this != &src) {
            mbEnabled = src.mbEnabled;
            mPair = src.mPair;
        }
        return *this;
    }
    inline bool operator==(const Bluetooth& rhs) const
    {
        return (mbEnabled == rhs.mbEnabled) && (mPair == rhs.mPair);
    }
    inline bool operator!=(const Bluetooth& rhs) const { return !(*this == rhs); }

    bool mbEnabled;
    std::string mPair;
} Bluetooth_t;

///////////////////////////////////////////////////////////////////////////////
typedef struct ActualSettings {
    ActualSettings()
        : mVolume(50)
        , mActStation(-1)
    {
    }
    ActualSettings& operator=(const ActualSettings& src)
    {
        if (this != &src) {
            mCredentials = src.mCredentials;
            mBluetooth = src.mBluetooth;
            for (size_t i = 0; i < sizeof(mStations) / sizeof(Station_t); i++) {
                mStations[i] = src.mStations[i];
            }
            mActTune = src.mActTune;
            mRadioName = src.mRadioName;
            mVolume = src.mVolume;
            mActStation = src.mActStation;
        }
        return *this;
    }
    inline bool operator==(const ActualSettings& rhs) const
    {
        bool bEqual = (mRadioName == rhs.mRadioName) && (mVolume == rhs.mVolume) && (mActStation == rhs.mActStation);
        bEqual &= mCredentials == rhs.mCredentials;
        bEqual &= mBluetooth == rhs.mBluetooth;
        for (size_t i = 0; i < mStations.size(); i++) {
            bEqual &= mStations[i] == rhs.mStations[i];
        }
        bEqual &= mActTune == rhs.mActTune;

        return bEqual;
    }
    inline bool operator!=(const ActualSettings& rhs) const { return !(*this == rhs); }

    Credentials_t mCredentials;
    Bluetooth_t mBluetooth;
    std::vector<Station_t> mStations;
    Station_t mActTune;
    std::string mRadioName;
    int mVolume;
    int mActStation;
} Settings_t;

#endif // DATA_JSON_INTERFACE_H
