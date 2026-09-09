#include "options.h"
#include <ESPmDNS.h>
#include <esp_netif.h>
#include "time.h"
#include "rtcsupport.h"
#include "network.h"
#include "display.h"
#include "config.h"
#include "telnet.h"
#include "netserver.h"
#include "player.h"
#include "mqtt.h"
#include "timekeeper.h"
#include "../pluginsManager/pluginsManager.h"

#ifndef WIFI_ATTEMPTS
  #define WIFI_ATTEMPTS  16
#endif

#ifndef SEARCH_WIFI_CORE_ID
  #define SEARCH_WIFI_CORE_ID  0
#endif
MyNetwork network;

void MyNetwork::WiFiReconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  network.beginReconnect = false;
  player.lockOutput = false;
  delay(100);
  display.putRequest(NEWMODE, PLAYER);
  if(config.getMode()==PM_SDCARD) {
    network.status=CONNECTED;
    display.putRequest(NEWIP, 0);
  }else{
    display.putRequest(NEWMODE, PLAYER);
    if (network.lostPlaying) player.sendCommand({PR_PLAY, config.lastStation()});
  }
  #ifdef MQTT_ROOT_TOPIC
    connectToMqtt();
  #endif
}

void MyNetwork::WiFiLostConnection(WiFiEvent_t event, WiFiEventInfo_t info){
  if(!network.beginReconnect){
    Serial.printf("Lost connection, reconnecting to %s...\n", config.ssids[config.store.lastSSID-1].ssid);
    if(config.getMode()==PM_SDCARD) {
      network.status=SDREADY;
      display.putRequest(NEWIP, 0);
    }else{
      network.lostPlaying = player.isRunning();
      if (network.lostPlaying) { player.lockOutput = true; player.sendCommand({PR_STOP, 0}); }
      display.putRequest(NEWMODE, LOST);
    }
  }
  network.beginReconnect = true;
  WiFi.reconnect();
}

bool MyNetwork::wifiBegin(bool silent){
  uint8_t ls = (config.store.lastSSID == 0 || config.store.lastSSID > config.ssidsCount) ? 0 : config.store.lastSSID - 1;
  uint8_t startedls = ls;
  uint8_t errcnt = 0;
  //WiFi.mode(WIFI_STA);
  while (true) {
    if(!silent){
      Serial.printf("##[BOOT]#\tAttempt to connect to %s\n", config.ssids[ls].ssid);
      Serial.print("##[BOOT]#\t");
      display.putRequest(BOOTSTRING, ls);
    }
    //WiFi.disconnect(true, true); //disconnect & erase internal credentials https://github.com/e2002/yoradio/pull/164/commits/89d8b4450dde99cd7930b84bb14d81dab920b879
    //delay(100);
    WiFi.mode(WIFI_STA);
#ifdef WIFI_SELECT_STRONGEST_BSSID
    int32_t strongestRssi = -127;
    int32_t strongestChannel = 0;
    uint8_t strongestBssid[6] = {0};
    const int scanCount = WiFi.scanNetworks(false, true);
    for (int i = 0; i < scanCount; ++i) {
      if (WiFi.SSID(i) == config.ssids[ls].ssid && WiFi.RSSI(i) > strongestRssi) {
        strongestRssi = WiFi.RSSI(i);
        strongestChannel = WiFi.channel(i);
        memcpy(strongestBssid, WiFi.BSSID(i), sizeof(strongestBssid));
      }
    }
    WiFi.scanDelete();
    if (strongestChannel > 0) {
      Serial.printf("##[BOOT]#\tStrongest AP for %s: channel %d, RSSI %d dBm\n",
                    config.ssids[ls].ssid, strongestChannel, strongestRssi);
      WiFi.begin(config.ssids[ls].ssid, config.ssids[ls].password,
                 strongestChannel, strongestBssid, true);
    } else {
      WiFi.begin(config.ssids[ls].ssid, config.ssids[ls].password);
    }
#else
    WiFi.begin(config.ssids[ls].ssid, config.ssids[ls].password);
#endif
    while (WiFi.status() != WL_CONNECTED) {
      if(!silent) Serial.print(".");
      delay(500);
      if(REAL_LEDBUILTIN!=255 && !silent) digitalWrite(REAL_LEDBUILTIN, !digitalRead(REAL_LEDBUILTIN));
      errcnt++;
      if (errcnt > WIFI_ATTEMPTS) {
        errcnt = 0;
        ls++;
        if (ls > config.ssidsCount - 1) ls = 0;
        if(!silent) Serial.println();
        WiFi.mode(WIFI_OFF);
        break;
      }
    }
    if (WiFi.status() != WL_CONNECTED && ls == startedls) {
      return false; break;
    }
    if (WiFi.status() == WL_CONNECTED) {
      config.setLastSSID(ls + 1);
      return true; break;
    }
  }
  return false;
}

