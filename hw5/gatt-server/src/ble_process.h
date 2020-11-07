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

#include <vector>

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble_logger.h"

#define DEVICE_NAME "BLE GATT 101"

static const uint16_t MAX_ADVERTISING_PAYLOAD_SIZE = 50;

class BLEProcess : private mbed::NonCopyable<BLEProcess>,
                   public ble::Gap::EventHandler {
 public:
  BLEProcess(events::EventQueue &event_queue, ble::BLE &ble)
      : _event_queue(event_queue),
        _ble(ble),
        _gap(ble.gap()),
        _adv_data_builder(_adv_buffer),
        _adv_handle(ble::LEGACY_ADVERTISING_HANDLE),
        _post_init_cbs(NULL) {}

  void start() {
    if (_ble.hasInitialized()) {
      log_errorln("the ble instance has already been initialized");
      return;
    }

    _gap.setEventHandler(this);

    _ble.onEventsToProcess(
        makeFunctionPointer(this, &BLEProcess::schedule_ble_events));

    ble_error_t ble_error = _ble.init(this, &BLEProcess::on_init_complete);

    if (ble_error) {
      ble_log_error(ble_error, "failed to init ble process");
    }

    log_debugln("ble process started");

    _event_queue.dispatch_forever();
  }

  void stop() {
    if (!_ble.hasInitialized()) {
      return;
    }

    _ble.shutdown();

    log_debugln("ble process stopped");
  }

  void on_init(mbed::Callback<void(ble::BLE &, events::EventQueue &)> cb) {
    _post_init_cbs.push_back(cb);
  }

 private:
  void on_init_complete(ble::BLE::InitializationCompleteCallbackContext *ctx) {
    if (ctx->error) {
      ble_log_error(ctx->error, "failed when initialzation");
      return;
    }

    log_infoln("ble initialized");

    _event_queue.call(this, &BLEProcess::start_advertising);

    for (auto cb : _post_init_cbs) {
      cb(_ble, _event_queue);
    }
  }

  void start_advertising() {
    ble_error_t ble_error;

    ble::AdvertisingParameters adv_params;

    ble_error = _gap.setAdvertisingParameters(_adv_handle, adv_params);
    if (ble_error) {
      ble_log_error(ble_error, "failed to set advertising parameters");
    }

    _adv_data_builder.clear();
    _adv_data_builder.setFlags();
    _adv_data_builder.setName(DEVICE_NAME);

    ble_error = _gap.setAdvertisingPayload(
        _adv_handle, _adv_data_builder.getAdvertisingData());
    if (ble_error) {
      ble_log_error(ble_error, "failed to set advertising data");
    }

    ble_error = _gap.startAdvertising(_adv_handle);
    if (ble_error) {
      ble_log_error(ble_error, "failed to start advertising");
    }

    log_infoln("start to advertising");
  }

  void schedule_ble_events(ble::BLE::OnEventsToProcessCallbackContext *event) {
    _event_queue.call(mbed::callback(&event->ble, &ble::BLE::processEvents));
  }

 private:
  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event) {
    if (event.getStatus() == BLE_ERROR_NONE) {
      log_infoln("connection established");
    } else {
      log_infoln("connection failed");
      _event_queue.call(this, &BLEProcess::start_advertising);
    }
  }

  virtual void onDisconnectionComplete(
      const ble::DisconnectionCompleteEvent &event) {
    log_infoln("connection reset");
    _event_queue.call(this, &BLEProcess::start_advertising);
  }

 private:
  events::EventQueue &_event_queue;
  ble::BLE &_ble;
  ble::Gap &_gap;

  uint8_t _adv_buffer[MAX_ADVERTISING_PAYLOAD_SIZE];
  ble::AdvertisingDataBuilder _adv_data_builder;
  ble::advertising_handle_t _adv_handle;

  std::vector<mbed::Callback<void(ble::BLE &, events::EventQueue &)>>
      _post_init_cbs;
};
