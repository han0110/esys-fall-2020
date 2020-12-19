#ifndef __NS_LOGGER_H__
#define __NS_LOGGER_H__

#include "logger.h"
#include "nsapi_types.h"
#include "stdarg.h"

inline void ns_log_errorln(nsapi_error_t error, const char* msg, ...) {
  char buffer[256];
  va_list args;
  va_start(args, msg);
  vsprintf(buffer, msg, args);
  log_errorln("%s", buffer);
  va_end(args);

#if defined(LOG_LEVEL_DEBUG) || defined(LOG_LEVEL_TRACE)
  switch (error) {
    case NSAPI_ERROR_WOULD_BLOCK:
      log_errorln(
          "NSAPI_ERROR_WOULD_BLOCK: no data is not available but call is "
          "non-blocking");
      break;
    case NSAPI_ERROR_UNSUPPORTED:
      log_errorln("NSAPI_ERROR_UNSUPPORTED: unsupported functionality");
      break;
    case NSAPI_ERROR_PARAMETER:
      log_errorln("NSAPI_ERROR_PARAMETER: invalid configuration");
      break;
    case NSAPI_ERROR_NO_CONNECTION:
      log_errorln("NSAPI_ERROR_NO_CONNECTION: not connected to a network");
      break;
    case NSAPI_ERROR_NO_SOCKET:
      log_errorln("NSAPI_ERROR_NO_SOCKET: socket not available for use");
      break;
    case NSAPI_ERROR_NO_ADDRESS:
      log_errorln("NSAPI_ERROR_NO_ADDRESS: ip address is not known");
      break;
    case NSAPI_ERROR_NO_MEMORY:
      log_errorln("NSAPI_ERROR_NO_MEMORY: memory resource not available");
      break;
    case NSAPI_ERROR_NO_SSID:
      log_errorln("NSAPI_ERROR_NO_SSID: ssid not found");
      break;
    case NSAPI_ERROR_DNS_FAILURE:
      log_errorln(
          "NSAPI_ERROR_DNS_FAILURE: dns failed to complete successfully");
      break;
    case NSAPI_ERROR_DHCP_FAILURE:
      log_errorln(
          "NSAPI_ERROR_DHCP_FAILURE: dhcp failed to complete successfully");
      break;
    case NSAPI_ERROR_AUTH_FAILURE:
      log_errorln(
          "NSAPI_ERROR_AUTH_FAILURE: connection to access point failed");
      break;
    case NSAPI_ERROR_DEVICE_ERROR:
      log_errorln(
          "NSAPI_ERROR_DEVICE_ERROR: failure interfacing with the network "
          "processor");
      break;
    case NSAPI_ERROR_IN_PROGRESS:
      log_errorln(
          "NSAPI_ERROR_IN_PROGRESS: operation (eg connect) in progress");
      break;
    case NSAPI_ERROR_ALREADY:
      log_errorln(
          "NSAPI_ERROR_ALREADY: operation (eg connect) already in progress");
      break;
    case NSAPI_ERROR_IS_CONNECTED:
      log_errorln("NSAPI_ERROR_IS_CONNECTED: socket is already connected");
      break;
    case NSAPI_ERROR_CONNECTION_LOST:
      log_errorln("NSAPI_ERROR_CONNECTION_LOST: connection lost");
      break;
    case NSAPI_ERROR_CONNECTION_TIMEOUT:
      log_errorln("NSAPI_ERROR_CONNECTION_TIMEOUT: connection timed out");
      break;
    case NSAPI_ERROR_ADDRESS_IN_USE:
      log_errorln("NSAPI_ERROR_ADDRESS_IN_USE: address already in use");
      break;
    case NSAPI_ERROR_TIMEOUT:
      log_errorln("NSAPI_ERROR_TIMEOUT: operation timed out");
      break;
    case NSAPI_ERROR_BUSY:
      log_errorln(
          "NSAPI_ERROR_BUSY: device is busy and cannot accept new operation");
      break;
    default:
      log_errorln("%d: unknown error", error);
  }
#endif
}

#endif
