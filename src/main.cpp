#include "lcd.h"
#include "pinout.h"
#include "timer.h"
#include "serif_18i.h"
#include "adc.h"
#include "pins.h"

ADC adc;
LCD lcd;
int mode = 1;

int main(void)
{
  // T1_DIV_1024;
  T1_DIV_1;
  TCNT1 = 0;
  ADC3.init(GP_Float);

  adc.init(3);
  lcd.init();
  lcd.font(serif_18i, 0, 0);
  lcd.color(White);
  lcd.background(MidnightBlue);
  lcd.clear();

  while (true) {
    TCNT1 = 0;
    adc.single();
    adc.wait();
    u16 value = adc.value();
    lcd.printf(P("\fTick: %u   \n"), TCNT1);
    lcd.printf(P("Value: %u   \n"), value);
  }
}
