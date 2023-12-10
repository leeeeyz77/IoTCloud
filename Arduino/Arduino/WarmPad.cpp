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

void WarmPad::on_C(){ //on_COLD
  state= W_C;
}

void WarmPad::on_W(){//on_WARM
  state= W_W;
}

void WarmPad::on_H(){//on_HOT
  state= W_H;
}
