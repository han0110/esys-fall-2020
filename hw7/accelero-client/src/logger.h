#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "mbed.h"

#define __log(level, x, ...) \
  printf("[" level "](%s:%d): " x, __FILE__, __LINE__, ##__VA_ARGS__);
#define __logln(level, x, ...) \
  printf("[" level "](%s:%d): " x "\r\n", __FILE__, __LINE__, ##__VA_ARGS__);

#define log_trace(x, ...) __log("TRACE", x, ##__VA_ARGS__);
#define log_debug(x, ...) __log("DEBUG", x, ##__VA_ARGS__);
#define log_info(x, ...) __log("INFO", x, ##__VA_ARGS__);
#define log_warn(x, ...) __log("WARN", x, ##__VA_ARGS__);
#define log_error(x, ...) __log("ERROR", x, ##__VA_ARGS__);

#define log_traceln(x, ...) __logln("TRACE", x, ##__VA_ARGS__);
#define log_debugln(x, ...) __logln("DEBUG", x, ##__VA_ARGS__);
#define log_infoln(x, ...) __logln("INFO", x, ##__VA_ARGS__);
#define log_warnln(x, ...) __logln("WARN", x, ##__VA_ARGS__);
#define log_errorln(x, ...) __logln("ERROR", x, ##__VA_ARGS__);

#if defined LOG_LEVEL_DEBUG
#define log_trace(x, ...)
#define log_traceln(x, ...)

#elif defined LOG_LEVEL_INFO
#define log_trace(x, ...)
#define log_traceln(x, ...)
#define log_debug(x, ...)
#define log_debugln(x, ...)

#elif defined LOG_LEVEL_WARN
#define log_trace(x, ...)
#define log_traceln(x, ...)
#define log_debug(x, ...)
#define log_debugln(x, ...)
#define log_info(x, ...)
#define log_infoln(x, ...)

#elif defined LOG_LEVEL_ERROR
#define log_trace(x, ...)
#define log_traceln(x, ...)
#define log_debug(x, ...)
#define log_debugln(x, ...)
#define log_info(x, ...)
#define log_infoln(x, ...)
#define log_warn(x, ...)
#define log_warnln(x, ...)

#else
#endif

static BufferedSerial serial_port(USBTX, USBRX, 9600);

FileHandle *mbed::mbed_override_console(int fd) { return &serial_port; }

#endif
