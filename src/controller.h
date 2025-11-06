#pragma once
#include "gpio.h"

#define E_BUFFER 100

#define KEY_ON  0x01  // Ключ открыт
#define CAP_ON  0x02  // Конденсатор заряжен
#define POWER   0x04  // Включено управление мощностью
#define PULSE   0x08  // Режим 1 импульса
#define PAUSE   0x10  // Пауза
#define MODE_X4 0x20  // Точный режим
#define REJECT  0x40  // Аварийный режим

Pin<PB, 1> PWM;
Pin<PC, 3> CAP;

class Controller {
protected:
  volatile int16_t buffer[E_BUFFER];
  volatile int32_t sum;     // Сумма всех значений буфера
  volatile uint8_t ptr;     // Текущая позиция буфера
  volatile int16_t mod;     // Перерасход энергии mJ / полупериод
  volatile int16_t target;  // x1.024 [mJ] на полупериод

  volatile uint8_t status;

public:
  Controller()
  {
    for (u8 i = 0; i < E_BUFFER; i++) buffer[i] = 0;
    ptr = 0;
    mod = 0;
    sum = 0;
    status = 0;

    PWM.init(GPO_Max);
    PWM.clr();
    CAP.init(GP_Float);
    CAP.clr();
  }

  void step(s16 current)
  {
    CAP.init(GPO_Max); // Сброс конденсатора
    status &= ~CAP_ON; // Сброс статуса конденсатора

    if (is_x4()) current >>= 2;
    int16_t energy = (current << 1) + current + (current >> 2); // I * 3.25 [mJ]

    mod += energy - target;
    sum += energy - buffer[ptr];
    buffer[ptr++] = energy;
    if (ptr == E_BUFFER) ptr = 0;
    if (mod < 0) mod = 0;
    if (status & REJECT) return;    // Аварийный режим - безусловная блокировка
    if (status & PULSE) goto pulse0;// Одиночный импульс
    if (status & PAUSE) return;     // Пауза
    if (!(status & POWER)) return;  // Если выключено
    if (mod >= target) return;      // Накопленная энергия выше среднего

  pulse0:
    PWM.set();  // Открыть симистор
    status = (status & ~PULSE) | KEY_ON;  // Сбросить флаг импульса и открыть симистор
  }

  void release()
  {
    if (is_open()) {
      CAP.init(GP_Float); // Начать накопление, если симистор открыт
      status = (status & ~KEY_ON) | CAP_ON;
    }
    delay_ms(1); // 10%
    PWM.clr();   // Отключить открывающий ток
  }

  bool is_open() { return status & KEY_ON; }
  bool is_cap() { return status & CAP_ON; }
  bool is_on() { return status & POWER; }
  bool is_stop() { return status & REJECT; }
  bool is_x4() { return status & MODE_X4; }

  void stop() { status |= REJECT; }
  void pulse() { status |= PULSE; }
  void on() { status |= POWER; }
  void off() { status &= ~POWER; target = 0; }
  void pause() { status |= PAUSE; }
  void mode_x4(bool mode = true) { status = (status & ~MODE_X4) | (mode ? MODE_X4 : 0); }

  u16 get_power() { return sum >> 10; } // /1024

  void set_power(s16 power)
  {
    if (power > 0) target = power * 10 + (power >> 2);  // x10.25
    else target = 0;
  }
};
