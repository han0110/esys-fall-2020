#ifndef __BUG_EVENT_CLIENT_H__
#define __BUG_EVENT_CLIENT_H__

#include "ISM43362Interface.h"
#include "NTPClient.h"
#include "TCPSocket.h"
#include "bug_event.h"
#include "config.h"
#include "events/mbed_events.h"
#include "logger.h"
#include "logger_netsocket.h"
#include "mbed.h"

#if defined LOG_LEVEL_TRACE
ISM43362Interface wifi(true);
#else
ISM43362Interface wifi(false);
#endif

class BugEventClient {
  typedef BugEventClient Self;

 public:
  BugEventClient(uint64_t secret)
      : _thread(new rtos::Thread),
        _event_queue(new events::EventQueue),
        _socket(NULL),
        _codec(new BugEventCodec(secret)) {}

  int connect() {
    int ret;

    ret = wifi.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD,
                       NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
      log_infoln("failed to connect to wifi ap");
      return -1;
    }
    log_infoln(
        "connected to wifi ap\n  mac: %s\n  ip: %s\n  netmask: %s\n  gateway: "
        "%s\n  rssi: %d",
        wifi.get_mac_address(), wifi.get_ip_address(), wifi.get_netmask(),
        wifi.get_gateway(), wifi.get_rssi());

    ret = sync_timestamp();
    if (ret) {
      return ret;
    }

    nsapi_error_t socket_ret;
    SocketAddress addr(MBED_CONF_APP_UDP_SOCKET_IP,
                       MBED_CONF_APP_UDP_SOCKET_PORT);

    _socket = new TCPSocket;
    socket_ret = _socket->open((NetworkInterface*)&wifi);
    if (socket_ret) {
      ns_log_errorln(socket_ret, "failed to open socket");
      return socket_ret;
    }

    socket_ret = _socket->connect(addr);
    if (socket_ret) {
      ns_log_errorln(socket_ret, "failed to connect address");
      return socket_ret;
    }

    return ret;
  }

  int sync_timestamp() {
    NTPClient ntp_client((NetworkInterface*)&wifi);

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

  void send(BugEventKind kind, time_t ts) {
    _event_queue->call(this, &Self::send_callback, kind, ts);
  }

  void start() {
    _thread->start(
        mbed::callback(_event_queue, &events::EventQueue::dispatch_forever));
  }

  void join() { _thread->join(); }

 private:
  void send_callback(BugEventKind kind, time_t ts) {
    uint8_t buf[128];
    int len = _codec->encode(buf, kind, ts);

    auto socket_ret = _socket->send(buf, len);
    if (socket_ret < 0) {
      ns_log_errorln(socket_ret, "failed to send data");
    }
  }

 private:
  rtos::Thread* _thread;
  events::EventQueue* _event_queue;
  TCPSocket* _socket;
  BugEventCodec* _codec;
};

#endif
