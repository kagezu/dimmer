#include "lcd.h"
#include "pinout.h"
#include "timer.h"
#include "serif_18i.h"
#include "adc.h"
#include "pins.h"
// #include "const.h"
#include "average.h"
#include "encoder.h"

#define AVG_FACTOR 1

ADC adc;
LCD lcd;
int mode = 1;

Pin<PC, 3> CAP_RST;
Encoder enc;


Average<uint16_t, AVG_FACTOR> a1;
Average<uint16_t, AVG_FACTOR> a2;

volatile uint8_t f = 0;
volatile uint16_t t1;
volatile uint16_t t11;
volatile uint16_t t2;

volatile uint16_t v1;
volatile uint16_t v2;

int main(void)
{
  EICRA = 0b1011; // INT1 INT0
  EIMSK = 0b11;

  T0_DIV_64;
  // T0_CTC;
  // OCR0A = 15; // 1ms
  T0_OVF_ON;

  T1_DIV_8;

  CAP_RST.init(GP_Float);
  CAP_RST.clr();

  adc.init(6, ADC_DIV_16);
  adc.start();
  lcd.init();
  lcd.font(serif_18i, 0, 0);
  lcd.color(White);
  lcd.background(MidnightBlue);
  lcd.clear();

  sei();
  delay_ms(30);
  a1 = t1;
  a2 = t2;

  while (true) {
    lcd.printf(P("\fT1: %u      \n"), a1.value >> 2);
    lcd.printf(P("T2: %u      \n"), a2.value >> 2);
    lcd.printf(P("%%: %.2.4q      \n"), ((uint32_t)a1.value * 1600) / a2.value);
    lcd.printf(P("Value: %u %u      \n"), v1, v2);
    lcd.printf(P("t1: %u   \n"), t1);
    lcd.printf(P("t2: %u   \n"), t2);
    lcd.printf(P("Encoder: %c %u       \n"),
      enc.is_push() ? '+' : '-', enc.count);
  }
}

ISR(INT0_vect)
{
  t2 = TCNT1;
  TCNT1 = 0;
  a2 += t2;

  if (f) { v1 = adc.value(); f = 0; }
  else { v2 = adc.value(); f = 1; }

  CAP_RST.init(GPO_Max); // Сброс конденсатора
}

ISR(INT1_vect)
{
  t1 = TCNT1;
  a1 += t1;

  CAP_RST.init(GP_Float);
}

ISR(TIMER0_OVF_vect)
{
  sei();
  enc.scan();
}
