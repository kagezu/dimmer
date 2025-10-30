#pragma once
#include "printf.h"
#include "spi.h"

uint8_t table[] = {
  0b1000000,  // -
  0b1000000,  // .
  0b1000000,  // /
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
  0b1000000,  // :
  0b1000000,  // ;
  0b1000000,  // <
  0b1000000,  // =
  0b1000000,  // >
  0b1000000,  // ?
  0b1000000,  // @
  0b0001000,  // A
  0b1000000,  // B
  0b1000000,  // C
  0b1000000,  // D
  0b1000000,  // E
  0b1000000,  // F
};

Port<PC, 0b111> DIGIT;

class Print : public PrintF {
protected:
  SPI spi;
  u8 buffer_index;
  u8 digit;

  volatile uint8_t buffer[3] = { 127,127,127 };

public:
  Print()
  {
    DIGIT.init(GPO_Max);
    DIGIT.set(0b111);

    spi.fq(2000);
    spi.begin();
    buffer_index = 2;
    digit = 0;
  }

  void number(u16 num)
  {
    if (num > 999) printf(P("\fA00"), num);
    else printf(P("\f%3u"), num);
  }

  void putc(uint8_t ch)
  {
    switch (ch) {
      case '\f':
      case '\n':
        buffer_index = 2;
        break;

      case ' ':
        buffer[buffer_index--] = 0b1111111;
        break;

      default:
        buffer[buffer_index--] = table[ch - '-'];
    }

    if (buffer_index > 2) buffer_index = 2;
  }

  void view()
  {
    if (digit++ == 2) digit = 0;
    DIGIT.set(0b111);
    spi.send(buffer[digit]);
    spi.wait();
    DIGIT.clr(1 << digit);
  }

};
