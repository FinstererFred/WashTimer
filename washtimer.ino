#include <avr/io.h> 
#include <avr/interrupt.h>

#include <LiquidCrystal.h>
#include <Wire.h>
#define DS1307_I2C_ADDRESS 0x68

#define BUTTON1_PIN 6  // Button 1
#define BUTTON2_PIN 7  // Button 2
#define BUTTON3_PIN 8  // Button 2

#define START_PIN 9 

#define LED1 5
#define LED2 4

byte alarm[8] = {
  B00000,
  B00100,
  B01010,
  B01010,
  B10001,
  B00100,
  B00000,
};

LiquidCrystal lcd(17, 15, 14, 13, 12, 11);


/*****************************************/
/* structs */

struct wtimer_t {
  int hour;
  int minute;
};

struct page_t {
  String Text;
  int posX;
  int posY;
  int parent;
  void (*Draw)();
  void (*Plus)();
  void (*Minus)();
  void (*Enter)();
  
  void (*LongEnter)();
};

struct dauer_t {
  int hours;
  int minutes;
  String name;
};

static dauer_t Programm[3] = {
          {0,2, "P1"},
          {1,05, "Buntwäsche"},
          {3,40, "Ku"}
        };

volatile int activeProgramm = 0;
volatile int i = -1;
volatile int j = 0;
volatile int activePage = 0;
volatile bool timerActive = false;
volatile wtimer_t wtimer = {12,52};
volatile int b = 0;
volatile int timerMode = 0;
volatile bool running = false;
volatile bool stateChanged = false;

/* page 1 - startseite */
  void page0_plus() {
    if (activePage <= 1) {
      activePage++;
    }
  }

  void page0_minus() {
    if (activePage > 0) {
      activePage--;
    }
  }

  void page0_enter() {
    if (timerActive == false) {
      timerActive = true;
    } else {
      timerActive = false;
    }
    displayMenu(activePage);
  }

  void page0_draw();


  void page0_longEnter() {
    ;
  }


/* page 2 - timer stellen */
  void page1_plus() {
    if (timerMode == 0) {
      if (wtimer.hour < 23) {
        wtimer.hour++;
      } else {
        wtimer.hour = 0;
      }
    } else {
      if (wtimer.minute < 59) {
        wtimer.minute++;
      } else {
        wtimer.minute = 0;
      }
    }
    displayMenu(activePage);
  }

  void page1_minus() {
    if (timerMode == 0) {
      if (wtimer.hour > 0) {
        wtimer.hour--;
      } else {
        wtimer.hour = 23;
      }
    } else {
      if (wtimer.minute > 0) {
        wtimer.minute--;
      } else {
        wtimer.minute = 59;
      }
    }
    displayMenu(activePage);
  }



  void page1_longEnter() {
    activePage--;
    lcd.clear();
  }

  void page1_draw();


static page_t Pages[2] = {
        {"startseite",0,0,0, &page0_draw, &page0_plus, &page0_minus, &page0_enter, &page0_longEnter},
        {"timer stellen",0,0,0, &page1_draw, &page1_plus, &page1_minus, &page1_enter, &page1_longEnter}
      };


  void page1_enter() {
    /* change timer digit */
    if (timerMode == 0) {
      timerMode = 1;
    } else {
      timerMode = 0;
    }

  }

void page0_draw() {

    int doneHour = wtimer.hour+Programm[activeProgramm].hours;
    int doneMinute = 0;
    if (wtimer.minute+Programm[activeProgramm].minutes >= 60)
    {
      doneHour++;
      doneMinute = wtimer.minute+Programm[activeProgramm].minutes - 60;
    } else {
      doneMinute = wtimer.minute+Programm[activeProgramm].minutes;
    }


  if (!running) {
    lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY);
    char lcd_buffer[130];
    sprintf(lcd_buffer,"%02i:%02i  > %02i:%02i ",wtimer.hour, wtimer.minute, doneHour, doneMinute);
    lcd.print(lcd_buffer);
  } else {
    lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY);
    char lcd_buffer[130];
    sprintf(lcd_buffer,"       > %02i:%02i ", doneHour, doneMinute);
    lcd.print(lcd_buffer);
  }

  if (timerActive == true) {
    lcd.write(byte(0));
  }
  else
  {
    lcd.print(" ");
  }

  lcd.setCursor(0, 1);
  lcd.print(Programm[activeProgramm].name);

}


