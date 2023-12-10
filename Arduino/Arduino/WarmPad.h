#include <Arduino.h>

#define W_C 0
#define W_W 1
#define W_H 2


class WarmPad{
  private:
    int state;

  public:
    void init();
    int getState();
    void on_C();
    void on_W();
    void on_H();
};
