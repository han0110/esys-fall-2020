#include "bug_event.h"
#include "bug_event_client.h"
#include "logger.h"
#include "mbed.h"

int main() {
  BugEventClient client(MBED_CONF_APP_BUG_CLIENT_SECRET);

  // connect to wifi until success
  while (client.connect() < 0) {
    // wait for 10 second to connect again
    rtos::ThisThread::sleep_for(chrono::milliseconds(10 * 1000));
  }

  // start thread to run dispatch_forever
  client.start();

  // join thread
  client.join();
}
