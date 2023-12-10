#include "WarmPad.h"

void WarmPad::init() {
  // Always try to avoid duplicate code.
  // Instead of writing digitalWrite(pin, LOW) here,
  // call the function off() which already does that
  on_W();
  state = W_H;
}

int WarmPad::getState() {
  return state;
}

void WarmPad::on_C(){
  state= W_C;
}

void WarmPad::on_W(){
  state= W_W;
}

void WarmPad::on_H(){
  state= W_H;
}
