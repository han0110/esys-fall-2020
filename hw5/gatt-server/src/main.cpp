/* mbed Microcontroller Library
 * Copyright (c) 2017-2019 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <events/mbed_events.h>
#include <mbed.h>

#include "ble_process.h"
#include "logger.h"
#include "service_button.h"
#include "service_led.h"
#include "service_student_id.h"

int main() {
  events::EventQueue event_queue;
  BLE &ble = BLE::Instance();
  BLEProcess ble_process(event_queue, ble);

  LEDService led_service(false);
  ButtonService button_service(false);
  StudentIDService student_id_service("bxx901xxx");
  ble_process.on_init(mbed::callback(&led_service, &LEDService::start));
  ble_process.on_init(mbed::callback(&button_service, &ButtonService::start));
  ble_process.on_init(
      mbed::callback(&student_id_service, &StudentIDService::start));
  ble_process.start();

  return 0;
}
