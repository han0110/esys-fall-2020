#ifndef __NS_LOGGER_H__
#define __NS_LOGGER_H__

#include "logger.h"
#include "nsapi_types.h"

inline void ns_log_error(nsapi_error_t error, const char* msg) {
  log_error("%s: ", msg);
  switch (error) {
    case NSAPI_ERROR_WOULD_BLOCK:
      printf(
          "NSAPI_ERROR_WOULD_BLOCK: no data is not available but call is "
          "non-blocking");
      break;
    case NSAPI_ERROR_UNSUPPORTED:
      printf("NSAPI_ERROR_UNSUPPORTED: unsupported functionality");
      break;
    case NSAPI_ERROR_PARAMETER:
      printf("NSAPI_ERROR_PARAMETER: invalid configuration");
      break;
    case NSAPI_ERROR_NO_CONNECTION:
      printf("NSAPI_ERROR_NO_CONNECTION: not connected to a network");
      break;
    case NSAPI_ERROR_NO_SOCKET:
      printf("NSAPI_ERROR_NO_SOCKET: socket not available for use");
      break;
    case NSAPI_ERROR_NO_ADDRESS:
      printf("NSAPI_ERROR_NO_ADDRESS: ip address is not known");
      break;
    case NSAPI_ERROR_NO_MEMORY:
      printf("NSAPI_ERROR_NO_MEMORY: memory resource not available");
      break;
    case NSAPI_ERROR_NO_SSID:
      printf("NSAPI_ERROR_NO_SSID: ssid not found");
      break;
    case NSAPI_ERROR_DNS_FAILURE:
      printf("NSAPI_ERROR_DNS_FAILURE: dns failed to complete successfully");
      break;
    case NSAPI_ERROR_DHCP_FAILURE:
      printf("NSAPI_ERROR_DHCP_FAILURE: dhcp failed to complete successfully");
      break;
    case NSAPI_ERROR_AUTH_FAILURE:
      printf("NSAPI_ERROR_AUTH_FAILURE: connection to access point failed");
      break;
    case NSAPI_ERROR_DEVICE_ERROR:
      printf(
          "NSAPI_ERROR_DEVICE_ERROR: failure interfacing with the network "
          "processor");
      break;
    case NSAPI_ERROR_IN_PROGRESS:
      printf("NSAPI_ERROR_IN_PROGRESS: operation (eg connect) in progress");
      break;
    case NSAPI_ERROR_ALREADY:
      printf("NSAPI_ERROR_ALREADY: operation (eg connect) already in progress");
      break;
    case NSAPI_ERROR_IS_CONNECTED:
      printf("NSAPI_ERROR_IS_CONNECTED: socket is already connected");
      break;
    case NSAPI_ERROR_CONNECTION_LOST:
      printf("NSAPI_ERROR_CONNECTION_LOST: connection lost");
      break;
    case NSAPI_ERROR_CONNECTION_TIMEOUT:
      printf("NSAPI_ERROR_CONNECTION_TIMEOUT: connection timed out");
      break;
    case NSAPI_ERROR_ADDRESS_IN_USE:
      printf("NSAPI_ERROR_ADDRESS_IN_USE: address already in use");
      break;
    case NSAPI_ERROR_TIMEOUT:
      printf("NSAPI_ERROR_TIMEOUT: operation timed out");
      break;
    case NSAPI_ERROR_BUSY:
      printf(
          "NSAPI_ERROR_BUSY: device is busy and cannot accept new operation");
      break;
    default:
      printf("%d: unknown error", error);
  }
  printf("\r\n");
}

#endif
