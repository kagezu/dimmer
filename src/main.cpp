#include "lcd.h"
#include "pinout.h"
#include "timer.h"
#include "serif_18i.h"


LCD lcd;
int mode = 1;

int main(void)
{
  T1_DIV_1024;
  TCNT1 = 0;

  lcd.init();
  lcd.font(serif_18i, 0, 0);
  lcd.color(White);
  lcd.background(MidnightBlue);
  // lcd.clear();

  // timer.enable();

  int x = 0;
  while (true) {
    TCNT1 = 0;
    lcd.demo(x++);
    // if (USER_B.get()) {
    //   while (USER_B.get());
    //   mode++;
    // }
    // timer.clear();

    // switch (mode) {
    //   case 0:
    //     lcd.background(color[x++ & 0x7F]);
    //     lcd.clear();
    //     break;
    //   case 1: lcd.demo(x++); break;
    //     // case 2: lcd.demo2(x++); break;
    //   case 2: lcd.demo3(x++); break;

    //   default: mode = 0;
    // }

    u16 fps = ((F_CPU >> 6) / TCNT1);
    lcd.at(0, lcd.max_y() - 2 * lcd.get_height() + 1);
    lcd.color(White);
    lcd.background(MidnightBlue);
    lcd.printf(
      P("FPS: %.2.4q\n%u X %u X %u"), fps,
      lcd.max_x() + 1, lcd.max_y() + 1, RGB::len());
  }
}

/*
int main(void)
{
  SPI0_SCK.init(GPO_Max);

  while (true) {

    SPI0_SCK.inv();
    delay_ms(100);
  }
}

*/
