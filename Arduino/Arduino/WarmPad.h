#include <Arduino.h>

#define W_C 0 //"COLD"
#define W_W 1 //"WARM"
#define W_H 2 //"HOT"


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
