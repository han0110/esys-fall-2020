#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "mbed.h"

#define __logln(level, msg, ...) \
  printf("[" level "](%s:%d): " msg "\r\n", __FILE__, __LINE__, ##__VA_ARGS__);

#define log_traceln(msg, ...) __logln("TRACE", msg, ##__VA_ARGS__);
#define log_debugln(msg, ...) __logln("DEBUG", msg, ##__VA_ARGS__);
#define log_infoln(msg, ...) __logln("INFO", msg, ##__VA_ARGS__);
#define log_warnln(msg, ...) __logln("WARN", msg, ##__VA_ARGS__);
#define log_errorln(msg, ...) __logln("ERROR", msg, ##__VA_ARGS__);

#if defined LOG_LEVEL_DEBUG
#define log_traceln(msg, ...)

#elif defined LOG_LEVEL_INFO
#define log_traceln(msg, ...)
#define log_debugln(msg, ...)

#elif defined LOG_LEVEL_WARN
#define log_traceln(msg, ...)
#define log_debugln(msg, ...)
#define log_infoln(msg, ...)

#elif defined LOG_LEVEL_ERROR
#define log_traceln(msg, ...)
#define log_debugln(msg, ...)
#define log_infoln(msg, ...)
#define log_warnln(msg, ...)

#else
#endif

static BufferedSerial serial_port(USBTX, USBRX, 9600);

FileHandle *mbed::mbed_override_console(int fd) { return &serial_port; }

#endif
