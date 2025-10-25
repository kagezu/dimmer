#include "timer.h"
#include "adc.h"
#include "spi.h"
#include "average.h"
#include "encoder.h"
#include "accel.h"

#define max(a,b) ((a)>(b)?(a):(b))
#define E_HISTORY 100
#define MAX_CURRENT 6000

ADC adc;
SPI spi;

Pin<PB, 1> PWM;
Pin<PC, 3> CAP;

Port<PC, 0b111> DIGIT;
Pin<PD, 4> USER;

Encoder enc;

///////////////////////////////////////////////////////////////////////////////

uint8_t number[] = {
  0b1000000,  // 0
  0b1111001,  // 1
  0b0100100,  // 2
  0b0110000,  // 3
  0b0011001,  // 4
  0b0010010,  // 5
  0b0000010,  // 6
  0b1111000,  // 7
  0b0000000,  // 8
  0b0010000,  // 9
  0b0001000,  // A
  0b1111111,  // 
};

volatile uint8_t out[3] = { 11,11,11 };
volatile uint8_t out_i = 0;

void print(uint16_t n)
{
  uint8_t d2 = n / 100;
  uint8_t d1 = n / 10 - d2 * 10;
  out[0] = n % 10;
  if (d2 == 0) {
    out[2] = 11;
    if (d1 == 0)out[1] = 11; else out[1] = d1;
  }
  else { out[2] = d2; out[1] = d1; }
  // if (!n) out[0] = 11;
}

///////////////////////////////////////////////////////////////////////////////

volatile int16_t energy_mod = 0;
volatile int16_t energy_target = 0;
volatile int32_t energy_sum = 0;
volatile uint16_t energy = 0;

int16_t energy_last[E_HISTORY] = {};
int8_t index = 0;

volatile int8_t push = 0;
volatile int16_t digit = 1;
volatile int16_t power = 0;

volatile int16_t delay = 0;

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

///////////////////////////////////////////////////////////////////////////////

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
  USER.init(GP_Float);
  DIGIT.init(GPO_Max);
  DIGIT.set(0b111);

  delay_ms(100);
  adc.init(6, ADC_DIV_16);
  adc.start();
  spi.fq(2000);
  spi.begin();

  sei();

  while (true) {
    // lcd.printf(P("Power: %u      \n"), power);
    // lcd.printf(P("Energy x10: %u      \n"), energy_sum / E_HISTORY);
    if (delay)print(power);
    else print(energy_sum / E_HISTORY / 10);
    char i = 100;
    while (i-- && !delay) delay_ms(5);
  }
}

ISR(INT0_vect)
{
  uint16_t value = current(adc.value());
  // if (value > MAX_CURRENT) { power = 0; energy_target = 0; } // Ограничитель тока
  CAP.init(GPO_Max);                          // Сброс конденсатора

  energy = (value << 1) + (value >> 2);  // I * 225V / 100 = I * 2.25 mJ
  energy_mod += energy - energy_target;       // Перерасход энергии mJ / полупериод

  if (energy_mod < 0) energy_mod = 0;
  if (energy_mod < energy_target) PWM.set();  // Открыть симистор

  energy_sum += energy - energy_last[index];
  energy_last[index++] = energy;
  if (index == E_HISTORY) index = 0;
}

ISR(INT1_vect)
{
  CAP.init(GP_Float); // Начать накопление
  PWM.clr();          // Отключить открывающий ток
}

ISR(TIMER0_OVF_vect)
{
  // if (USER.get()) power = 0;

  if (enc.is_push()) push = 1;
  else if (push) { push = 0; digit *= 10; }
  if (digit == 1000) digit = 1;

  int16_t inc = enc.scan();
  power += inc * digit;
  if (power < 0) power = 0;
  if (power > 1000) power = 1000;
  energy_target = power * 10; // mJ на полупериод

  if (out_i++ == 2) out_i = 0;
  DIGIT.set(0b111);
  spi.send(number[out[out_i]]);
  spi.wait();
  DIGIT.clr(1 << out_i);

  if (inc) delay = 1000;
  if (delay) delay--;
}
