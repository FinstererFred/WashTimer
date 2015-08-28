#include <avr/io.h> 
#include <avr/interrupt.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#define DS1307_I2C_ADDRESS 0x68

#define BUTTON1_PIN 6  // Button 1
#define BUTTON2_PIN 7  // Button 2
#define BUTTON3_PIN 8  // Button 2
#define INT_PIN1 0
#define INT_PIN2 1

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

typedef struct wtimer_t {
  int hour;
  int minute;
  int second;
};

typedef struct page_t {
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

typedef struct dauer_t {
  int hours;
  int minutes;
  String name;
};

static dauer_t Programm[3] = {
          {0,2, "Kurzwaesche"},
          {1,05, "Buntwaesche"},
          {3,40, "Ku"}
        };

volatile int activeProgramm = 1;
volatile int i = -1;
volatile int j = 0;
volatile int activePage = 0;
volatile bool timerActive = false;
volatile wtimer_t wtimer = {15,44};
volatile wtimer_t ctimer = {0,0,0};
volatile int b = 0;
volatile int timerDigit = 0;
volatile int clockDigit = 0;
volatile bool running = false;
volatile bool stateChanged = false;
volatile bool clockEdit = false;
volatile bool clockChanged = false;
volatile int semaRunning = 0;
volatile int semaNotRunning = 0;
volatile int runningTimeMin = 0;
volatile int runningTimeSec = 60;
volatile unsigned long locTime = 0;
volatile unsigned long oldSec = 0;
volatile bool blink = false;

void page0_draw();
void page0_plus();
void page0_minus();
void page0_enter();
void page0_longEnter();

void page1_draw();
void page1_plus();
void page1_minus();
void page1_enter();
void page1_longEnter();

void setupISR();
void readButtons();
int button(int pinNr);

static page_t Pages[] = {
        {"startseite",   0,0,0, &page0_draw, &page0_plus, &page0_minus, &page0_enter, &page0_longEnter},
        {"timer stellen",0,0,0, &page1_draw, &page1_plus, &page1_minus, &page1_enter, &page1_longEnter}, 
        {"uhr stellen",  0,0,0, &page2_draw, &page2_plus, &page2_minus, &page2_enter, &page2_longEnter}
      };

int pageCount = sizeof(Pages)/sizeof(Pages[0]);


/********************************/
/********************************/

void setup() {

  setupISR();

  pciSetup(INT_PIN1);
  pciSetup(INT_PIN2);

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
    digitalWrite(START_PIN, HIGH);
    
    locTime = millis() / 1000;
    runningTimeMin = runTimeMin-1;
    timerActive = false;
    i = -1;
    semaRunning = 0;
  }
  
  if (!running && semaNotRunning == 1) {
    digitalWrite(LED2, LOW);
    digitalWrite(START_PIN, LOW);
    i = -1;
    semaNotRunning = 0;
  }

  if (running)
  {
    displayCountdown();
  }

/* debug */  
  if (activePage == 0 && 1 == 0) {
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

/*****************************************/
/* ende    */

void setupISR() {
  cli(); // stop interrupts while we meddle with the clocks
  TCNT1=0x0000; // reset counter to 0 at start
  TCCR1A= B00000000; // turn off PWM
  TCCR1B= B00001100; // set CTC on and prescalar to /256
  TIMSK1 |= _BV(OCIE1A) ; // enable timer compare A interrupt
  OCR1A = 62499; //set the comparator A to trigger at 1 second
  sei(); //start interrupts again

}

ISR(TIMER1_COMPA_vect) {

  if (running) {
    runningTimeSec--;

    if (runningTimeSec < 0 && runningTimeMin > 0)
    {
      runningTimeMin--;
      runningTimeSec = 59;
    }
  }

  blink = !blink;
  if (!running) {
    displayMenu(activePage);
  }
}

void getDateDs1307(byte *second, byte *minute,byte *hour,byte *dayOfWeek,byte *dayOfMonth,byte *month, byte *year) {
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7, true);

  // A few of these need masks,, because certain bits are control bits
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f); // 0x3f == 24 h --  0x1f == 12h Need to change this if 12 hour am/pm
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());

}

byte bcdToDec(byte val) {
  return ( (val/16*10) + (val%16) );
}

byte decToBcd(byte val) {
  return ( (val/10*16) + (val%10) );
}


void displayMenu(int page) {
  Pages[page].Draw();
}

void displayCountdown() {
    lcd.setCursor(0,0); 
    if (runningTimeMin < 100)lcd.print("0");
    if (runningTimeMin < 10)lcd.print("0");
    lcd.print(runningTimeMin);
    lcd.print(":");
    
    if (runningTimeSec < 10) lcd.print("0");
    lcd.print(runningTimeSec);
    lcd.print(" >");

}

