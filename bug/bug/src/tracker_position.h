#ifndef __TRACKER_POSITION_H__
#define __TRACKER_POSITION_H__

#include "accelero.h"
#include "logger.h"
#include "mbed.h"

#define UPDATE_STATE_INTERVAL 30ms

class PositionTracker {
  typedef PositionTracker Self;

 public:
  PositionTracker(BugEventClient* client)
      : _client(client),
        _thread(new rtos::Thread),
        _event_queue(new events::EventQueue),
        _xyz{0} {
    _event_queue->call(this, &Self::init_callback);
  }

 protected:
  void start() {
    _event_queue->call_every(UPDATE_STATE_INTERVAL, this,
                             &Self::update_state_callback);
    _thread->start(
        mbed::callback(_event_queue, &events::EventQueue::dispatch_forever));
  }

  void join() { _thread->join(); }

 private:
  void init_callback() {
    auto accelero_ret = BSP_ACCELERO_Init();
    if (accelero_ret != ACCELERO_StatusTypeDef::ACCELERO_OK) {
      log_errorln("failed to init accelero, error: %d", accelero_ret);
    }
  }

  void update_state_callback() { BSP_ACCELERO_AccGetXYZ(_xyz); }

 private:
  BugEventClient* _client;
  rtos::Thread* _thread;
  events::EventQueue* _event_queue;
  int16_t _xyz[3];
};

#endif
