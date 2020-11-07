#include <events/mbed_events.h>

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/gatt/GattCharacteristic.h"
#include "ble/gatt/GattService.h"
#include "ble/services/BatteryService.h"
#include "ble_logger.h"
#include "logger.h"

const UUID UUID_SERVICE_STUDENT = 0x0140;
const UUID UUID_CHARACTERISTIC_STUDENT_ID = 0x0112;

class StudentIDService {
  typedef StudentIDService Self;

 public:
  StudentIDService(const char* student_id = nullptr)
      : _student_id(UUID_CHARACTERISTIC_STUDENT_ID, (uint8_t*)student_id,
                    student_id == nullptr ? 0 : strlen(student_id),
                    student_id == nullptr ? 0 : strlen(student_id),
                    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ) {}

  void start(ble::BLE& ble, events::EventQueue& event_queue) {
    static bool started = false;
    if (started) {
      return;
    }

    GattCharacteristic* characteristics[] = {&_student_id};
    GattService _service(UUID_SERVICE_STUDENT, characteristics,
                         sizeof(characteristics) / sizeof(characteristics[0]));

    GattServer* gatt_server = &ble.gattServer();

    ble_error_t ble_error = gatt_server->addService(_service);
    if (ble_error) {
      ble_log_error(ble_error,
                    "failed to register service student_id to gatt server");
    }

    gatt_server->onDataRead(makeFunctionPointer(this, &Self::on_data_read));

    log_infoln("service student_id registered to gatt server");
    log_infoln("service student_id handle: %u", _service.getHandle());
    log_infoln("characteristic student_id handle: %u",
               _student_id.getValueHandle());

    started = true;
  }

 private:
  void on_data_read(const GattReadCallbackParams* params) {
    if (params->handle == _student_id.getValueHandle()) {
      log_infoln("student_id read");
    }
  }

 private:
  GattCharacteristic _student_id;
};
