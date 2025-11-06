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

volatile int8_t block = 0;
volatile int16_t digit = 1;
volatile int16_t power = 0;
volatile uint16_t raw = 0;

volatile int16_t encode = 0;

///////////////////////////////////////////////////////////////////////////////


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
  // ctrl.on();

  sei();

  while (true) {
    if (ctrl.is_stop()) break;
    if (ctrl.is_on()) {
      if (encode) out.number(power);
      else out.number(ctrl.get_power());
    }
    else {
      if (ctrl.is_x4()) out.printf(P("\f4 <"));
      else out.printf(P("\f1 ="));
    }
    char i = 100;
    while (i-- && !encode) delay_ms(5);
  }

  out.printf(P("---"));
  while (true);
}

ISR(INT0_vect)
{
  uint16_t value = adc.value(); // 0 - 1023
  if (value > 1000) ctrl.stop();// Блокировка
  value = current(value);
  if (ctrl.is_x4()) value >>= 2;
  else if (value > MAX_CURRENT) ctrl.stop();  // Блокировка
  ctrl.step(value);
}

ISR(INT1_vect)
{
  ctrl.release();
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
    if (power > 1000) power = 1000;
    ctrl.set_power(power);
  }
  else if (inc) ctrl.mode_x4(!ctrl.is_x4());

  out.view();

  if (inc) encode = 1000;
  if (encode) encode--;
}
