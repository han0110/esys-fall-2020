#include <events/mbed_events.h>

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/gatt/GattCharacteristic.h"
#include "ble/gatt/GattService.h"
#include "ble/services/BatteryService.h"
#include "ble_logger.h"
#include "logger.h"

const UUID UUID_SERVICE_LED = 0x0120;
const UUID UUID_CHARACTERISTIC_LED_STATE = 0x0100;

class LEDService {
  typedef LEDService Self;

 public:
  LEDService(bool led_state = false)
      : _led_state(UUID_CHARACTERISTIC_LED_STATE, &led_state),
        _led(LED1, led_state) {}

  void start(ble::BLE& ble, events::EventQueue& event_queue) {
    static bool started = false;
    if (started) {
      return;
    }

    GattCharacteristic* characteristics[] = {&_led_state};
    GattService _service(UUID_SERVICE_LED, characteristics,
                         sizeof(characteristics) / sizeof(GattCharacteristic*));

    GattServer* gatt_server = &ble.gattServer();

    ble_error_t ble_error = gatt_server->addService(_service);
    if (ble_error) {
      ble_log_error(ble_error, "failed to register service led to gatt server");
    }

    gatt_server->onDataWritten(
        makeFunctionPointer(this, &Self::on_data_written));

    log_infoln("service led registered to gatt server");
    log_infoln("service led handle: %u", _service.getHandle());
    log_infoln("characteristic led_state handle: %u",
               _led_state.getValueHandle());

    started = true;
  }

 private:
  void on_data_written(const GattWriteCallbackParams* params) {
    if (params->handle == _led_state.getValueHandle()) {
      _led = !_led;
      if (params->writeOp != GattWriteCallbackParams::WriteOp_t::OP_INVALID &&
          params->len == 1) {
        uint8_t led_state = params->data[0];
        _led = bool(led_state);
        log_infoln("led_state written: %s", led_state ? "true" : "false");
      }
    }
  }

  ReadWriteGattCharacteristic<bool> _led_state;
  DigitalOut _led;
};
