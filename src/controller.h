#pragma once
#include "gpio.h"

#define E_BUFFER 100

#define KEY_ON  0x01  // Ключ открыт
#define CAP_ON  0x02  // Конденсатор заряжен
#define POWER   0x04  // Включено управление мощностью
#define PULSE   0x08  // Режим 1 импульса
#define PAUSE   0x10  // Пауза
#define REJECT  0x20  // Аварийный режим

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
    CAP.init(GPO_Max);  // Сброс конденсатора
    status &= ~(CAP_ON | KEY_ON); // Сброс статуса симистора и конденсатора

    // int16_t energy = (current << 1) + (current >> 2); // I * 225V / 100 [mJ]
    int16_t energy = (current << 1) + (current >> 2) + (current >> 5); // I * 2.23 * 1.024 [mJ]
    mod += energy - target;
    sum += energy - buffer[ptr];
    buffer[ptr++] = energy;
    if (ptr == E_BUFFER) ptr = 0;
    if (mod < 0) mod = 0;
    if (status & REJECT) return;    // Аварийный режим - безусловная блокировка
    if (status & PULSE) goto pulse; // Одиночный импульс
    if (status & PAUSE) return;     // Пауза
    if (!(status & POWER)) return;  // Если выключено
    if (mod >= target) return;      // Накопленная энергия выше среднего

  pulse:
    PWM.set();  // Открыть симистор
    status = (status & ~PULSE) | KEY_ON;  // Сбросить флаг импульса и открыть симистор
  }

  void release()
  {
    PWM.clr(); // Отключить открывающий ток
    if (status & KEY_ON) {
      CAP.init(GP_Float); // Начать накопление, если симистор открыт
      status |= CAP_ON;
    }
  }

  bool is_open() { return status & KEY_ON; }
  bool is_cap() { return status & CAP_ON; }
  bool is_stop() { return status & REJECT; }

  void stop() { status |= REJECT; }
  void pulse() { status |= PULSE; }
  void on() { status |= POWER; }
  void off() { status &= ~POWER; target = 0; }
  void pause() { status |= PAUSE; }

  u16 get_power() { return sum >> 10; }

  void set_power(s16 power) { target = power * 10 + (power >> 2); }
};
