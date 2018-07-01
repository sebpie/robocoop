#include "pins.h"

#define DEBUG(x)  Serial.println(x)
#define BLUETOOTH
#define RTC_ENABLED

/***************************************************************************************/
/* RTC and timing declarations                                                         */

#include <RTClib.h>

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "Calendar.h"

#define hourMinToMin(h,m) (h * 60 + m)

RTC_DS1307 rtc;

char *daysOfTheWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
uint8_t lastDayDoorOpened = -1;
uint8_t lastDayDoorClosed = -1;


#define LOG_SIZE  10
DateTime log_verin[LOG_SIZE];


// Adresse dans la NVRAM du DS1307 des parametres d'heure d'ouverture et de fermeture
#define NVRAM_ADDR_OFFSET       (uint16_t) 0x00             // Adresse du nombre de minutes apres le coucher du soleil ou il faut fermer la porte
#define NVRAM_ADDR_OPEN_HOUR    NVRAM_ADDR_OFFSET + 2       // Adresse de l'heure de l'heure d'ouverture
#define NVRAM_ADDR_OPEN_MIN     NVRAM_ADDR_OPEN_HOUR + 1    // Adresse des minutes de l'heure d'ouverture



void setup_RTC() {
  /* using A2 and A3 as GND/VCC for the RTC module */
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  digitalWrite(A2, LOW);
  digitalWrite(A3, HIGH);

  
  if (! rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }


}


const uint8_t daysInMonth []  = { 31,28,31,30,31,30,31,31,30,31,30,31 };

uint16_t dayOfTheYear(DateTime dt) {
    uint8_t d = dt.day();
    uint8_t m = dt.month();
    uint16_t days = 0;
    
    for (uint8_t i = 1; i < m; ++i)
        days += daysInMonth[ i - 1];

    return days; 
}


/***************************************************************************************/
/* Verin control declarations                                                          */

//variable controle verin
int pinIn1 = PIN_VERIN_DIR1;
int pinIn2 = PIN_VERIN_DIR2;

#define VERIN_DELAY     5000
#define DUTYCYCLE_FULL  0xFF    /* 100% duty cycle */
#define DUTYCYCLE_25    0x3F    /*  25% duty cycle */
#define DUTYCYCLE_0     0x00    /*   0% duty cycle */

#define VERIN_IN            0x01  /* 0000 0001  */
#define VERIN_OUT           0x02  /* 0000 0010  */
#define VERIN_STOP          0x10  /* 0001 0000  */
#define VERIN_ACTIVATE      0x20  /* 0010 0000  */


uint32_t  verin_timer = 0;
boolean   verin_active = false;


void verin(uint8_t bits) {
      if((bits & VERIN_IN) == VERIN_IN) {
          digitalWrite(PIN_VERIN_DIR1, LOW);
          digitalWrite(PIN_VERIN_DIR2, HIGH);
      }
      else if((bits & VERIN_OUT) == VERIN_OUT) {
          digitalWrite(PIN_VERIN_DIR1, HIGH);
          digitalWrite(PIN_VERIN_DIR2, LOW);
      }

      if((bits & VERIN_ACTIVATE) == VERIN_ACTIVATE)
          verin_activate();
      else if((bits & VERIN_STOP) == VERIN_STOP)
          verin_deactivate();
}

void verin_activate() {
  analogWrite(PIN_VERIN_EN, DUTYCYCLE_FULL);
  verin_active  = true;
  verin_timer   = rtc.now().unixtime() + VERIN_DELAY;

  log_add();
}

void verin_deactivate() {
  analogWrite(PIN_VERIN_EN, DUTYCYCLE_0);
  verin_active = false;
 
  log_add();
 }

void setup_verin() {
  pinMode(PIN_VERIN_DIR1, OUTPUT);
  pinMode(PIN_VERIN_DIR2, OUTPUT);
  pinMode(PIN_VERIN_EN,   OUTPUT);

  verin(VERIN_IN | VERIN_STOP );
}




/***************************************************************************************/
/* Bluetooth & CLI declarations                                                        */
#include <CommandLine.h>
#include <SoftwareSerial.h>

