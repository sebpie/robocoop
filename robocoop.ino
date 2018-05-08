#include <RTClib.h>

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>

#include "Calendar.h"

int hourMinToMin(int in_hour, int in_minute)
{
  return in_hour * 60 + in_minute;
}

RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
int lastDayDoorOpened = -1;
int lastDayDoorClosed = -1;
// possibilité de mettre les minutes soit avec une ligne de plus, soit en convertissant le tout en minutes (*60)
int minuteToOpenTheDoor = hourMinToMin(,34);
int minuteToCloseTheDoor = hourMinToMin(19,35);

//variable controle verin
int pinIn1 = 3;
int pinIn2 = 5;

//fonction controle verin
void verinStop()
{
  // Verin stop
  digitalWrite(pinIn1, LOW);
  digitalWrite(pinIn2, LOW);
}

void verinOut()
{
  // Verin sort
  digitalWrite(pinIn1, HIGH);
  //si variation vitesse entre 0 et 255
  //analogWrite(pinIn1, 255);
  digitalWrite(pinIn2, LOW);
  delay (30000);
  verinStop();
}

void verinIn()
{ 
  // Verin rentre
  digitalWrite(pinIn1, LOW);
  digitalWrite(pinIn2, HIGH);
  //si variation vitesse entre 0 et 255
  //analogWrite(pinIn2, 255);
  delay (30000);
  verinStop();
}






void setup () {
  while (!Serial); // for Leonardo/Micro/Zero

  Serial.begin(57600);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void loop () {
    DateTime now = rtc.now();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    

    // Si la porte n'a pas encore été ouverte aujourd'hui
    if (lastDayDoorOpened != now.dayOfTheWeek())
    { 
      if (hourMinToMin(now.hour(), now.minute()) >= minuteToOpenTheDoor)
      {
        Serial.println("Ouverture");
        //commande ouverture vérin
        verinOut();
        lastDayDoorOpened = now.dayOfTheWeek();
        Serial.println(lastDayDoorOpened, DEC);
      }
    }

      // Si la porte n'a pas encore été fermée aujourd'hui
    if (lastDayDoorClosed != now.dayOfTheWeek())
    { 
      if (hourMinToMin(now.hour(), now.minute()) >= minuteToCloseTheDoor)
      {
        Serial.println("Fermeture");
        //commande fermeture vérin
        verinIn();
        lastDayDoorClosed = now.dayOfTheWeek();
      }
    }
  
    delay(3000);
}
