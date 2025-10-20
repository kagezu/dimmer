#include "lcd.h"
#include "pinout.h"
#include "timer.h"
#include "serif_18i.h"
#include "adc.h"
#include "pins.h"

ADC adc;
LCD lcd;
int mode = 1;

volatile uint16_t t1 = 0;
volatile uint16_t t2 = 0;

uint32_t a1 = 0;
uint32_t a2 = 0;

int main(void)
{
  EICRA = 0b1011; // INT1 INT0
  EIMSK = 0b11;

  // T1_DIV_1024;
  T1_DIV_8;
  // TCNT1 = 0;
  ADC3.init(GP_Float);

  adc.init(3, ADC_DIV_16);
  adc.start();
  lcd.init();
  lcd.font(serif_18i, 0, 0);
  lcd.color(White);
  lcd.background(MidnightBlue);
  lcd.clear();

  sei();
  delay_ms(30);
  a1 = (uint32_t)t1 << 3;
  a2 = (uint32_t)t2 << 3;

  while (true) {
    u16 value = adc.value();

    a1 = a1 - (a1 >> 3) + t1;
    a2 = a2 - (a2 >> 3) + t2;

    lcd.printf(P("\fT1: %u      \n"), a1 >> 4);
    lcd.printf(P("T2: %u      \n"), a2 >> 4);
    lcd.printf(P("%%: %.2.4q      \n"), ((uint32_t)a1 * 1600) / a2);
    lcd.printf(P("Value: %u      \n"), value);
  }
}

ISR(INT0_vect)
{
  t2 = TCNT1;
  TCNT1 = 0;
}
ISR(INT1_vect)
{
  t1 = TCNT1;
}
