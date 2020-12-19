#include "bug_event.h"
#include "bug_event_client.h"
#include "logger.h"
#include "mbed.h"
#include "wifi.h"

using namespace std::chrono;

int main() {
  WIFI wifi;
  BugEventClient client(wifi.interface());

  // connect to wifi until success
  while (wifi.connect() < 0) {
    // wait for 10 second to connect again
    rtos::ThisThread::sleep_for(10s);
  }

  // start thread to run dispatch_forever
  client.start();

  while (true) {
    // send something and sleep
    time_t now = time(NULL);
    client.send(Luminosity{from : 10, to : 800}, now);
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(3s));
  }

  // join thread
  client.join();
}
