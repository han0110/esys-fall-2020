#ifndef __SERVICE_BUTTON_H__
#define __SERVICE_BUTTON_H__

#include "mbed.h"

class ButtonService {
  typedef ButtonService Self;

 public:
  ButtonService() : _button(USER_BUTTON), button_pressed(false) {
    _button.fall(mbed::callback(this, &Self::on_button_pressed));
    _button.rise(mbed::callback(this, &Self::on_button_released));
  }

 private:
  void on_button_pressed() { button_pressed = true; }

  void on_button_released() { button_pressed = false; }

 public:
  bool button_pressed;

 private:
  InterruptIn _button;
};

#endif
