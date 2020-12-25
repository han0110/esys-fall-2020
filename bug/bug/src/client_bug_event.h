#ifndef __BUG_EVENT_CLIENT_H__
#define __BUG_EVENT_CLIENT_H__

#include "bug_event.h"
#include "client_tcp.h"
#include "config.h"
#include "logger.h"
#include "logger_netsocket.h"
#include "mbed.h"
#include "wifi.h"

#define BUG_SERVER_RESPONSE_OK 1

class BugEventClient : public TCPClient {
  typedef BugEventClient Self;

 public:
  BugEventClient(WIFI* wifi,
                 SocketAddress server_address =
                     SocketAddress(MBED_CONF_APP_BUG_BACKEND_SERVER_IP,
                                   MBED_CONF_APP_BUG_BACKEND_SERVER_PORT),
                 uint32_t secret = MBED_CONF_APP_BUG_CLIENT_SECRET)
      : TCPClient(wifi, server_address),
        _thread(new rtos::Thread),
        _codec(new BugEventCodec(secret)) {}

  ~BugEventClient() {
    delete _thread;
    delete _codec;
  }

  void send(BugEventKind kind, TimeRange& tr) {
    uint8_t* buf = new uint8_t[128];
    int len = _codec->encode(buf, kind, tr);
    TCPClient::send(buf, len);
  }

 protected:
  void start() {
    _thread->start(mbed::callback(this, &Self::dispatch_forever));
  }

  void join() { _thread->join(); }

  int handle_response() {
    uint8_t buf[1] = {0};

    auto ns_ret = TCPSocket::recv(buf, 1);
    if (ns_ret < 0) {
      return ns_ret;
    }

    if (*buf != BUG_SERVER_RESPONSE_OK) {
      return -1;
    }

    return 0;
  }

 private:
  rtos::Thread* _thread;
  BugEventCodec* _codec;
};

#endif
