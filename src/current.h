#pragma once
#include "type/int.h"

#define MAX_CURRENT 6000

#define C_M1  32
#define C_A1  243
#define C_B1  6.9412f

#define C_M2  66
#define C_A2  251
#define C_B2  6.8236f


uc16 cur_raw[] = {
  0,132,155,173,188,200,212,224,236,248,260,271,281,293,304,312,319,
  327,334,342,349,356,364,374,383,392,403,414,425,435,445,455,465
};

// Возвращает ток в мА
uint16_t current(uint16_t arg)
{
  if (arg <= C_M1) return cur_raw[arg];
  if (arg > C_M1 && arg <= C_M2) return C_A1 + C_B1 * arg;
  if (arg > C_M2) return C_A2 + C_B2 * arg;
  return 0x1FFF;
}
