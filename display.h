#ifndef __DISPLAY__
#define __DISPLAY__

#include <TM1637Display.h>

void resetDisplay(TM1637Display &display);

void showTime( TM1637Display &display, int dighour, int digmin);

void showTemp( TM1637Display &display,float t);
void showHumi( TM1637Display &display,float t);

void formatTemp(float t, int &digit_oneT, int &digit_twoT, int &digit_threeT);
void formatHumi(float t, int &digit_oneT, int &digit_twoT);

#endif // __DISPLAY__

void resetDisplay(TM1637Display &display)
{
 display.setBrightness(0xF);
 uint8_t data[] = { 0x0, 0x0, 0x0, 0x0 };
 display.setSegments(data);
}

void showTime( TM1637Display &display,int tdelay, int dighour, int digmin)
{
 int tcounter = 0;
 const int tinc = 500;
 // turn off all display digits
 uint8_t data[] = { 0x0, 0x0, 0x0, 0x0 };
 display.setSegments(data);

 //turn on semicolon 
 uint8_t segto;
 int digit_All = (dighour*100) + (digmin);
 segto = 0x80 | display.encodeDigit(dighour%10);

 //The following code scans really fast throug the digits so it creates persistance of vision... aditional delays really screw with this.
  while (tcounter < tdelay) { //Wait for connection to get actual time
   display.showNumberDec(digit_All,false,4,0);
   delay(tinc);
   display.setSegments(&segto, 1, 1);
   delay(tinc);
   tcounter = tcounter + tinc;
  } 
 
}

void showTemp( TM1637Display &display,float t)
{
 const uint8_t Letter_C[] = { SEG_A | SEG_F | SEG_E | SEG_D }; // C
 const uint8_t Symbol_C[] = { SEG_A | SEG_B | SEG_F | SEG_G }; // o derajat
 const bool leading_zero = false;
 uint8_t segto;
 int digit_oneT = 0 ,digit_twoT =0 ,digit_threeT = 0;
 
 formatTemp(t, digit_oneT, digit_twoT, digit_threeT);
 
 if (0 != digit_oneT )
 { 
 display.showNumberDec(digit_oneT,leading_zero,1,0);
 }
 else
 {
 display.setSegments(Letter_C,1,3);
 }
 //turn on semicolon 
 //segto = 0x80 | display.encodeDigit(digit_twoT);
 segto = display.encodeDigit(digit_twoT);
 display.setSegments(&segto, 1, 1);
 //display.showNumberDec(digit_threeT,leading_zero,1,2);
 display.setSegments(Symbol_C,1,2);
 display.setSegments(Letter_C,1,3);
}

void formatTemp(float t, int &digit_oneT, int &digit_twoT, int &digit_threeT)
{ 
 if (t >= 100.0)
 {
  t -=100.0;
 }
 t = 0.005 + t/10.;
 digit_oneT = (int)t;
 t = (t - digit_oneT)*10.;
 digit_twoT = (int)t;
 t = (t - digit_twoT)*10;
 digit_threeT = (int)t;
}

void showHumi( TM1637Display &display,float t)
{
 const uint8_t Letter_H[] = { SEG_G | SEG_F | SEG_E | SEG_B | SEG_C }; // H
 const bool leading_zero = false;
 uint8_t segto;
 int digit_oneT = 0 ,digit_twoT =0;

 uint8_t LEmpty[] = { 0x0, 0x0, 0x0, 0x0 };
 display.setSegments(LEmpty);
 
 formatHumi(t, digit_oneT, digit_twoT);
 
 if (0 != digit_oneT )
 { 
 display.showNumberDec(digit_oneT,leading_zero,1,0);
 }
 else
 {
 display.setSegments(Letter_H,1,3);
 }
 segto = display.encodeDigit(digit_twoT);
 display.setSegments(&segto, 1, 1);
 display.setSegments(Letter_H,1,3);
}

void formatHumi(float t, int &digit_oneT, int &digit_twoT)
{ 
 if (t >= 100.0)
 {
  t -=100.0;
 }
 t = 0.005 + t/10.;
 digit_oneT = (int)t;
 t = (t - digit_oneT)*10.;
 digit_twoT = (int)t;
 t = (t - digit_twoT)*10;
}
