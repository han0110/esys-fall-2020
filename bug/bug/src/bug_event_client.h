#ifndef __BUG_EVENT_CLIENT_H__
#define __BUG_EVENT_CLIENT_H__

#include "bug_event.h"
#include "client_tcp.h"
#include "config.h"
#include "logger.h"
#include "logger_netsocket.h"
#include "mbed.h"

class BugEventClient : public TCPClient {
  typedef BugEventClient Self;

 public:
  BugEventClient(NetworkInterface* interface,
                 SocketAddress server_address =
                     SocketAddress(MBED_CONF_APP_BUG_BACKEND_SERVER_IP,
                                   MBED_CONF_APP_BUG_BACKEND_SERVER_PORT),
                 uint32_t secret = MBED_CONF_APP_BUG_CLIENT_SECRET)
      : TCPClient(interface, server_address),
        _thread(new rtos::Thread),
        _codec(new BugEventCodec(secret)) {}

  void send(BugEventKind kind, time_t ts) {
    uint8_t* buf = new uint8_t[128];
    int len = _codec->encode(buf, kind, ts);
    TCPClient::send(buf, len);
  }

  void start() {
    _thread->start(mbed::callback(this, &Self::dispatch_forever));
  }

  void join() { _thread->join(); }

 private:
  rtos::Thread* _thread;
  BugEventCodec* _codec;
};

#endif
