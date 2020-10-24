from RPi import GPIO
from time import sleep, time
from math import floor

PIN_LED = 18
PIN_BUTTON = 7
PWM_FREQUENCY = 50

BLINK_MODES = [
    (1, lambda duration: ((duration // 1000) % 2) * 100),
    (0.1, lambda duration: ((duration // 100) % 2) * 100),
    (0.02, lambda duration: (2000 - (duration % 2000)
                             if (duration % 2000) > 1000 else duration % 2000) / 10),
]

def milli():
    return int(round(time() * 1000))

if __name__ == "__main__":
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(PIN_LED, GPIO.OUT)
    GPIO.setup(PIN_BUTTON, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    blink_mode_idx, last_edge_ts = 0, milli()

    def callback(channel):
        global last_edge_ts, blink_mode_idx

        last_edge_ts = milli()

        if blink_mode_idx == len(BLINK_MODES) - 1:
            blink_mode_idx = 0
        else:
            blink_mode_idx += 1
        print("change blink mode to #%d" % blink_mode_idx)

    GPIO.add_event_detect(
        PIN_BUTTON, GPIO.FALLING, callback=callback, bouncetime=100)

    try:
        pwm = GPIO.PWM(PIN_LED, PWM_FREQUENCY)
        pwm.start(0)

        while True:
            (sleep_duration, duty_cycle_fn) = BLINK_MODES[blink_mode_idx]
            pwm.ChangeDutyCycle(duty_cycle_fn(milli() - last_edge_ts))
            sleep(sleep_duration)

    except KeyboardInterrupt:
        pass

    pwm.stop()
    GPIO.cleanup()