void searchWiFi(void * pvParameters){
  if(!network.wifiBegin(true)){
    delay(10000);
    xTaskCreatePinnedToCore(searchWiFi, "searchWiFi", 1024 * 4, NULL, 0, NULL, SEARCH_WIFI_CORE_ID);
  }else{
    network.status = CONNECTED;
    netserver.begin(true);
    telnet.begin(true);
    network.setWifiParams();
    display.putRequest(NEWIP, 0);
    #ifdef MQTT_ROOT_TOPIC
      mqttInit();
    #endif
  }
  vTaskDelete( NULL );
}

#define DBGAP false

void MyNetwork::begin() {
  BOOTLOG("network.begin");
  config.initNetwork();
  if (config.ssidsCount == 0 || DBGAP) {
    raiseSoftAP();
    return;
  }
  if(config.getMode()!=PM_SDCARD){
    if(!wifiBegin()){
      raiseSoftAP();
      Serial.println("##[BOOT]#\tdone");
      return;
    }
    Serial.println(".");
    status = CONNECTED;
    setWifiParams();
    #ifdef MQTT_ROOT_TOPIC
      mqttInit();
    #endif
  }else{
    status = SDREADY;
    xTaskCreatePinnedToCore(searchWiFi, "searchWiFi", 1024 * 4, NULL, 0, NULL, SEARCH_WIFI_CORE_ID);
  }
  
  Serial.println("##[BOOT]#\tdone");
  if(REAL_LEDBUILTIN!=255) digitalWrite(REAL_LEDBUILTIN, LOW);
  
#if RTCSUPPORTED
  if(config.isRTCFound()){
    rtc.getTime(&network.timeinfo);
    mktime(&network.timeinfo);
    display.putRequest(CLOCK);
  }
#endif
  if (network_on_connect) network_on_connect();
  pm.on_connect();
}

void MyNetwork::setWifiParams(){
  WiFi.setSleep(false);
  WiFi.onEvent(WiFiReconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiLostConnection, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  //config.setTimeConf(); //??
  if(strlen(config.store.mdnsname)>0)
    MDNS.begin(config.store.mdnsname);
}

void MyNetwork::requestTimeSync(bool withTelnetOutput, uint8_t clientId) {
  if (withTelnetOutput) {
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    if (config.store.tzHour < 0) {
      telnet.printf(clientId, "##SYS.DATE#: %s%03d:%02d\n> ", timeStringBuff, config.store.tzHour, config.store.tzMin);
    } else {
      telnet.printf(clientId, "##SYS.DATE#: %s+%02d:%02d\n> ", timeStringBuff, config.store.tzHour, config.store.tzMin);
    }
  }
}

void rebootTime() {
  ESP.restart();
}

void MyNetwork::raiseSoftAP() {
  WiFi.mode(WIFI_AP);
  const IPAddress captiveIp(192, 168, 4, 1);
  const IPAddress captiveMask(255, 255, 255, 0);
  WiFi.softAP(apSsid, apPassword);
  WiFi.softAPConfig(captiveIp, captiveIp, captiveMask);

  // iOS bekommt den DNS-Server per DHCP. Nur ein laufender Wildcard-DNS reicht
  // fuer die automatische Captive-Portal-Erkennung nicht auf jedem Geraet aus.
  esp_netif_t *apNetif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (apNetif != nullptr) {
    esp_err_t stopResult = esp_netif_dhcps_stop(apNetif);
    if (stopResult == ESP_OK || stopResult == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
      esp_netif_dns_info_t dnsInfo = {};
      dnsInfo.ip.type = ESP_IPADDR_TYPE_V4;
      dnsInfo.ip.u_addr.ip4.addr = static_cast<uint32_t>(captiveIp);
      // ESP-IDF 4.4: DHCP-Offer-Bit 0x02 aktiviert Option 6 (DNS-Server).
      uint8_t dnsOffer = 0x02;
      esp_err_t dnsResult = esp_netif_set_dns_info(apNetif, ESP_NETIF_DNS_MAIN, &dnsInfo);
      esp_err_t offerResult = esp_netif_dhcps_option(
        apNetif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER,
        &dnsOffer, sizeof(dnsOffer)
      );
      esp_err_t startResult = esp_netif_dhcps_start(apNetif);
      Serial.printf("##[BOOT]#\tCaptive DNS DHCP: dns=%d offer=%d start=%d\n",
                    dnsResult, offerResult, startResult);
    } else {
      Serial.printf("##[BOOT]#\tCaptive DNS DHCP stop failed: %d\n", stopResult);
    }
  }
  Serial.println("##[BOOT]#");
  BOOTLOG("************************************************");
  BOOTLOG("Running in AP mode");
  BOOTLOG("Connect to AP %s", apSsid);
  BOOTLOG("and go to http://192.168.4.1/ to configure");
  BOOTLOG("************************************************");
  status = SOFT_AP;
  if(config.store.softapdelay>0)
    timekeeper.waitAndDo(config.store.softapdelay*60, rebootTime);
}

void MyNetwork::requestWeatherSync(){
  display.putRequest(NEWWEATHER);
}
