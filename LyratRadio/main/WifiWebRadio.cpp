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
#include "lwip/sockets.h"

#include "esp_http_client.h"
#include "periph_wifi.h"

#include "WebRadio.h"
#include "DataWebRadio.h"
#include "WifiWebRadio.h"

#define AP_MAX_STA_CONN 4
#define UDP_PORT 44948

//EventGroupHandle_t WifiWebRadio::s_wifi_event_group;
WebRadio* WifiWebRadio::mWebRadio = 0;

///////////////////////////////////////////////////////////////////////////////
WifiWebRadio::WifiWebRadio()
    : mId("")
    , mIp("")
{
    char buf[16];

    // get and unique id (part of mac address)
    esp_efuse_mac_get_default((uint8_t*)buf);
    snprintf(buf, sizeof(buf), "Lyrat%X%X", buf[4], buf[5]);
    mId = buf;
}

///////////////////////////////////////////////////////////////////////////////
esp_err_t WifiWebRadio::StartClient(WebRadio* webRadio, esp_periph_set_handle_t& set)
{
    mWebRadio = webRadio;

    char buf[16];
    DataWebRadio& data = webRadio->GetDataWebRadio();
    Credentials_t credentials;
    data.GetCredentials(credentials);

    tcpip_adapter_init();

    periph_wifi_cfg_t wifi_cfg;

    wifi_cfg.ssid = credentials.mSSID.c_str();
    wifi_cfg.password = credentials.mPassword.c_str();
    wifi_cfg.reconnect_timeout_ms = 1000;
    wifi_cfg.wpa2_e_cfg.diasble_wpa2_e = false;

    ESP_LOGI(TAG, "[ WIFI ] Connect to ...");

    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    // reset restart counter
    data.ResetRestartNoRooter();

    ESP_LOGI(TAG, "[ WIFI ] Connect to ap SSID:%s", credentials.mSSID.c_str());

    xTaskCreate(udp_server_task, "udp_server", 2 * 4096, NULL, 5, NULL);

    // get and set own ip
    tcpip_adapter_ip_info_t sta_ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &sta_ip);
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", IP2STR(&sta_ip.ip));
    mIp = buf;
    ESP_LOGI(TAG, "[ WIFI ] Got IP:%s", mIp.c_str());

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////
esp_err_t WifiWebRadio::StartAccessPoint(WebRadio* webRadio, esp_periph_set_handle_t& set)
{
    const char* AP_WIFI_SSID = "ESP_Webradio";
    const char* AP_WIFI_PASS = "";

    mWebRadio = webRadio;
    wifi_init_softap();

    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);

    // get and set own ip
    tcpip_adapter_ip_info_t sta_ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &sta_ip);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", IP2STR(&sta_ip.ip));
    mIp = buf;

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////
void WifiWebRadio::udp_server_task(void* pvParameters)
{
    char buffer[1024];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    DataWebRadio& data = mWebRadio->GetDataWebRadio();

    while (1) {

        //#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(UDP_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
        /*#else // IPV6
        struct sockaddr_in6 destAddr;
        bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
        destAddr.sin6_family = AF_INET6;
        destAddr.sin6_port = htons(UDP_PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif
*/
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "[ UDP ] Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "[ UDP ] Socket created");

        int err = bind(sock, (struct sockaddr*)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG, "[ UDP ] Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "[ UDP ] Socket binded");

        while (1) {

            ESP_LOGI(TAG, "[ UDP ] Waiting for data udp");
            struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "[ UDP ] recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                if (sourceAddr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in*)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }
                else if (sourceAddr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }
                buffer[len] = 0; // Null-terminate whatever we received and treat like a string...

                DataWebRadio::MessageType_e msgType = data.IsWebRadioRequest(buffer);

                if (msgType != DataWebRadio::NoWebRadioRequest) {
                    ESP_LOGI(TAG, "[ UDP ] Received udp  %d bytes from %s:", len, addr_str);
                    ESP_LOGI(TAG, "[ UDP ] rx: %s", buffer);

                    data.HandleMessage(msgType, buffer, sizeof(buffer));

                    ////////////////////////////////////
                    if (data.CreateMessageResponse(msgType, buffer, sizeof(buffer))) {
                        ESP_LOGI(TAG, "[ UDP ] tx: %s", buffer);

                        len = strlen(buffer);
                        int err = sendto(sock, buffer, len, 0, (struct sockaddr*)&sourceAddr, sizeof(sourceAddr));
                        if (err < 0) {
                            ESP_LOGE(TAG, "[ UDP ] Error occured during sending: errno %d", errno);
                            break;
                        }
                    }
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "[ UDP ] Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}
