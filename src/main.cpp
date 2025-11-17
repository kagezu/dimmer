// #include "display.h"
#include "timer.h"
#include "adc.h"
#include "average.h"
#include "encoder.h"
#include "print.h"
#include "controller.h"
#include "current.h"
#include "system_5x7.h"

#define max(a,b) ((a)>(b)?(a):(b))

ADC adc;
Print out;
// Display<SSD1306> lcd;

Pin<PD, 4> USER;

Encoder enc;
Controller ctrl;

///////////////////////////////////////////////////////////////////////////////

volatile int16_t digit = 1;
volatile int16_t power = 0;
volatile int16_t encode = 0;

///////////////////////////////////////////////////////////////////////////////

#define MODE_1  0
#define MODE_4  1
#define MODE_C  2

volatile int8_t mode = 0;
volatile int8_t tick = 0;
volatile int16_t max_value = 0;

void print_mode()
{
  switch (mode) {
    case MODE_1:
      out.printf(P("\fP.="));
      break;
    case MODE_4:
      out.printf(P("\fP,<"));
      break;
    case MODE_C:
      out.printf(P("\fC@="));
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////

int main(void)
{
  USER.init(GP_Float);

  EICRA = 0b1011; // INT1 INT0
  EIMSK = 0b11;

  T0_DIV_64;
  T0_OVF_ON;

  delay_ms(100);
  adc.init(6, ADC_DIV_16);
  adc.start();

  sei();

  while (true) {
    if (ctrl.is_stop()) break;
    if (ctrl.is_on()) {
      if (encode) out.number(power);
      else {
        if (mode == MODE_C) out.number(temperature(max_value));
        else out.number(ctrl.get_power());
      }
    }
    else print_mode();

    char i = 100;
    while (i-- && !encode) delay_ms(5);
  }

  out.printf(P("---"));
  while (true);
}

ISR(INT0_vect)
{
  int16_t value = adc.value();    // 0 - 1023
  if (value >= 1000) ctrl.stop(); // Блокировка > 7A
  value = current(value);
  if (mode == MODE_C) {
    if (max_value < value) max_value = value;
    if (power <= temperature(max_value)) {
      ctrl.set_power((power >> 2) - 10);
      max_value--;
      if (max_value < 0) max_value = 0;
    }
  }

  ctrl.step(value);
}

ISR(INT1_vect)
{
  ctrl.release();
  ctrl.set_power(power);
}

volatile int8_t key = 0;

ISR(TIMER0_OVF_vect)
{
  if (USER.get()) {
    key = 1;
    if (ctrl.is_stop()) asm volatile("jmp 0");
  }
  else if (key) {
    key = 0;
    if (ctrl.is_on()) ctrl.off();
    else ctrl.on();
  }

  if (enc.is_push())  digit *= 10;
  if (digit == 1000) digit = 1;

  int16_t inc = enc.scan();
  if (ctrl.is_on()) {
    power += inc * digit;
    if (power < 0) power = 0;
    if (power > 210 && mode == MODE_C) power = 210;
    else if (power > 1000) power = 1000;
  }
  else if (inc) {
    mode += inc;
    if (mode < MODE_1) mode = MODE_C;
    if (mode > MODE_C) mode = MODE_1;
    if (mode == MODE_4) ctrl.mode_x4();
    else ctrl.mode_x4(false);
  }

  out.view();

  if (inc) encode = 1000;
  if (encode) encode--;
}