int button(int pinNr) {
  unsigned long StartTime = millis();
  while( digitalRead(pinNr) == HIGH ) {}
  unsigned long CurrentTime = millis();
  unsigned long ElapsedTime = CurrentTime - StartTime;

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

ISR (PCINT2_vect) {
  if (digitalRead(INT_PIN1) == HIGH) {
    activeProgramm = 0;
  }
  
  if (digitalRead(INT_PIN2) == HIGH) {
    activeProgramm = 1;
  }

  displayMenu(activePage);
}  

void pciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group 
}


/**************************/
/* page 1 - startseite */
/**************************/
  void page0_plus() {
    if (activePage <= 1) {
      activePage++;
    }
  }

  void page0_minus() {
    if (activePage > 0) {
      activePage--;
    } else {
      activePage = pageCount-1;
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

  void page0_longEnter() {
    ;
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
/**************************/
/**************************/



/* page 2 - timer stellen */
/**************************/
  void page1_plus() {
    if (timerDigit == 0) {
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
    if (timerDigit == 0) {
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

  void page1_enter() {
    /* change timer digit */
    if (timerDigit == 0) {
      timerDigit = 1;
    } else {
      timerDigit = 0;
    }

  }

  void page1_longEnter() {
    activePage = 0;
    lcd.clear();
  }

  void page1_draw() {
    lcd.clear();
    lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY);
    lcd.print("Timer stellen");
    
    lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY+1);
    char lcd_buffer[130];    
    
    if (blink) {
      sprintf(lcd_buffer,"%02i:%02i Uhr",wtimer.hour, wtimer.minute);
    } else {
      
      if(timerDigit==0) {
        sprintf(lcd_buffer,"  :%02i Uhr",wtimer.minute);
      } else {
        sprintf(lcd_buffer,"%02i:   Uhr",wtimer.hour);
      }
    }
    lcd.print(lcd_buffer);
    
    lcd.setCursor(Pages[activePage].posX+1, Pages[activePage].posY+1);

  }
/**************************/
/**************************/



/* page 3 - uhr stellen */
/**************************/
  void page2_plus() {
    if (clockDigit == 0) {
      if (ctimer.hour < 23) {ctimer.hour++; } else {ctimer.hour = 0; }
    } else if (clockDigit == 1) {
      if (ctimer.minute < 59) {ctimer.minute++; } else {ctimer.minute = 0; }
    } else {
      if (ctimer.second < 59) {ctimer.second++; } else {ctimer.second = 0; }
    }
    clockChanged = true;
    displayMenu(activePage);
  }

  void page2_minus() {
    if (clockDigit == 0) {
      if (ctimer.hour > 0) {ctimer.hour--; } else { ctimer.hour = 23;}

    } else if (clockDigit == 1) {
      if (ctimer.minute > 0) {ctimer.minute--; } else { ctimer.minute = 59;}

    } else {
      if (ctimer.second > 0) {ctimer.second--; } else { ctimer.second = 59;}
    }
    clockChanged = true;
    displayMenu(activePage);
  }

  void page2_enter() {
    /* change timer digit */
    if (clockDigit == 0) {
      clockDigit = 1;
    } else if(clockDigit == 1) {
      clockDigit = 2;
    } else {
      clockDigit = 0;
    }
  }

  void page2_longEnter() {
    if (clockChanged) {
      saveTime();
      clockChanged = false;
    }
    activePage = 0;
    clockEdit = false;
    lcd.clear();
  }


  void page2_draw() {
    
    if (clockEdit == false) {
      byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
      getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

      ctimer.hour = hour;
      ctimer.minute = minute;
      ctimer.second = second;
      clockEdit = true;
    }
    
    lcd.clear();
    lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY);
    lcd.print("Uhrzeit stellen");

    lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY+1);
    char lcd_buffer[130];    
    
    if (blink) {
      sprintf(lcd_buffer,"%02i:%02i:%02i Uhr", ctimer.hour, ctimer.minute, ctimer.second);
    } else {
      if (clockDigit == 0) {
        sprintf(lcd_buffer,"  :%02i:%02i Uhr", ctimer.minute, ctimer.second);
      } else if (clockDigit == 1) {
        sprintf(lcd_buffer,"%02i:  :%02i Uhr", ctimer.hour, ctimer.second);
      } else {
        sprintf(lcd_buffer,"%02i:%02i:   Uhr", ctimer.hour, ctimer.minute );
      }
    }
    lcd.print(lcd_buffer);
    
  }

  void saveTime() {
    byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
    getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

    Wire.beginTransmission(0x68);
    Wire.write(0);
    Wire.write(decToBcd(0));              // 0 to bit 7 (= Sekunden) starts the clock
    Wire.write(decToBcd(ctimer.minute));  // minute
    Wire.write(decToBcd(ctimer.hour));    // stunde
    Wire.write(decToBcd(dayOfWeek));      // tag der woche 7 = sonntag
    Wire.write(decToBcd(dayOfMonth));     // tag
    Wire.write(decToBcd(month));          // monat
    Wire.write(decToBcd(year));           // jahr
    Wire.write(0x10);                     // Aktiviere 1 HZ Output des DS1307 auf Pin 7
    Wire.endTransmission();
  }
/**************************/
/**************************/