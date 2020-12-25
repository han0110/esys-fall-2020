#ifndef __NOISE_GATE_H__
#define __NOISE_GATE_H__

template <typename T>
class Sink {
 public:
  virtual void record(T) = 0;
  virtual void end_of_record() = 0;
};

template <typename T, typename TT = T>
class NoiseGate {
  typedef NoiseGate Self;

 public:
  enum State : uint { Opened = 0, Closing = 1 << 0, Closed = 1 << 1 };

  NoiseGate(TT threshold, uint16_t release_count, Sink<T>* sink)
      : _state(Closed),
        _remaining_release_count(0),
        _threshold(threshold),
        _release_count(release_count),
        _sink(sink) {}

  void process(T value) {
    bool was_open = is_opened();

    next_state(value);
    if (is_opened()) {
      _sink->record(value);
    } else if (was_open) {
      _sink->end_of_record();
    }
  }

  bool is_opened() { return _state & (Opened | Closing); }

 private:
  void next_state(T value) {
    bool over_threshould = value > _threshold;

    if (_state & Opened) {
      if (!over_threshould) {
        _state = Closing;
        _remaining_release_count = _release_count;
      }
    } else if (_state & Closing) {
      if (over_threshould) {
        _state = Opened;
      } else {
        if (_remaining_release_count == 0) {
          _state = Closed;
        } else {
          _remaining_release_count -= 1;
        }
      }
    } else if (_state & Closed) {
      if (over_threshould) {
        _state = Opened;
      }
    }
  }

 private:
  State _state;
  uint16_t _remaining_release_count;
  TT _threshold;
  const uint16_t _release_count;
  Sink<T>* _sink;
};

#endif
