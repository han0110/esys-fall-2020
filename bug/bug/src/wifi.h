#ifndef __WIFI_H__
#define __WIFI_H__

#include "ISM43362Interface.h"
#include "NTPClient.h"
#include "config.h"
#include "logger.h"
#include "logger_netsocket.h"
#include "mbed.h"
#include "util.h"

#if defined LOG_LEVEL_TRACE
#define MBED_CONF_WIFI_DEBUG 1
#else
#define MBED_CONF_WIFI_DEBUG 0
#endif

#define WIFI_CONNECT_RETRY_INTERVAL 5s
#define WIFI_RESET_RETRY_INTERVAL 100ms
#define DEVICE_ERROR_COUNT_THRETHOLD 3

#define ns_err_to_idx(err) (-3020 <= err && err <= -3001) ? (-err - 3001) : 20

class WIFI {
  typedef WIFI Self;

 public:
  WIFI(const char* ssid = MBED_CONF_APP_WIFI_SSID,
       const char* password = MBED_CONF_APP_WIFI_PASSWORD)
      : _event_queue(new events::EventQueue),
        _thread(new rtos::Thread),
        _error_count{0},
        _wifi(new ISM43362Interface(MBED_CONF_WIFI_DEBUG)),
        _connection_status(NSAPI_STATUS_DISCONNECTED),
        _ssid(ssid),
        _password(password),
        _ntp_synced(false) {
    _wifi->attach(mbed::callback(this, &Self::status_change_callback));
    _event_queue->call(this, &Self::connect_callback);
  }

  ~WIFI() {
    auto ns_ret = _wifi->disconnect();
    if (ns_ret != 0) {
      ns_log_errorln(ns_ret, "failed to disconnect to wifi ap");
    }

    delete _wifi;
  }

  NetworkInterface* interface() { return (NetworkInterface*)_wifi; }

  bool connected() {
    return _connection_status == NSAPI_STATUS_LOCAL_UP ||
           _connection_status == NSAPI_STATUS_GLOBAL_UP;
  }

  void report_error(nsapi_error_t ns_err) {
    log_debugln("report_error, ns_err: %d", ns_err);

    uint32_t count =
        core_util_atomic_incr_u32(&_error_count[ns_err_to_idx(ns_err)], 1);
    // assume wifi is broken, call connect_callback again
    if (ns_err == NSAPI_ERROR_DEVICE_ERROR &&
        count > DEVICE_ERROR_COUNT_THRETHOLD &&
        core_util_atomic_compare_exchange_weak_u32(
            &_error_count[ns_err_to_idx(ns_err)], &count,
            count - DEVICE_ERROR_COUNT_THRETHOLD)) {
      _event_queue->call(this, &Self::connect_callback);
    }
  }

 protected:
  void connect_callback() {
    log_debugln("connect_callback");

    auto ns_ret = connect();
    if (ns_ret < 0) {
      ns_log_errorln(ns_ret, "failed to connect, retry in %d",
                     WIFI_CONNECT_RETRY_INTERVAL);
      _event_queue->call_in(WIFI_CONNECT_RETRY_INTERVAL, this,
                            &Self::connect_callback);
    }
  }

  void start() {
    _thread->start(
        mbed::callback(_event_queue, &events::EventQueue::dispatch_forever));
  }

  void join() { _thread->join(); }

  void status_change_callback(nsapi_event_t event, intptr_t connection_status) {
    log_infoln("status change: {event: %d, connection_status: %d}", event,
               (nsapi_connection_status_t)connection_status);
    _connection_status = (nsapi_connection_status_t)connection_status;
  }

 private:
  int sync_timestamp() {
    if (_ntp_synced) return 0;

    log_debugln("try to sync timestamp");

    NTPClient ntp_client(interface());

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

    _ntp_synced = true;

    return 0;
  }

  nsapi_error_t connect() {
    log_debugln("connect");

    _wifi->disconnect();
    try_until_success(_wifi->reset(), WIFI_RESET_RETRY_INTERVAL);

    auto ns_ret = _wifi->connect(_ssid, _password, NSAPI_SECURITY_WPA_WPA2);
    if (ns_ret != 0) {
      ns_log_errorln(ns_ret, "failed to connect to wifi ap");
      return ns_ret;
    }
    log_infoln(
        "connected to wifi ap\n  mac: %s\n  ip: %s\n  netmask: %s\n  gateway: "
        "%s\n  rssi: %d",
        _wifi->get_mac_address(), _wifi->get_ip_address(), _wifi->get_netmask(),
        _wifi->get_gateway(), _wifi->get_rssi());

    // synchronize timestamp
    return sync_timestamp();
  }

 private:
  events::EventQueue* _event_queue;
  rtos::Thread* _thread;
  uint32_t _error_count[21];
  ISM43362Interface* _wifi;
  nsapi_connection_status_t _connection_status;
  const char* _ssid;
  const char* _password;
  bool _ntp_synced;
};

#endif
