#ifndef __CLIENT_TCP_H__
#define __CLIENT_TCP_H__

#include "TCPSocket.h"
#include "logger.h"
#include "logger_netsocket.h"
#include "mbed.h"
#include "mbed_chrono.h"

// Copy from ISM43362Interface.cpp
struct ISM43362_socket {
  int id;
  nsapi_protocol_t proto;
  volatile bool connected;
  // SocketAddress addr;
  // char read_data[1400];
  // volatile uint32_t read_data_size;
};

class TCPClient : public TCPSocket {
  typedef TCPClient Self;

 public:
  TCPClient(NetworkInterface* interface, SocketAddress socket_address)
      : TCPSocket(),
        _interface(interface),
        _socket_address(socket_address),
        _event_queue(new events::EventQueue) {
    _event_queue->call(this, &Self::connect_socket_callback);
  }

 protected:
  void send(uint8_t* buf, int len) {
    _event_queue->call(this, &Self::send_callback, buf, len);
  }

  void dispatch_forever() { _event_queue->dispatch_forever(); }

  bool connected() { return ((struct ISM43362_socket*)_socket)->connected; }

 private:
  void connect_socket_callback() {
    log_debugln("connect_socket_callback");

    // connect until success
    while (true) {
      TCPSocket::close();

      nsapi_error_t ns_ret = TCPSocket::open((NetworkInterface*)_interface);
      if (ns_ret) {
        ns_log_errorln(ns_ret, "failed to open socket");
        goto RETRY;
      }

      ns_ret = TCPSocket::connect(_socket_address);
      if (ns_ret) {
        ns_log_errorln(ns_ret, "failed to connect address");
        goto RETRY;
      }

      log_infoln("socket connected");
      break;

    RETRY:
      rtos::ThisThread::sleep_for(std::chrono::milliseconds(3s));
      continue;
    }
  }

  void send_callback(uint8_t* buf, int len) {
    log_debugln("send_callback");

    nsapi_size_or_error_t ns_ret;

    if (!Self::connected()) {
      log_errorln("socket disconnected, connect again");
      goto RETRY;
    }

    // log_debugln("sendto ret: %d", ns_ret);
    ns_ret = TCPSocket::sendto(_socket_address, buf, len);
    if (ns_ret < 0) {
      ns_log_errorln(ns_ret, "failed to send data");
      goto RETRY;
    }

    // cleanup buf
    delete buf;
    return;

  RETRY:
    // connect server again
    _event_queue->call(this, &Self::connect_socket_callback);
    // call this event again
    send(buf, len);
  }

 private:
  const NetworkInterface* _interface;
  SocketAddress _socket_address;
  events::EventQueue* _event_queue;
};

#endif
