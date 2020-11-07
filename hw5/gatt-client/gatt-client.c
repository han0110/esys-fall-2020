#include <glib.h>
#include <stdlib.h>

#include "gattlib.h"

#define __log(stream, level, x, ...) \
  fprintf(stream, "[" level "](%s:%d): " x, __FILE__, __LINE__, ##__VA_ARGS__);
#define __logln(stream, level, x, ...)                                 \
  fprintf(stream, "[" level "](%s:%d): " x "\r\n", __FILE__, __LINE__, \
          ##__VA_ARGS__);

#define log_infoln(x, ...) __logln(stdout, "INFO", x, ##__VA_ARGS__);
#define log_errorln(x, ...) __logln(stderr, "ERROR", x, ##__VA_ARGS__);

#define BLE_SCAN_TIMEOUT 3
#define BLE_GATT_SERVER_DEVICE_NAME "BLE GATT 101"

const uuid_t UUID_CHARACTERISTIC_WRITABLE_LED = CREATE_UUID16(0x0100);
const uuid_t UUID_CHARACTERISTIC_READABLE_STUDENT_ID = CREATE_UUID16(0x0112);
const uuid_t UUID_CHARACTERISTIC_NOTIFICATION_BUTTON = CREATE_UUID16(0x0120);

static GMainLoop* m_main_loop;

static void discovered_device_cb(void* adapter, const char* addr,
                                 const char* name, void* data) {
  char** addr_dst = ((void**)data)[0];
  const char* target_name = ((void**)data)[1];
  if (name) {
    log_infoln("device discovered, name: %s, addr: %s", name, addr);

    if (strncmp(name, target_name, strlen(target_name)) == 0) {
      int size = strlen(addr);
      *addr_dst = malloc(size);
      strncpy(*addr_dst, addr, size + 1);
    }
  }
}

int discover_gatt_server_addr(char** addr, const char* target_name) {
  int ret;
  void* adapter;

  ret = gattlib_adapter_open(NULL, &adapter);
  if (ret) {
    log_errorln("failed to open adapter");
    return ret;
  }

  ret = gattlib_adapter_scan_enable(adapter, discovered_device_cb,
                                    BLE_SCAN_TIMEOUT,
                                    &(const void*[]){addr, target_name});
  if (ret) {
    log_errorln("failed to enable adapter scan");
    goto CLOSE_EXIT;
  }

  ret = gattlib_adapter_scan_disable(adapter);
  if (ret) {
    log_errorln("failed to disable adapter scan");
    goto CLOSE_EXIT;
  }

CLOSE_EXIT:
  gattlib_adapter_close(adapter);
  return ret;
}

int write_led_state(uint8_t** led_state, gatt_connection_t* gatt_connection) {
  gattlib_write_char_by_uuid(gatt_connection,
                             (uuid_t*)&UUID_CHARACTERISTIC_WRITABLE_LED,
                             &led_state, 1);
}

int read_student_id(uint8_t** stduent_id, gatt_connection_t* gatt_connection) {
  int ret;
  size_t len;

  ret = gattlib_read_char_by_uuid(
      gatt_connection, (uuid_t*)&UUID_CHARACTERISTIC_READABLE_STUDENT_ID,
      (void**)stduent_id, &len);
  if (ret) {
    return ret;
  }

  return 0;
}

void notification_handler(const uuid_t* uuid, const uint8_t* data,
                          size_t data_length, void* user_data) {
  log_infoln("button_state notified: %s", data[0] ? "true" : "false");
}

static void on_user_abort(int arg) {
  log_infoln("got sigterm, quit main loop");
  g_main_loop_quit(m_main_loop);
}

int handle_notification(gatt_connection_t* gatt_connection) {
  int ret;

  gattlib_register_notification(gatt_connection, notification_handler, NULL);

  ret = gattlib_notification_start(gatt_connection,
                                   &UUID_CHARACTERISTIC_NOTIFICATION_BUTTON);
  if (ret) {
    log_errorln("failed to start notification");
    return ret;
  }

  signal(SIGINT, on_user_abort);

  m_main_loop = g_main_loop_new(NULL, 0);
  g_main_loop_run(m_main_loop);

  ret = gattlib_notification_stop(gatt_connection,
                                  &UUID_CHARACTERISTIC_NOTIFICATION_BUTTON);
  if (ret) {
    log_errorln("failed to stop notification");
  }

  g_main_loop_unref(m_main_loop);

  return ret;
}

int main() {
  int ret;

  char* addr;
  ret = discover_gatt_server_addr(&addr, BLE_GATT_SERVER_DEVICE_NAME);
  if (ret) {
    return ret;
  }

  if (!addr) {
    log_errorln("failed to discover target %s", BLE_GATT_SERVER_DEVICE_NAME);
    return 1;
  }
  log_infoln("target device discovered, addr: %s", addr);

  gatt_connection_t* gatt_connection =
      gattlib_connect(NULL, addr, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
  if (gatt_connection == NULL) {
    log_errorln("failed to establish connection");
    return 1;
  }

  // HW5 Problem #1 - Write LED value to peripheral.
  uint8_t led_state[1] = {1};
  ret = write_led_state((uint8_t**)&led_state, gatt_connection);
  if (ret) {
    log_errorln("failed to write led_state to true");
    goto DISCONNECT_EXIT;
  }
  log_infoln("led_state written: %s", led_state[0] ? "true" : "false");

  // HW5 Problem #3 - Read your student id from peripheral.
  uint8_t* student_id = NULL;
  ret = read_student_id(&student_id, gatt_connection);
  if (ret) {
    log_errorln("failed to read student_id");
    goto DISCONNECT_EXIT;
  }
  log_infoln("student_id read: %s", student_id);

  // HW5 Problem #2 - Handle notification when button pressed on peripheral.
  ret = handle_notification(gatt_connection);
  if (ret) {
    log_errorln("failed to handle notification");
  }

DISCONNECT_EXIT:
  gattlib_disconnect(gatt_connection);
  return ret;
}