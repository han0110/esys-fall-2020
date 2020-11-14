#ifndef __SERVICE_BUTTON_H__
#define __SERVICE_BUTTON_H__

#include "logger.h"
#include "mbed.h"

class ButtonService {
  typedef ButtonService Self;

 public:
  ButtonService() : _button(USER_BUTTON) {
    _button.fall(mbed::callback(this, &Self::on_button_pressed));
    _button.rise(mbed::callback(this, &Self::on_button_released));
  }

 private:
  void update_button_state(bool button_pressed_) {
    button_pressed = button_pressed_;
    log_infoln("update_button_state: %s", button_pressed ? "true" : "false")
  }

  void on_button_pressed() { button_pressed = true; }

  void on_button_released() { button_pressed = false; }

 public:
  bool button_pressed;

 private:
  InterruptIn _button;
};

#endif
