#include "client.h"
#include "logger.h"
#include "mbed.h"
#include "ns_logger.h"
#include "service_accelero.h"
#include "service_button.h"

int main() {
  int ret;

  ButtonService button_service;
  AcceleroService accelero_service;
  Client client(button_service, accelero_service);

  ret = accelero_service.init();
  if (ret) {
    log_errorln("failed to init to accelero service");
    return ret;
  }

  ret = client.connect();
  if (ret) {
    log_errorln("failed to connnect to websocket server");
    return ret;
  }

  accelero_service.start();
  client.start();
  client.join();
  accelero_service.join();
}
