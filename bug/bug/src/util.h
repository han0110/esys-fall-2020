#ifndef __UTIL_H__
#define __UTIL_H__

#define try_until_success(f, retry_duration)     \
  while (!f) {                                   \
    rtos::ThisThread::sleep_for(retry_duration); \
  }

#endif
