#ifndef __WIFI_H__
#define __WIFI_H__

#include "ISM43362Interface.h"
#include "NTPClient.h"
#include "config.h"
#include "logger.h"
#include "logger_netsocket.h"
#include "mbed.h"

#if defined LOG_LEVEL_TRACE
#define MBED_CONF_WIFI_DEBUG 1
#else
#define MBED_CONF_WIFI_DEBUG 0
#endif

struct WIFIConfig {
  const char* wifi_ssid;
  const char* wifi_password;
};

class WIFI {
  typedef WIFI Self;

 public:
  WIFI(const char* ssid = MBED_CONF_APP_WIFI_SSID,
       const char* password = MBED_CONF_APP_WIFI_PASSWORD)
      : _ssid(ssid),
        _password(password),
        _wifi(new ISM43362Interface(MBED_CONF_WIFI_DEBUG)) {}

  ~WIFI() { delete _wifi; }

  int connect() {
    nsapi_error_t ns_ret;

    // connect wifi
    ns_ret = _wifi->connect(_ssid, _password, NSAPI_SECURITY_WPA_WPA2);
    if (ns_ret != 0) {
      ns_log_errorln(ns_ret, "failed to connect to wifi ap");
      return -1;
    }
    log_infoln(
        "connected to wifi ap\n  mac: %s\n  ip: %s\n  netmask: %s\n  gateway: "
        "%s\n  rssi: %d",
        _wifi->get_mac_address(), _wifi->get_ip_address(), _wifi->get_netmask(),
        _wifi->get_gateway(), _wifi->get_rssi());

    // synchronize timestamp
    ns_ret = sync_timestamp();
    if (ns_ret) {
      return ns_ret;
    }

    return 0;
  }

  NetworkInterface* interface() { return (NetworkInterface*)_wifi; }

 private:
  int sync_timestamp() {
    log_debugln("try to sync timestamp");

    NTPClient ntp_client(Self::interface());

    time_t ts = ntp_client.get_timestamp();
    if (ts < 0) {
      if (ts < -2) {
        ns_log_errorln(ts, "failed to sync timestamp");
      } else {
        log_errorln("failed to sync timestamp");
      }

      return ts;
    }

    // set RTC time to global time
    set_time(ts);

    // info to show current time
    ts = time(NULL);
    struct tm* ts_tm = localtime(&ts);
    char ts_str[32];
    strftime(ts_str, 32, "%Y/%m/%d %T", ts_tm);
    log_infoln("current time is %s", ts_str);

    return 0;
  }

 private:
  const char* _ssid;
  const char* _password;
  ISM43362Interface* _wifi;
};

#endif
