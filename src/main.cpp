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
#define AVG_FACTOR 5

ADC adc;
LCD lcd;

Pin<PB, 1> PWM;
Pin<PC, 3> CAP_RST;
Encoder enc;


Average<uint16_t, AVG_FACTOR> a1;
Average<uint32_t, AVG_FACTOR> a2;

volatile uint8_t f = 0;

volatile uint16_t t1;
volatile uint16_t t2;

volatile uint16_t v1;
volatile uint16_t v2;

#define CUR_A     2.714f
#define CUR_R     7.384f
#define CUR_AR    2.7268f
#define CUR_A_MIN 30
#define CUR_R_MIN 11

volatile uint8_t reset = 0;

uint16_t current(uint16_t arg)
{
  if (arg == 0) return 0;
  if (reset) {
    if (arg > CUR_R_MIN) return 140 + (arg - CUR_R_MIN) * CUR_R;
    return 50 + arg * 3 * CUR_AR;
  }
  else {
    if (arg > CUR_A_MIN) return 140 + (arg - CUR_A_MIN) * CUR_A;
    return 47 + arg * 3;
  }
}

int main(void)
{
  EICRA = 0b1011; // INT1 INT0
  EIMSK = 0b11;

  T0_DIV_64;
  T0_OVF_ON;

  T1_DIV_8;
  T1_FAST_PWM_CUSTOM;
  T1_OC1A_PWM_INV;
  T1_COMPA_ON;
  ICR1 = -1;
  OCR1A = 20000;
  PWM.init(GPO_Max);
  enc.count = 199;

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
    lcd.printf(P("\fT1: %u      \n"), a1.value >> 6);
    lcd.printf(P("T2: %u      \n"), (u32)(a2.value >> 6));
    lcd.printf(P("Value: %u      \n"), max(v1, v2));
    lcd.printf(P("Current: %u      \n"), current(max(v1, v2)));
    lcd.printf(P("\nEncoder: %c %u       \n"),
      enc.is_push() ? '+' : '-', enc.count);
  }
}

ISR(INT0_vect)
{
  t2 = TCNT1;
  TCNT1 = -1;
  a2 += t2;

  if (f) { v1 = adc.value(); f = 0; }
  else { v2 = adc.value(); f = 1; }

  if (reset) CAP_RST.init(GPO_Max); // Сброс конденсатора
  OCR1A = enc.count * 100;
}

ISR(INT1_vect)
{
  t1 = TCNT1;
  TCNT1 = t1 >> 1;
  a1 += t1;

}

ISR(TIMER1_COMPA_vect)
{
  delay_us(10);
  CAP_RST.init(GP_Float);
}

ISR(TIMER0_OVF_vect)
{
  sei();
  enc.scan();
  if (enc.is_push()) reset = 1;
  else reset = 0;
}
