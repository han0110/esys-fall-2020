#ifndef __SERVICE_ACCELRO_H__
#define __SERVICE_ACCELRO_H__

#include "events/mbed_events.h"
#include "logger.h"
#include "mbed.h"
#include "stm32l475e_iot01_accelero.h"

class AcceleroService {
  typedef AcceleroService Self;

 public:
  AcceleroService() : _thread(new rtos::Thread), _event_queue(new events::EventQueue) {}

  int init() {
    ACCELERO_StatusTypeDef accelero_ret = BSP_ACCELERO_Init();
    if (accelero_ret != ACCELERO_StatusTypeDef::ACCELERO_OK) {
      log_errorln("failed to init accelero, error: %d", accelero_ret);
      return -1;
    }
    return 0;
  }

  void start() {
    _event_queue->call_every(30, this, &Self::update_accelero_state);
    _thread->start(mbed::callback(_event_queue, &events::EventQueue::dispatch_forever));
  }

  void join() { _thread->join(); }

 private:
  void update_accelero_state() {
    BSP_ACCELERO_AccGetXYZ(xyz);
    log_infoln("update_accelero_state, x: %d, y: %d, z: %d", xyz[0], xyz[1],
               xyz[2]);
  }

 public:
  int16_t xyz[3];

 private:
  rtos::Thread* _thread;
  events::EventQueue* _event_queue;
};

#endif