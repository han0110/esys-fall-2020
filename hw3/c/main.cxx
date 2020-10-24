#include <errno.h>
#include <wiringPi.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <tuple>

#define now()                                              \
  std::chrono::duration_cast<std::chrono::milliseconds>(   \
      std::chrono::system_clock::now().time_since_epoch()) \
      .count()

#define PIN_LED 18
#define PIN_BUTTON 7
#define PWM_FREQUENCY 50

long long slowBlink(long long duration) {
  return (duration / 1000) % 2 ? 1023 : 0;
}

long long fastBlink(long long duration) {
  return (duration / 100) % 2 ? 1023 : 0;
}

long long gradualBlink(long long duration) {
  duration = duration % 2000;
  duration = duration > 1000 ? 2000 - duration : duration;
  return floor(1023 * duration / 1000);
}

std::array BLINK_MODES = {
    std::make_tuple(1000, slowBlink),
    std::make_tuple(100, fastBlink),
    std::make_tuple(10, gradualBlink),
};

volatile int blinkModeIdx = 0;
volatile long long lastEdgeTs = now();

void isr(void) {
  lastEdgeTs = now();
  if (blinkModeIdx == BLINK_MODES.size() - 1) {
    blinkModeIdx = 0;
  } else {
    blinkModeIdx++;
  }
  fprintf(stdout, "change blink mode to #%d\n", blinkModeIdx);
}

int main(void) {
  if (wiringPiSetupGpio() < 0) {
    fprintf(stderr, "failed to setup wiringPi: %s\n", strerror(errno));
    return 1;
  }

  pinMode(PIN_LED, PWM_OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  pullUpDnControl(PIN_BUTTON, PUD_UP);

  if (wiringPiISR(PIN_BUTTON, INT_EDGE_FALLING, &isr) < 0) {
    fprintf(stderr, "failed to setup ISR: %s\n", strerror(errno));
    return 1;
  }

  while (true) {
    auto dutyCycleFn = std::get<1>(BLINK_MODES[blinkModeIdx]);
    pwmWrite(PIN_LED, dutyCycleFn(now() - lastEdgeTs));

    auto sleepDuration = std::get<0>(BLINK_MODES[blinkModeIdx]);
    delay(sleepDuration);
  }

  return 0;
}
