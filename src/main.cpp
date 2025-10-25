#include "lcd.h"
#include "pinout.h"
#include "timer.h"
#include "serif_18i.h"
#include "adc.h"
#include "pins.h"
// #include "const.h"
#include "average.h"
#include "encoder.h"

#define max(a,b) ((a)>(b)?(a):(b))

ADC adc;
LCD lcd;

Pin<PB, 1> PWM;
Pin<PC, 3> CAP;
Encoder enc;

volatile uint8_t f;
volatile uint16_t v1;
volatile uint16_t v2;

Average<int32_t, 7> e_avg;

///////////////////////////////////////////////////////////////////////////////

volatile int16_t energy_sum = 0;
volatile int16_t energy_target = 0;
volatile int16_t energy_last = 0;

///////////////////////////////////////////////////////////////////////////////

#define CUR_K1    7.384f
#define CUR_K2    8.1804f
#define CUR_MIN   11

volatile uint8_t reset = 0;

uint16_t current(uint16_t arg)
{
  uint16_t result;
  if (arg == 0) return 0;
  if (arg > CUR_MIN) result = 59 + arg * CUR_K1;
  else result = 50 + arg * CUR_K2;
  return result;
}

int main(void)
{
  EICRA = 0b1011; // INT1 INT0
  EIMSK = 0b11;

  T0_DIV_64;
  T0_OVF_ON;

  PWM.init(GPO_Max);
  PWM.clr();
  CAP.init(GP_Float);
  CAP.clr();

  enc.count = 0;

  adc.init(6, ADC_DIV_16);
  adc.start();
  lcd.init();
  lcd.font(serif_18i, 0, 0);
  lcd.color(White);
  lcd.background(MidnightBlue);
  lcd.clear();

  sei();

  while (true) {
    lcd.printf(P("\fCurrent: %u      \n"), max(v1, v2));
    lcd.printf(P("Power target: %u      \n"), enc.count >> 2);
    lcd.printf(P("Energy: %u      \n"), (uint16_t)e_avg / 10);
    lcd.printf(P("\nEncoder: %c %u       \n"),
      enc.is_push() ? '+' : '-', enc.count >> 2);
  }
}

ISR(INT0_vect)
{
  uint16_t value = current(adc.value());
  CAP.init(GPO_Max); // Сброс конденсатора
  if (f) { v1 = value; f = 0; }
  else { v2 = value; f = 1; }

  energy_last = (value << 1) + (value >> 2); // I * 225V / 100 = I * 2.25 mJ
  energy_last >>= 3;
  energy_sum += energy_last - energy_target; // Перерасход энергии mJ / полупериод

  if (energy_sum < 0) energy_sum = 0;
  if (energy_sum < energy_target) PWM.set(); // Открыть симистор

  e_avg += energy_last;
}

ISR(INT1_vect)
{
  CAP.init(GP_Float); // Начать накопление
  PWM.clr(); // Отключить открывающий ток
}

ISR(TIMER0_OVF_vect)
{
  enc.scan();
  if (enc.count < 0) enc.count = 0;
  if (enc.count > 3999) enc.count = 3999;
  energy_target = (enc.count * 10) >> 2; // mJ на полупериод
}
