#ifndef __CLIENT_TCP_H__
#define __CLIENT_TCP_H__

#include "TCPSocket.h"
#include "config.h"
#include "logger.h"
#include "logger_netsocket.h"
#include "mbed.h"
#include "mbed_chrono.h"
#include "wifi.h"

// Copy from ISM43362Interface.cpp
struct ISM43362_socket {
  int _;
  nsapi_protocol_t __;
  volatile bool connected;
};

class TCPClient : protected TCPSocket {
  typedef TCPClient Self;

 public:
  TCPClient(WIFI* wifi, SocketAddress socket_address)
      : TCPSocket(),
        _wifi(wifi),
        _socket_address(socket_address),
        _event_queue(new events::EventQueue),
        _connecting(false) {
    set_timeout(MBED_CONF_APP_SOCKET_TIMEOUT);
    _event_queue->call_in(3s, this, &Self::connect_socket_callback);
  }

 protected:
  void send(uint8_t* buf, int len) {
    _event_queue->call(this, &Self::send_callback, buf, len);
  }

  void dispatch_forever() { _event_queue->dispatch_forever(); }

  bool connected() {
    return _wifi->connected() && ((struct ISM43362_socket*)_socket)->connected;
  }

  virtual int handle_response() { return -1; };

 private:
  void connect_socket_callback() {
    log_debugln("connect_socket_callback");

    TCPSocket::close();

    // connect until success
    while (true) {
      nsapi_error_t ns_ret = 0;

      if (!_wifi->connected()) {
        goto RETRY;
      }
      log_debugln("_wifi->connected() ok");

      ns_ret = TCPSocket::open(_wifi->interface());
      if (ns_ret < 0) {
        ns_log_errorln(ns_ret, "failed to open socket");
        goto RETRY;
      }
      log_debugln("TCPSocket::open ok");

      ns_ret = TCPSocket::connect(_socket_address);
      if (ns_ret < 0) {
        ns_log_errorln(ns_ret, "failed to connect address");
        goto RETRY;
      }
      log_debugln("TCPSocket::connect ok");

      break;

    RETRY:
      if (ns_ret < 0) {
        TCPSocket::close();
        log_debugln("close ok");
        _wifi->report_error(ns_ret);
        log_debugln("report_error ok");
      }
      rtos::ThisThread::sleep_for(std::chrono::milliseconds(3s));
      continue;
    }

    _connecting = false;

    log_infoln("socket connected");
  }

  void send_callback(uint8_t* buf, int len) {
    log_debugln("send_callback");

    nsapi_size_or_error_t ns_ret = 0;

    // check whether remote socket is connected
    if (!connected()) {
      log_errorln("socket disconnected, connect again");
      goto RETRY;
    }

    ns_ret = TCPSocket::sendto(_socket_address, buf, len);
    if (ns_ret < 0) {
      ns_log_errorln(ns_ret, "failed to send data");
      goto RETRY;
    }

    ns_ret = handle_response();
    if (ns_ret < 0) {
      ns_log_errorln(ns_ret, "failed to handle response");
      goto RETRY;
    }

    // cleanup buf
    delete buf;
    return;

  RETRY:
    // NOTE: currently we just call this event again after connected.
    //       implement these callback when when offline mode is supported.
    // _event_queue->call(this, &Self::store_data_callback, buf, len);
    // if (!_connecting) {
    //   _connecting = true;
    //   _event_queue->call(this, &Self::connect_socket_callback);
    //   _event_queue->call(this, &Self::load_data_callback, buf, len);
    // }

    // connect socket if not connecting
    if (!_connecting) {
      _connecting = true;
      _wifi->report_error(ns_ret);
      _event_queue->call(this, &Self::connect_socket_callback);
    }
    // call this event again
    send(buf, len);
  }

 private:
  WIFI* _wifi;
  SocketAddress _socket_address;
  events::EventQueue* _event_queue;
  bool _connecting;
};

#endif
