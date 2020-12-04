from RPi import GPIO
from smbus import SMBus
from time import sleep

# From ADXL345 data sheet: https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf
I2C_ADDRESS_LOW = 0x53
I2C_ADDRESS_HIGH = 0x1D
REG_THRESH_ACT = 0x24
REG_THRESH_INACT = 0x25
REG_TIME_INACT = 0x26
REG_INT_ENABLE = 0x2E
REG_INT_SOURCE = 0x30
REG_VAL_INT_ENABLE_ACTIVITY = 1 << 4
REG_VAL_INT_ENABLE_INACTIVITY = 1 << 3
REG_INT_MAP = 0x2F
REG_VAL_INT_MAP_INT2_ACTIVITY = 1 << 4
REG_VAL_INT_MAP_INT2_INACTIVITY = 1 << 3
REG_INT_SOURCE = 0x30

# My config
REG_VAL_THRESH_ACT = 20  # in scale factor of 62.5 mg/LSB
REG_VAL_THRESH_INACT = 20  # in scale factor of 62.5 mg/LSB
REG_VAL_TIME_INACT = 1  # in scale factor of second/LSB
PIN_INT2 = 7


def callback():
    inter = bus.read_byte_data(I2C_ADDRESS_LOW, REG_INT_SOURCE)
    if inter & REG_VAL_INT_ENABLE_INACTIVITY:
        print("interrupt ACT detected")
    if inter & REG_VAL_INT_ENABLE_ACTIVITY:
        print("interrupt INACT detected")


if __name__ == "__main__":
    try:
        # Init
        bus = SMBus(1)
        GPIO.setmode(GPIO.BCM)

        # Register callback
        GPIO.setup(PIN_INT2, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        GPIO.add_event_detect(
            PIN_INT2, GPIO.FALLING, callback=callback, bouncetime=50)

        # Enable interrupt of activity and inactivity
        bus.write_byte_data(I2C_ADDRESS_LOW, REG_INT_ENABLE,
                            REG_VAL_INT_ENABLE_ACTIVITY | REG_VAL_INT_ENABLE_INACTIVITY)

        # Set activity threshold
        bus.write_byte_data(
            I2C_ADDRESS_LOW, REG_THRESH_ACT, REG_VAL_THRESH_ACT)

        # Set inactivity threshold and time
        bus.write_byte_data(I2C_ADDRESS_LOW, REG_THRESH_INACT,
                            REG_VAL_THRESH_INACT)
        bus.write_byte_data(I2C_ADDRESS_LOW, REG_TIME_INACT,
                            REG_VAL_TIME_INACT)

        # Set interrupt map of activity and inactivity
        bus.write_byte_data(I2C_ADDRESS_LOW, REG_INT_MAP,
                            REG_VAL_INT_MAP_INT2_ACTIVITY & REG_VAL_INT_MAP_INT2_INACTIVITY)

        while True:
            sleep(1)

    except KeyboardInterrupt:
        pass

    finally:
        GPIO.cleanup()
