#include "timer.h"
#include "adc.h"
#include "spi.h"
#include "average.h"
#include "encoder.h"
#include "accel.h"
#include "controller.h"
#include "system_5x7.h"

#define max(a,b) ((a)>(b)?(a):(b))
#define MAX_CURRENT 5000

ADC adc;
SPI spi;

Port<PC, 0b111> DIGIT;
Pin<PD, 4> USER;

Encoder enc;
Controller ctrl;

///////////////////////////////////////////////////////////////////////////////

volatile int8_t push = 0;
volatile int8_t block = 0;
volatile int16_t digit = 1;
volatile int16_t power = 0;

volatile int16_t encode = 0;

///////////////////////////////////////////////////////////////////////////////



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
  0b0111111,  // -
};

volatile uint8_t out[3] = { 11,11,11 };
volatile uint8_t out_i = 0;

void print(uint16_t n)
{
  uint8_t d2 = n / 100;
  uint8_t d1 = n / 10 - d2 * 10;
  out[0] = number[n % 10];
  if (d2 == 0) {
    out[2] = number[11];
    if (d1 == 0)out[1] = number[11]; else out[1] = number[d1];
  }
  else { out[2] = number[d2]; out[1] = number[d1]; }

  if (USER.get()) out[2] = number[12];
}

///////////////////////////////////////////////////////////////////////////////

#define CUR_K1    7.384f
#define CUR_K2    8.1804f
#define CUR_K3    10.93f
#define CUR_MIN   11

uint16_t current(uint16_t arg)
{
  uint16_t result;
  if (arg == 0) return 0;
  // if (arg > CUR_MIN) result = 59 + arg * CUR_K1;
  // else result = 50 + arg * CUR_K2;
  if (arg < 200) result = 60 + arg * 9.39f;
  else result = arg * CUR_K3 - 188;
  return result;
}

///////////////////////////////////////////////////////////////////////////////

int main(void)
{
  USER.init(GP_Float);
  DIGIT.init(GPO_Max);
  DIGIT.set(0b111);

  EICRA = 0b1011; // INT1 INT0
  EIMSK = 0b11;

  T0_DIV_64;
  T0_OVF_ON;

  delay_ms(100);
  adc.init(6, ADC_DIV_16);
  adc.start();
  spi.fq(2000);
  spi.begin();

  sei();
  ctrl.on();

  while (true) {
    if (ctrl.is_stop()) break;
    if (encode) print(power);
    else print(ctrl.get_power());
    char i = 100;
    while (i-- && !encode) delay_ms(5);
  }

  out[0] = out[1] = out[2] = 63;
  while (true);
}

ISR(INT0_vect)
{
  uint16_t value = adc.value();
  value = current(value);

  if (value > MAX_CURRENT) { // Блокировка
    ctrl.stop();
    out[2] = 63;
    out[1] = 63;
    out[0] = 63;
  }

  ctrl.step(value);
}

ISR(INT1_vect)
{
  ctrl.release();
}

ISR(TIMER0_OVF_vect)
{
  if (USER.get()) asm volatile("jmp 0");

  if (enc.is_push()) push = 1;
  else if (push) { push = 0; digit *= 10; }
  if (digit == 1000) digit = 1;

  int16_t inc = enc.scan();
  power += inc * digit;
  if (power < 0) power = 0;
  if (power > 1000) power = 1000;
  ctrl.set_power(power);

  if (out_i++ == 2) out_i = 0;
  DIGIT.set(0b111);
  spi.send(out[out_i]);
  spi.wait();
  DIGIT.clr(1 << out_i);

  if (inc) encode = 1000;
  if (encode) encode--;
}
