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

#ifndef __BLE_LOGGER_H__
#define __BLE_LOGGER_H__

#include <mbed.h>

#include "ble/BLE.h"
#include "logger.h"

inline void ble_log_error(ble_error_t error, const char* msg) {
  log_error("%s: ", msg);
  switch (error) {
    case BLE_ERROR_NONE:
      log_error("BLE_ERROR_NONE: no error");
      break;
    case BLE_ERROR_BUFFER_OVERFLOW:
      log_error(
          "BLE_ERROR_BUFFER_OVERFLOW: the requested action would cause a "
          "buffer overflow and has been aborted");
      break;
    case BLE_ERROR_NOT_IMPLEMENTED:
      log_error(
          "BLE_ERROR_NOT_IMPLEMENTED: requested a feature that isn't yet "
          "implemented or isn't supported by the target HW");
      break;
    case BLE_ERROR_PARAM_OUT_OF_RANGE:
      log_error(
          "BLE_ERROR_PARAM_OUT_OF_RANGE: one of the supplied parameters is "
          "outside the valid range");
      break;
    case BLE_ERROR_INVALID_PARAM:
      log_error(
          "BLE_ERROR_INVALID_PARAM: one of the supplied parameters is "
          "invalid");
      break;
    case BLE_STACK_BUSY:
      log_error("BLE_STACK_BUSY: the stack is busy");
      break;
    case BLE_ERROR_INVALID_STATE:
      log_error("BLE_ERROR_INVALID_STATE: invalid state");
      break;
    case BLE_ERROR_NO_MEM:
      log_error("BLE_ERROR_NO_MEM: out of memory");
      break;
    case BLE_ERROR_OPERATION_NOT_PERMITTED:
      log_error(
          "BLE_ERROR_OPERATION_NOT_PERMITTED: the operation requested is not "
          "permitted");
      break;
    case BLE_ERROR_INITIALIZATION_INCOMPLETE:
      log_error(
          "BLE_ERROR_INITIALIZATION_INCOMPLETE: the ble subsystem has not "
          "completed its initialization");
      break;
    case BLE_ERROR_ALREADY_INITIALIZED:
      log_error(
          "BLE_ERROR_ALREADY_INITIALIZED: the ble system has already been "
          "initialized");
      break;
    case BLE_ERROR_UNSPECIFIED:
      log_error("BLE_ERROR_UNSPECIFIED: unknown error");
      break;
    case BLE_ERROR_INTERNAL_STACK_FAILURE:
      log_error(
          "BLE_ERROR_INTERNAL_STACK_FAILURE: the platform-specific stack "
          "failed");
      break;
    case BLE_ERROR_NOT_FOUND:
      log_error(
          "BLE_ERROR_NOT_FOUND: data not found or there is nothing to return");
      break;
    default:
      log_error("unknown error");
  }
  log_errorln();
}

inline void ble_log_address(const ble::address_t& addr) {
  log_infoln("%02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3],
             addr[2], addr[1], addr[0]);
}

inline void ble_log_address() {
  ble::own_address_type_t addr_type;
  ble::address_t address;
  BLE::Instance().gap().getAddress(addr_type, address);
  log_info("device mac address: ");
  ble_log_address(address);
}

#endif
