#include <Wire.h>
#include <LiquidCrystal.h>
#define DS1307_I2C_ADDRESS 0x68

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(17, 15, 14, 13, 12, 11);

volatile int stunden   = 0;
volatile int minuten   = 0;
volatile int sekunden  = 0;

void setup() {
  lcd.begin(16,2);
  Wire.begin();
/*  
  Wire.beginTransmission(0x68);
  Wire.write(0);
  Wire.write(decToBcd(0)); // 0 to bit 7 (= Sekunden) starts the clock
  Wire.write(decToBcd(29)); // minute
  Wire.write(decToBcd(20)); // stunde
  Wire.write(decToBcd(4)); // tag der woche 7 = sonntag
  Wire.write(decToBcd(28)); // tag
  Wire.write(decToBcd(8)); // monat
  Wire.write(decToBcd(15)); // jahr
  Wire.write(0x10); // Aktiviere 1 HZ Output des DS1307 auf Pin 7
  Wire.endTransmission();
 */
  
  
}

void loop() {
  // put your main code here, to run repeatedly:
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  sekunden = second;
  minuten = minute;
  stunden = hour;
  lcd.setCursor(0,0);
  lcd.print(hour);
  lcd.print(":");
  lcd.print(minute);
  lcd.print(":");
  lcd.print(second);
  delay(500);
}

void getDateDs1307(byte *second, byte *minute,byte *hour,byte *dayOfWeek,byte *dayOfMonth,byte *month, byte *year)
{
 // Reset the register pointer
 Wire.beginTransmission(DS1307_I2C_ADDRESS);
 Wire.write(0);
 Wire.endTransmission();
 
 Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
 
 // A few of these need masks,, because certain bits are control bits
 *second = bcdToDec(Wire.read() & 0x7f);
 *minute = bcdToDec(Wire.read());
 *hour = bcdToDec(Wire.read() & 0x3f); // 0x3f == 24 h --  0x1f == 12h Need to change this if 12 hour am/pm
 *dayOfWeek = bcdToDec(Wire.read());
 *dayOfMonth = bcdToDec(Wire.read());
 *month = bcdToDec(Wire.read());
 *year = bcdToDec(Wire.read());
}

byte bcdToDec(byte val)
{
 return ( (val/16*10) + (val%16) );
}

byte decToBcd(byte val)
{
 return ( (val/10*16) + (val%10) );
}

