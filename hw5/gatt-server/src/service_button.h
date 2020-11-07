#include <events/mbed_events.h>
#include <mbed.h>

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/gatt/GattCharacteristic.h"
#include "ble/gatt/GattService.h"
#include "ble/services/BatteryService.h"
#include "ble_logger.h"
#include "logger.h"

const UUID UUID_SERVICE_BUTTON = 0x0160;
const UUID UUID_CHARACTERISTIC_BUTTON_STATE = 0x0120;

class ButtonService {
  typedef ButtonService Self;

 public:
  ButtonService(bool button_state = false)
      : _event_queue(NULL),
        _gatt_server(NULL),
        _button_state(UUID_CHARACTERISTIC_BUTTON_STATE, &button_state,
                      GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ |
                          GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
        _button(USER_BUTTON) {}

  void start(ble::BLE& ble, events::EventQueue& event_queue) {
    static bool started = false;
    if (started) {
      return;
    }

    _event_queue = &event_queue;

    GattCharacteristic* characteristics[] = {&_button_state};
    GattService _service(UUID_SERVICE_BUTTON, characteristics,
                         sizeof(characteristics) / sizeof(GattCharacteristic*));

    _gatt_server = &ble.gattServer();

    ble_error_t ble_error = _gatt_server->addService(_service);
    if (ble_error) {
      ble_log_error(ble_error,
                    "failed to register service button to gatt server");
    }

    _gatt_server->onDataSent(makeFunctionPointer(this, &Self::on_data_sent));
    _button.fall(mbed::callback(this, &Self::on_button_pressed));
    _button.rise(mbed::callback(this, &Self::on_button_released));

    log_infoln("service button registered to gatt server");
    log_infoln("service button handle: %u", _service.getHandle());
    log_infoln("characteristic button_state handle: %u",
               _button_state.getValueHandle());

    started = true;
  }

 private:
  void set_button_state(bool button_state) {
    _gatt_server->write(_button_state.getValueHandle(),
                        new uint8_t(button_state ? 1 : 0), 1, false);
  }

  void on_button_pressed() {
    _event_queue->call(this, &Self::set_button_state, true);
  }

  void on_button_released() {
    _event_queue->call(this, &Self::set_button_state, false);
  }

  void on_data_sent(unsigned count) {
    log_infoln("button_state sent, count: %d", count);
  }

 private:
  events::EventQueue* _event_queue;
  GattServer* _gatt_server;

  ReadWriteGattCharacteristic<bool> _button_state;
  InterruptIn _button;
};
