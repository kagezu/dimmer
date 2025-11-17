#pragma once
#include "printf.h"
#include "spi.h"

uint8_t table[] = {
  0b1101111,  // , нижний 1
  0b0111111,  // -
  0b0100011,  // . нижний о
  0b0101101,  // /
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
  0b1101011,  // : нижний 2
  0b1111111,  // ;
  0b0110111,  // <
  0b1110110,  // =
  0b0111110,  // >
  0b0101100,  // ?
  0b0011100,  // @ верхний o
  0b0001000,  // A
  0b0000011,  // B
  0b1000110,  // C
  0b0100001,  // D
  0b0000110,  // E
  0b0001110,  // F
  0b1000010,  // G
  0b0001001,  // H
  0b1001111,  // I
  0b1100001,  // J
  0b1111111,  // K
  0b1000111,  // L
  0b1001001,  // M
  0b1001001,  // N
  0b1000000,  // O
  0b0001100,  // P
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
    if (num > 999) printf(P("\f%2u "), num / 100);
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
        buffer[buffer_index--] = table[ch - ','];
    }

    if (buffer_index > 2) buffer_index = 2;
  }

  void view()
  {
    DIGIT.set(0b111);
    spi.send(buffer[digit]);
    spi.wait();
    DIGIT.clr(1 << digit);
    digit++;
    if (digit > 2) digit = 0;
  }

};
