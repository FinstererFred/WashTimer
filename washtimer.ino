#include <LiquidCrystal.h>

#define BUTTON1_PIN 6  // Button 1
#define BUTTON2_PIN 7  // Button 2
#define BUTTON3_PIN 8  // Button 2

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
          {2,10, "Kurzwaesche"},
          {1,05, "Buntwäsche"},
          {3,40, "Kurzwaesche"}
        };

static int activeProgramm = 2;
static int i = -1;
static int j = 0;
static int activePage = 0;
static bool timerActive = false;
static wtimer_t wtimer = {14,30};
static int b = 0;
static int timerMode = 0;

/* page 1 - startseite */
  void page0_plus() {
    if(activePage <= 1) {
      activePage++;
    }
  }

  void page0_minus() {
    if(activePage > 0) {
      activePage--;
    }
  }

  void page0_enter() {
    if(timerActive == false) {
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
    if(timerMode == 0) {
      if(wtimer.hour < 23) {
        wtimer.hour++;
      } else {
        wtimer.hour = 0;
      }
    } else {
      if(wtimer.minute < 59) {
        wtimer.minute++;
      } else {
        wtimer.minute = 0;
      }
    }
    displayMenu(activePage);
  }

  void page1_minus() {
    if(timerMode == 0) {
      if(wtimer.hour > 0) {
        wtimer.hour--;
      } else {
        wtimer.hour = 23;
      }
    } else {
      if(wtimer.minute > 0) {
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
        {"startseite",0,0,0, &page0_draw, &page0_plus, &page0_minus, &page0_enter},
        {"timer stellen",0,0,0, &page1_draw, &page1_plus, &page1_minus, &page1_enter, &page1_longEnter}
      };


  void page1_enter() {
    /* change timer digit */
    if(timerMode == 0) {
      timerMode = 1;
    } else {
      timerMode = 0;
    }

  }

void page0_draw() {

    int doneHour = wtimer.hour+Programm[activeProgramm].hours;
    int doneMinute = 0;
    if(wtimer.minute+Programm[activeProgramm].minutes >= 60)
    {
      doneHour++;
      doneMinute = wtimer.minute+Programm[activeProgramm].minutes - 60;
    } else {
      doneMinute = wtimer.minute+Programm[activeProgramm].minutes;
    }

  
  lcd.setCursor(Pages[activePage].posX, Pages[activePage].posY);
  char lcd_buffer[130];
  sprintf(lcd_buffer,"%02i:%02i -> %02i:%02i ",wtimer.hour, wtimer.minute, doneHour, doneMinute);
  lcd.print(lcd_buffer);

  if(timerActive == true) {
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
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(BUTTON3_PIN, INPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  lcd.createChar(0, alarm);
  lcd.begin(16, 2);
//  lcd.print("hello, world! ");
//  lcd.write(byte(0));
}

int button(int pinNr) {
  unsigned long StartTime = millis();
  while( digitalRead(pinNr) == HIGH ) {}
  unsigned long CurrentTime = millis();
  unsigned long ElapsedTime = CurrentTime - StartTime;

//  lcd.setCursor(0, 1);
//  lcd.print(ElapsedTime);
 
  if (ElapsedTime >= 201UL) {
    return 2;
  } else if (ElapsedTime >= 20UL && ElapsedTime <= 200UL) {
    return 1;
  } else {
    return 0;
  }

}

void readButtons() {

  int ret1 = button(BUTTON1_PIN);
  int ret2 = button(BUTTON2_PIN);
  int ret3 = button(BUTTON3_PIN);
  
  if(ret1 == 1 || ret1 == 2) {
    Pages[activePage].Plus();
  }

  if(ret2 == 1) {
    Pages[activePage].Enter(); 
  } else if(ret2 == 2) {
    Pages[activePage].LongEnter();
  }

  if(ret3 == 1 || ret3 == 2) {
    Pages[activePage].Minus(); 
  }

}

void loop()
{
  readButtons();

  if(i != activePage) {
    displayMenu(activePage);
    i=activePage;
  }

  if(timerActive) {
    digitalWrite(LED1, HIGH);
  } else {
    digitalWrite(LED1, LOW);
  }
}

void displayMenu(int page) {
  Pages[page].Draw();
}
