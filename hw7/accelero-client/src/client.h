#include "TCPSocket.h"
#include "events/mbed_events.h"
#include "logger.h"
#include "mbed.h"
#include "ns_logger.h"
#include "service_accelero.h"
#include "service_button.h"

#define WIFI_IDW0XX1 2

#if (defined(TARGET_DISCO_L475VG_IOT01A))
#include "ISM43362Interface.h"

#if defined LOG_LEVEL_DEBUG
ISM43362Interface wifi(true);
#else
ISM43362Interface wifi(false);
#endif

#else

#if MBED_CONF_APP_WIFI_SHIELD == WIFI_IDW0XX1
#include "SpwfSAInterface.h"
SpwfSAInterface wifi(MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX);
#endif

#endif

class Client {
  typedef Client Self;

 public:
  Client(ButtonService& button_service, AcceleroService& accelero_service)
      : _thread(new rtos::Thread),
        _event_queue(new events::EventQueue),
        _socket(NULL),
        _button_service(&button_service),
        _accelero_service(&accelero_service) {}

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

    nsapi_error_t socket_ret;
    SocketAddress addr(MBED_CONF_APP_WEBSOCKET_SERVER_IP,
                       MBED_CONF_APP_WEBSOCKET_SERVER_PORT);

    _socket = new TCPSocket;
    socket_ret = _socket->open((NetworkInterface*)&wifi);
    if (socket_ret) {
      ns_log_error(socket_ret, "failed to open socket");
      return socket_ret;
    }

    socket_ret = _socket->connect(addr);
    if (socket_ret) {
      ns_log_error(socket_ret, "failed to connect address");
      return socket_ret;
    }

    _socket->set_blocking(true);

    return ret;
  }

  void start() {
    _event_queue->call_every(20, this, &Self::send);
    _thread->start(
        mbed::callback(_event_queue, &events::EventQueue::dispatch_forever));
  }

  void join() { _thread->join(); }

 private:
  void send() {
    char data[64];
    int len =
        sprintf(data, "{\"x\":%d,\"y\":%d,\"z\":%d,\"bp\":%s,\"ts\":%llu}",
                _accelero_service->xyz[0], _accelero_service->xyz[1],
                _accelero_service->xyz[2],
                _button_service->button_pressed ? "true" : "false",
                rtos::Kernel::get_ms_count());

    nsapi_error_t socket_ret = _socket->send(data, len);
    if (socket_ret < 0) {
      ns_log_error(socket_ret, "failed to send data");
    }
  }

 private:
  rtos::Thread* _thread;
  events::EventQueue* _event_queue;
  TCPSocket* _socket;
  ButtonService* _button_service;
  AcceleroService* _accelero_service;
};
