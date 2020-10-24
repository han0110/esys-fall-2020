package main

import (
	"fmt"
	"math"
	"time"

	"periph.io/x/periph/conn/gpio"
	"periph.io/x/periph/conn/physic"
	"periph.io/x/periph/host"
	"periph.io/x/periph/host/rpi"
)

const Frequency = 50 * physic.Hertz

var BlinkModes = []struct {
	sleepDuration time.Duration
	dutyCycleFn   func(time.Duration) gpio.Duty
}{
	{
		sleepDuration: 1000 * time.Millisecond,
		dutyCycleFn: func(duration time.Duration) gpio.Duty {
			if int64(math.Floor(duration.Seconds()))%2 == 1 {
				return gpio.DutyMax
			}
			return gpio.Duty(0)
		}},
	{
		sleepDuration: 100 * time.Millisecond,
		dutyCycleFn: func(duration time.Duration) gpio.Duty {
			if int64(math.Floor(duration.Seconds()*10))%2 == 1 {
				return gpio.DutyMax
			}
			return gpio.Duty(0)
		}},
	{
		sleepDuration: 10 * time.Millisecond,
		dutyCycleFn: func(duration time.Duration) gpio.Duty {
			ms := duration.Milliseconds() % 2000
			if ms > 1000 {
				ms = 2000 - ms
			}
			return gpio.Duty(ms * int64(gpio.DutyMax) / 1000)
		}},
}

func init() {
	_, err := host.Init()
	checkErr(err)
}

func main() {
	var (
		pinButton gpio.PinIn  = rpi.P1_26
		pinLED    gpio.PinOut = rpi.P1_12

		blinkModeIdx int
		lastEdgeTs   time.Time = time.Now()
	)

	checkErr(pinButton.In(gpio.PullDown, gpio.RisingEdge))
	go func() {
		for pinButton.WaitForEdge(-1) {
			lastEdgeTs = time.Now()
			if blinkModeIdx == len(BlinkModes)-1 {
				blinkModeIdx = 0
			} else {
				blinkModeIdx++
			}
			fmt.Printf("change blink mode to #%d\n", blinkModeIdx)
		}
	}()

	for {
		checkErr(pinLED.PWM(BlinkModes[blinkModeIdx].dutyCycleFn(time.Since(lastEdgeTs)), Frequency))
		time.Sleep(BlinkModes[blinkModeIdx].sleepDuration)
	}
}

func checkErr(err error) {
	if err != nil {
		panic(err)
	}
}
