#ifndef __TRACKER_POSITION_H__
#define __TRACKER_POSITION_H__

#include "accelero.h"
#include "bug_event.h"
#include "client_bug_event.h"
#include "logger.h"
#include "mbed.h"
#include "noise_gate.h"

#define calc_displacement(a1, a2, t1, t2) \
  ((a2 + a1) * (t2 - t1) * (t2 - t1) / 4)
#define calc_distance(d) std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2])

// #define SAMPLE_RATE 30ms
#define SAMPLE_RATE 3s
#define NOISE_GATE_THRESHOULD 10
#define NOISE_GATE_RELEASE_COUNT 0.5s / SAMPLE_RATE

struct DXYZ {
  time_t value[3];

  inline bool operator>(time_t rhs) { return calc_distance(value) > rhs; }
};

class PositionTracker : Sink<DXYZ> {
  typedef PositionTracker Self;

 public:
  PositionTracker(BugEventClient* client)
      : _client(client),
        _noise_gate(new NoiseGate<DXYZ, time_t>(
            NOISE_GATE_THRESHOULD, NOISE_GATE_RELEASE_COUNT, this)),
        _thread(new rtos::Thread),
        _event_queue(new events::EventQueue),
        _sample_ts(0),
        _event_time_range(TimeRange()),
        _event_position(NULL) {
    _event_queue->call(this, &Self::init_callback);
  }

 protected:
  void start() {
    _event_queue->call_every(SAMPLE_RATE, this, &Self::update_state_callback);
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
    BSP_ACCELERO_AccGetXYZ(_acc_bias);
    BSP_ACCELERO_AccGetXYZ(_acc);
  }

  void update_state_callback() {
    int16_t last_acc[3];
    std::memcpy(last_acc, _acc, 3 * sizeof(int16_t));
    BSP_ACCELERO_AccGetXYZ(_acc);
    log_traceln("acc: {x : %d, y : %d, z : %d}", _acc[0], _acc[1], _acc[2]);

    time_t last_sample_ts = _sample_ts;
    _sample_ts = time(NULL);

    _noise_gate->process(DXYZ{{
        calc_displacement(last_acc[0] - _acc_bias[0], _acc[0] - _acc_bias[0],
                          last_sample_ts, _sample_ts),
        calc_displacement(last_acc[1] - _acc_bias[1], _acc[1] - _acc_bias[1],
                          last_sample_ts, _sample_ts),
        calc_displacement(last_acc[2] - _acc_bias[2], _acc[2] - _acc_bias[2],
                          last_sample_ts, _sample_ts),
    }});
  }

  // give access for NoiseGate for sink implementation
  friend class NoiseGate<DXYZ, time_t>;

  // implement Sink
  virtual void record(DXYZ dxyz) {
    if (_event_position == NULL) {
      _event_time_range.started_at = _sample_ts;
      _event_position = new Position();
      _event_position->from.x = dxyz.value[0];
      _event_position->from.y = dxyz.value[1];
      _event_position->from.z = dxyz.value[2];
    }
    _event_position->to.x += dxyz.value[0];
    _event_position->to.y += dxyz.value[1];
    _event_position->to.z += dxyz.value[2];
  }

  // implement Sink
  virtual void end_of_record() {
    if (_event_position == NULL) {
      log_errorln("unexpected call before record something");
      return;
    }

    // send position event
    _event_time_range.ended_at = _sample_ts;
    _client->send(_event_position, _event_time_range);

    // cleanup
    _event_position = NULL;
    delete _event_position;
  }

 private:
  BugEventClient* _client;
  NoiseGate<DXYZ, time_t>* _noise_gate;
  rtos::Thread* _thread;
  events::EventQueue* _event_queue;
  int16_t _acc_bias[3];
  int16_t _acc[3];
  time_t _sample_ts;
  TimeRange _event_time_range;
  Position* _event_position;
};

#endif