#ifdef BLUETOOTH
#define BT_BAUDRATE 115200
#endif /* BLUETOOTH */

#if 0
const char* strVersion = "0.1";
const char* _OK = "OK ";
const char* _ERR = "ERROR ";
const char* _NOT_IMPLEMENTED = "NOT IMPLEMENTED";
#else
#define strVersion        F("0.2-dev")
#define _OK               F("OK")
#define _ERR              F("ERROR")
#define _NOT_IMPLEMENTED  F("NOT IMPLEMENTED")
#define _BAD_PARAMS       F("ERROR: BAD PARAMETERS") 
#endif

char buffer[100];

SoftwareSerial btSerial(PIN_BT_RX, PIN_BT_TX); // RX, TX

CommandLine usb_cli(Serial, "> ");

#ifdef BLUETOOTH
CommandLine bt_cli(btSerial, "> ");
#endif /* BLUETOOTH */


void cmdVersion(char*tokens, Stream& serial) {
//  sprintf(buffer, "%s: %s\n\r", _OK, strVersion);
  
  serial.print(_OK);
  serial.println(strVersion);
  
}

void cmdSunset(char*tokens, Stream& serial) {
  char *token = strtok(NULL, " ");
  uint16_t sunset =0;
  if(token == NULL) { /* TODAY */
    sunset = getSunsetTime();
  } else { /* Get date as parameter */
      uint16_t params[] = {0, 0};
      uint16_t day = 0;

      params[0] = atoi(token);
      if(NULL != (token = strtok(NULL, " "))) { /* Two parameters: month and day*/
         params[1]= atoi(token);
         day = params[1];
         for(int i=0; i < params[0]; i++) {
            day += daysInMonth[i];
         }
      } else { /* One parameter only: day of the year */
        day = params[0];
        if(day < 1 || day > 366) {
          serial.println(_BAD_PARAMS);
          return;
        }
      }
      sunset = getSunsetTime(day);
  }
  serial.print(_OK);
  serial.print(": ");
  uint8_t h, m;
  minToHourMin(sunset, &h, &m);
  serial.print(h);
  serial.print(' ');
  serial.println(m);
  
}

void minToHourMin(uint16_t minutes, uint8_t *h, uint8_t *m) {
  uint8_t tmp=0;
  while(minutes >= 60) {
      tmp++;
      minutes -= 60;
  }
  *h = tmp;
  *m = minutes;
}

void cmdOffset(char*tokens, Stream& serial) {
  char *token = strtok(NULL, " ");

  if(NULL == token) { /* GET command */
      serial.print(_OK);
      serial.println(getOffsetCloseAfterSunset());
  } else {    /* SET command */
    setOffsetCloseAfterSunset(atoi(token));
    serial.println(_OK);
  }
}

void cmdOpenTime(char*tokens, Stream& serial) {
 char *token = strtok(NULL, " ");

  if(NULL == token) { /* GET command */
      serial.print(_OK);
      serial.print(getOpenHour());
      serial.print(F(" "));
      serial.println(getOpenMin());
  } else {    /* SET command */
    setOpenHour((uint8_t) atoi(token));
    token = strtok(NULL, " ");
    if(NULL == token) {
      serial.println(_BAD_PARAMS);
      return;
    }
    setOpenMin((uint8_t) atoi(token));
    serial.println(_OK);
  }
}

void cmdDoor(char*tokens, Stream& serial) {
  char *token = strtok(NULL, " ");
  if(NULL == token) {
    serial.println(_BAD_PARAMS);
    return;
  }

  switch(atoi(token)) {
    case 0:
        verin(VERIN_IN | VERIN_ACTIVATE);
        
        serial.println(_OK);
        break;
    case 1: 
        verin(VERIN_OUT | VERIN_ACTIVATE);
        serial.println(_OK);
        break;
    case 2:
        verin(VERIN_STOP);
        serial.println(_OK);
        break;
    default: 
      serial.println(_BAD_PARAMS);
      break;
  }
}