void page1_draw() {
  lcd.clear();
  lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY);
  lcd.print("Timer stellen");
  lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY+1);
  char lcd_buffer[130];    
  sprintf(lcd_buffer,"%02i:%02i Uhr",wtimer.hour, wtimer.minute);
  lcd.print(lcd_buffer);
  lcd.setCursor(Pages[activePage].posX+1, Pages[activePage].posY+1);
}

/*****************************************/
/* ende    */

void setup() {

  cli(); // stop interrupts while we meddle with the clocks
  TCNT1=0x0000; // reset counter to 0 at start
  TCCR1A= B00000000; // turn off PWM
  TCCR1B= B00001100; // set CTC on and prescalar to /256
  TIMSK1 |= _BV(OCIE1A) ; // enable timer compare A interrupt
  OCR1A = 62499; //set the comparator A to trigger at 1 second
  sei(); //start interrupts again


  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(BUTTON3_PIN, INPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(START_PIN, OUTPUT);
  
  Wire.begin();

  lcd.createChar(0, alarm);
  lcd.begin(16, 2);
}

int button(int pinNr) {
  unsigned long StartTime = millis();
  while( digitalRead(pinNr) == HIGH ) {}
  unsigned long CurrentTime = millis();
  unsigned long ElapsedTime = CurrentTime - StartTime;

//  lcd.setCursor(0, 1);
//  lcd.print(ElapsedTime);
 
  if (ElapsedTime >= 401UL) {
    return 2;
  } else if (ElapsedTime >= 20UL && ElapsedTime <= 400UL) {
    return 1;
  } else {
    return 0;
  }

}

void readButtons() {

  int ret1 = button(BUTTON1_PIN);
  int ret2 = button(BUTTON2_PIN);
  int ret3 = button(BUTTON3_PIN);
  
  if (ret1 == 1 || ret1 == 2) {
    Pages[activePage].Plus();
  }

  if (ret2 == 1) {
    Pages[activePage].Enter(); 
  } else if (ret2 == 2) {
    Pages[activePage].LongEnter();
  }

  if (ret3 == 1 || ret3 == 2) {
    Pages[activePage].Minus(); 
  }

}

int semaRunning = 0;
int semaNotRunning = 0;

int runningTimeMin = 0;
int runningTimeSec = 60;

unsigned long locTime = 0;

void loop()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  readButtons();

  if (timerActive) digitalWrite(LED1, HIGH);
  else digitalWrite(LED1, LOW);
  

  int runTimeMin   = Programm[activeProgramm].hours*60 + Programm[activeProgramm].minutes;
  int startTimeMin = wtimer.hour*60 + wtimer.minute;
  int endTimeMin   = startTimeMin + runTimeMin;
  int nowTimeMin   = hour*60 + minute;

/* pruefen ob waschmaschine gestartet werden muss */
  if (nowTimeMin >= startTimeMin && nowTimeMin < endTimeMin) {
    if (!running && timerActive) {
      running = true;
      semaRunning++;
    }
  } else {
    if (running) {
      running = false;
      semaNotRunning++;
    }
  }

/* aktionen nur einmal starten */  
  if (running && semaRunning == 1) {
    digitalWrite(LED2, HIGH);
    locTime = millis() / 1000;
    runningTimeMin = runTimeMin-1;
    
    timerActive = false;
    i = -1;
    semaRunning = 0;
  }
  
  if (!running && semaNotRunning == 1) {
    digitalWrite(LED2, LOW);
    i = -1;
    semaNotRunning = 0;
  }

  if(running)
  {
    displayCountdown();
  }

/* debug */  
  if (activePage == 0 && 1 == 1)
  {
    lcd.setCursor(0,1);
    lcd.print(hour);
    lcd.print(":");
    if (minute < 10) lcd.print(0);
    lcd.print(minute);
    lcd.print(":");
    if (second < 10) lcd.print(0);
    lcd.print(second);
  }



  if (i != activePage) {
    displayMenu(activePage);
    i=activePage;
  }
}

void displayMenu(int page) {
  Pages[page].Draw();
}

unsigned long oldSec = 0;


volatile int tescht = 0;

ISR(TIMER1_COMPA_vect) {

  if(running) {
    runningTimeSec--;

    if(runningTimeSec <= 0 && runningTimeMin > 0)
    {
      runningTimeMin--;
      runningTimeSec = 60;
    }
  }
}


void displayCountdown()
{
    lcd.setCursor(0,0); 
    if(runningTimeSec < 100)lcd.print("0");
    if(runningTimeSec < 10)lcd.print("0");
    lcd.print(runningTimeMin);
    lcd.print(":");
    if(runningTimeSec < 10)lcd.print("0");
    lcd.print(runningTimeSec);
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