void cmdDate(char*tokens, Stream& serial) {

  char *token = strtok(NULL, " ");
  uint8_t argc = 0;

  uint16_t  YYYY = 0;
  uint8_t   MM = 0;
  uint8_t   DD = 0;
  uint8_t   HH = 0;
  uint8_t   mm = 0;
  uint8_t   SS = 0;
  

  if(token == NULL) {   /* Get command */
    DateTime now = rtc.now();
    sprintf(buffer, "OK: %d %d %d  %d %d %d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    serial.println(buffer);
    return;    
  } else { /* Set command */     
    do {
      int arg = atoi(token);
      
      switch(argc) {
        case  0: YYYY = arg; /* DEBUG(YYYY); */ break;
        case  1: MM = arg; /* DEBUG(MM); */ break;
        case  2: DD = arg; /* DEBUG(DD); */ break;
        case  3: HH = arg; /* DEBUG(HH); */ break;
        case  4: mm = arg; /* DEBUG(mm); */ break;
        case  5: SS = arg; /* DEBUG(SS); */ break;
        default:
          DEBUG(F("Unknown value")); DEBUG(argc);
          break;
        }
      token = strtok(NULL, " ");
      argc++;
    } while(NULL != token);

    if(6 != argc) {
      serial.println(F("ERROR: bad parameters"));
      return;
    }

    rtc.adjust(DateTime(YYYY, MM, DD, HH, mm, SS));
    serial.println(_OK);
  }
}

void cmdOpen(char*tokens, Stream& serial) {
  verin(VERIN_OUT | VERIN_ACTIVATE );

  serial.println(_OK);
}

void cmdClose(char*tokens, Stream& serial) {
  Serial.println("enter close");
  
  verin(VERIN_IN | VERIN_ACTIVATE) ;

  serial.println(_OK);
}
void cmdLog(char*tokens, Stream& serial) {  
  for(int i=0; i < LOG_SIZE; i++) {

    sprintf(buffer, "LOG[%d]: %d %d %d  %d %d %d", i, log_verin[i].year(), log_verin[i].month(), log_verin[i].day(), log_verin[i].hour(), log_verin[i].minute(), log_verin[i].second());
    serial.println(buffer);

  }
}

void cmdHelp(char*tokens, Stream& serial) {  
  serial.println(F("Commands:"));
  serial.println(F("\t-version"));
  serial.println(F("\t-date [YY MM DD hh mm ss]"));
  serial.println(F("\t\t*no args: returns current date and time"));
  serial.println(F("\t\t*with args: sets clock to given date and time"));

  serial.println(F("\t-sunset"));
  serial.println(F("\t-offset [x]"));
  serial.println(F("\t\t*no arg: returns current offset"));
  serial.println(F("\t\t*with arg: set offset to x"));
  
  serial.println(F("\t-door state"));
  serial.println(F("\t\t*state 0: close"));
  serial.println(F("\t\t*state 1: open"));
  
  serial.println(F("\t-open"));
  serial.println(F("\t-close"));

  serial.println(F("\t-help"));

}

void setup_cli() {

  usb_cli.add("version",  &cmdVersion);
  usb_cli.add("sunset",   &cmdSunset);
  usb_cli.add("offset",   &cmdOffset);
  usb_cli.add("opentime", &cmdOpenTime);
  usb_cli.add("date",     &cmdDate);
  usb_cli.add("open",     &cmdOpen);
  usb_cli.add("close",    &cmdClose);
  usb_cli.add("log",      &cmdLog);
  usb_cli.add("help",     &cmdHelp);
  usb_cli.add("door",     &cmdDoor);

  
#ifdef BLUETOOTH
  bt_cli.add("version",  &cmdVersion);
  bt_cli.add("sunset",   &cmdSunset);
  bt_cli.add("offset",   &cmdOffset);
  bt_cli.add("opentime", &cmdOpenTime);
  bt_cli.add("date",     &cmdDate);
  bt_cli.add("door",     &cmdDoor);
  bt_cli.add("open",     &cmdOpen);
  bt_cli.add("close",    &cmdClose);
  bt_cli.add("help",     &cmdHelp);
  btSerial.begin(BT_BAUDRATE);
#endif /* BLUETOOTH */

}




/***************************************************************************************/
/* RTC and timing declarations                                                        */


/***************************************************************************************/
/* setup                                                                              */

void setup () {

#if defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_LEONARDO_ETH) || defined(ARDUINO_AVR_MICRO)
  while (!Serial); // for Leonardo/Micro/Zero
#endif

  Serial.begin(115200);


  // Initialise Bluetooth and CLI interfaces
  setup_cli();  
  
#ifdef RTC_ENABLED
  setup_RTC();
#endif
  setup_verin();

  setup_log();

}

void setup_log() {
  for(int i=0; i < LOG_SIZE; i++) {
    log_add();
  }
}

void log_add() {
    DateTime now = rtc.now();
/*    log_entry_t l;
    l.action = a;
    l.dt = now;
*/
    log_shift();
    log_verin[0] = now;
    
}

void log_shift() {
  for(int i = (LOG_SIZE -1) ; i >0; i--) {
    log_verin[i] = log_verin[i-1];
  }
}


/***************************************************************************************/
/* main loop                                                                           */

void loop () {
  usb_cli.update();
#ifdef BLUETOOTH
  bt_cli.update();
#endif /* BLUETOOTH */

#ifdef RTC_ENABLED
    DateTime now = rtc.now();

    if(verin_active && (now.unixtime() > verin_timer)) {
      verin_deactivate();
    }

    // Si la porte n'a pas encore été ouverte aujourd'hui
    if (lastDayDoorOpened != now.dayOfTheWeek())
    { 
      if (hourMinToMin(now.hour(), now.minute()) >= hourMinToMin(getOpenHour(), getOpenMin()))
      {
        Serial.println(F("Ouverture"));
        //commande ouverture vérin
        verin(VERIN_OUT | VERIN_ACTIVATE);
        lastDayDoorOpened = now.dayOfTheWeek();
//        Serial.println(lastDayDoorOpened, DEC);
      }
    }

      // Si la porte n'a pas encore été fermée aujourd'hui
    if (lastDayDoorClosed != now.dayOfTheWeek())
    { 
      uint16_t minuteToCloseTheDoor = getSunsetTime(now)+ getOffsetCloseAfterSunset() ;
      if (hourMinToMin(now.hour(), now.minute()) >= minuteToCloseTheDoor)
      {
        verin(VERIN_IN | VERIN_ACTIVATE);
        lastDayDoorClosed = now.dayOfTheWeek();
      }
    }
#endif /* RTC_ENABLED   */
}


uint16_t getSunsetTime(void) {
    DateTime now = rtc.now();
    return getSunsetTime(now);
}

uint16_t getSunsetTime(DateTime &now) {
  return pgm_read_word_near(sunset + dayOfTheYear(now) );
}


uint16_t getSunsetTime(uint16_t day) {
  return pgm_read_word_near(sunset + day -1 );
}


void setOffsetCloseAfterSunset(uint16_t offset) {
  rtc.writenvram(NVRAM_ADDR_OFFSET, (uint8_t*) &offset, (uint8_t) sizeof(offset));

}

uint16_t getOffsetCloseAfterSunset(void) {
    uint16_t offset;
    rtc.readnvram((uint8_t*) &offset, (uint8_t) sizeof(offset), NVRAM_ADDR_OFFSET);
    return offset;
}


uint8_t getOpenHour() {
    return    rtc.readnvram(NVRAM_ADDR_OPEN_HOUR);
}

void setOpenHour(uint8_t h) {
    rtc.writenvram(NVRAM_ADDR_OPEN_HOUR, h);
    lastDayDoorOpened = -1;
}

uint8_t getOpenMin() {
    return    rtc.readnvram(NVRAM_ADDR_OPEN_MIN);
}

void setOpenMin(uint8_t m) {
    rtc.writenvram(NVRAM_ADDR_OPEN_MIN, m);
    lastDayDoorOpened = -1;
}

