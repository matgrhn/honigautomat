
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// ########## INIT DISPLAY ##########

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address

// HD44780 compatible LCD display special characters
#define UE "\365" // ü
#define AE "\341" // ä
#define OE "\357" // ö
#define SZ "\342" // ß

// EURO symbole
byte euro[8] = {
  B00110,
  B01001,
  B11100,
  B01000,
  B11100,
  B01001,
  B00110,
};


int idlePeriod = 6000; // time between idle messages
unsigned long idleTimerMillis = 0;
int randomNumber = 0;


// ########## INIT BUTTONS ##########

const int selector[5] = {17, 16, 15, 14, 0 }; // input pins for selector buttons (A0=14, A1=15, A2=16, A3=17, Dig0=0)

// ### don't uses dig1 PIN because it is related to the serial monitor and will causes issues with external periphery

const int configbutton = 13;

// ########## INIT VALUES ##########
// #max
// #PREIS
int conveyorPrice[5] = {650, 650, 650, 650, 650}; // default price  
//int conveyorItems[5] = {1, 1, 1, 1, 1};
int conveyorItems[5] = {2, 2, 2, 2, 2};

// ########## INIT COIN ACCEPTOR ##########

const int coinInt = 0; // attach coinInt to interrupt pin 0 = digital pin 2 = digitalPinToInterrupt(2) . (interrupt pin 1 = digital pin 3 = digitalPinToInterrupt(3))

// set the coinsCurrentValue to a volatile float
// volatile as this variable changes any time the Interrupt is triggered
volatile int coinsCurrentValue = 0;
int coinsChange = 0; // a coin has been inserted flag

unsigned long currentMillis = 0;
unsigned long oldMillis = 0;
int pulsecount;

// const int relaisPin = xx; //

// ########## INIT Relais (8 Port, nr1 (pin 3) - nr5(pin 7) davon für Fachöffner, nr 6 für homematic) ##########
// ########## weitere Relaisleiste (8 Port, nr1 (pin 9) - nr5(pin 13) davon für Fachöffner der zweiten Reihe) ##########

// const int homematic_pin = 9;
// const int homematic_pin = 1;  // Dig pin 1
//const int homematic_relais_id = 5; //ist das 6. Relais auf der ersten Leiste
// #max

const int relays[10] = {3, 4, 5, 6, 7, 8, 9, 10 , 11, 12};

// ########## END OF INIT ##########

// ########## SETUP ##########

void setup() {

  Serial.begin(9600); // start serial communication
  Serial.println("Setup gestartet");

  pinMode(selector[0], INPUT_PULLUP);
  pinMode(selector[1], INPUT_PULLUP);
  pinMode(selector[2], INPUT_PULLUP);
  pinMode(selector[3], INPUT_PULLUP);
  pinMode(selector[4], INPUT_PULLUP);
  //pinMode(flapPin, INPUT_PULLUP);
  pinMode(configbutton, INPUT_PULLUP);
  //pinMode(relaisPin,OUTPUT);

  pulsecount = 0;

  // if (digitalRead(selector[2]) == LOW && digitalRead(selector[3]) == LOW) {
  //     testServos = 1;
  // }

  lcd.createChar(0, euro); // build € character

  lcd.begin(16, 2); // set up the LCD's number of columns and rows
  lcd.print("Bitte warten..."); // Print wait message to the LCD

  Serial.println("Warten auf Münzprüfer");
  delay(3000); // don't start main loop until we're sure that the coin selector has started

  // if coinInt goes HIGH (a Pulse), call the coinInserted function
  // an attachInterrupt will always trigger, even if your using delays
  //  attachInterrupt(digitalPinToInterrupt(2), coinInserted, RISING);
  attachInterrupt(coinInt, coinInserted, RISING);

// Relais initialisieren
  digitalWrite(relays[0], HIGH); // Turn OFF
  digitalWrite(relays[1], HIGH); // Turn OFF
  digitalWrite(relays[2], HIGH); // Turn OFF
  digitalWrite(relays[3], HIGH); // Turn OFF
  digitalWrite(relays[4], HIGH); // Turn OFF
 // digitalWrite(homematic_pin, HIGH); // Turn OFF

  digitalWrite(relays[5], HIGH); // Turn OFF
  digitalWrite(relays[6], HIGH); // Turn OFF
  digitalWrite(relays[7], HIGH); // Turn OFF
  digitalWrite(relays[8], HIGH); // Turn OFF
  digitalWrite(relays[9], HIGH); // Turn OFF

 pinMode(relays[0], OUTPUT);
  pinMode(relays[1], OUTPUT);
  pinMode(relays[2], OUTPUT);
  pinMode(relays[3], OUTPUT);
  pinMode(relays[4], OUTPUT);
// relais nr 6 / am homematic_pin: informs homematic about sold item
//  pinMode(homematic_pin, OUTPUT);
// #max
// Relais der zweiten Leiste für zweite Reihe der Fächer
  pinMode(relays[5], OUTPUT);
  pinMode(relays[6], OUTPUT);
  pinMode(relays[7], OUTPUT);
  pinMode(relays[8], OUTPUT);
  pinMode(relays[9], OUTPUT);

delay(1000);
coinsCurrentValue = 0;
  lcd.print("Guthaben loeschen.");
delay(3000);
  Serial.println("Bereit");
  lcd.clear();
  lcd.print("Bereit");
}

// ########## COIN INSERT ##########

// This function is called by interrupt every time we receives a pulse from the coin acceptor
void coinInserted() {
coinsChange = 1; // flag that there has been a coin inserted

  currentMillis = millis();
  int difference = currentMillis - oldMillis;

//  Serial.print("difference: ");
  Serial.println(difference); 

  oldMillis = currentMillis;
  pulsecount++;

// new coin? start to count from beginning....   
if (difference > 134 or difference < 50 ) {
    if (pulsecount > 1) {
       pulsecount = 1;
     }
   }

if (difference < 135) {
switch (pulsecount) {
case 2: coinsCurrentValue = coinsCurrentValue + 10;  
        break; 
case 3: coinsCurrentValue = coinsCurrentValue + 10;  
        break; 
case 4: coinsCurrentValue = coinsCurrentValue + 30;  
        break; 
case 5: coinsCurrentValue = coinsCurrentValue + 50;  
        break; 
case 6: coinsCurrentValue = coinsCurrentValue + 100;  
        break;         
   } // switch
  } // if
  
  Serial.print ("(/coinsInserted)neuer Wert: ");
  Serial.println (  coinsCurrentValue);
 
}

// ########## Open Compartement ##########

// Fach Öffnen
void CompartementOpen(int j) {
  Serial.print("Fachnummer ");
  Serial.print(j);
  Serial.println(" öffnen:");

  //hier relais ansteuern
  
  digitalWrite(relays[j], LOW); // Fach öffnen
  delay(1000);
  digitalWrite(relays[j], HIGH); // Relais wieder aus
}



// ########## LCD IDLE ##########

// messages on LCD while idle
void idle() {
  // only run when idlePeriod has been reached since last run and no coins have been thrown
  if ((millis() > idleTimerMillis + idlePeriod) && (coinsCurrentValue < 1)) {
    idleTimerMillis = millis();
    randomNumber = random(0, 5); // random number from 0 to 4
    lcd.clear();
    if (randomNumber == 0) {
      lcd.setCursor(0, 0);
 // #PREIS
      lcd.print(" 500g Honig");
      lcd.setCursor(0, 1);
      lcd.print("");
    } else if (randomNumber == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Bitte passend ");
      lcd.setCursor(0, 1);
      lcd.print("zahlen. ");
    } else if (randomNumber == 2) {
      lcd.setCursor(0, 0);
      lcd.print("Geld einwerfen, dann");
      lcd.setCursor(0, 1);
      lcd.print("Fach waehlen");
    } else if (randomNumber == 3) {
      lcd.setCursor(0, 0);
  // #PREIS
      lcd.print("500g Honig");
      lcd.setCursor(0, 1);
      lcd.print("   ");
    } else if (randomNumber == 4) {
      lcd.setCursor(0, 0);
      lcd.print("Leergut bitte");
      lcd.setCursor(0, 1);
      lcd.print("zurueck. Danke.");
    }
  }
  // clear lcd 1000 msec before idle periode expires and no coins have been thrown
  if ((millis() > idleTimerMillis + idlePeriod - 1000) && (coinsCurrentValue < 1)) {
    lcd.clear();
  }
}



// ########## SET VALUES ##########

void set_values() {
  Serial.println("Gerätekonfiguration gestartet");
 
 //#max hat keinen Einfluss, da es immer nur 5 Preise bzw. Reihen gibt
  for (int index = 0; index <= 4; index++) {
      // set conveyor items
    int scope = 0;
    int pass = index + 1;
    Serial.print("Anzahl");
    Serial.println(pass);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fachnr. ");
    lcd.print(pass);
    lcd.print(" Anzahl ");
    lcd.setCursor(0, 1);
    lcd.print(conveyorItems[index]);
    lcd.print("  ");


    // Setup Warenmenge Button 1 = runterzählen, Button 2 = raufzählen, Button 3 = Ende und Anzeige
    Serial.print("scope ");
    Serial.println(scope);
    Serial.print("Setup Warenmenge");

    Serial.print("digitalRead(selector[0] ");
    Serial.println (digitalRead(selector[0])) ;
    Serial.print("digitalRead(selector[1] ");
    Serial.println (digitalRead(selector[1])) ;
    Serial.print("digitalRead(selector[2] ");
    Serial.println (digitalRead(selector[2])) ;
    Serial.print("digitalRead(selector[3] ");
    Serial.println (digitalRead(selector[3])) ;

    Serial.print("while schleife Anzahl def beginnt");
    while (scope == 0) {
      if (digitalRead(selector[0]) == LOW && conveyorItems[index] > 0) {
        conveyorItems[index] = conveyorItems[index] - 1;
        lcd.setCursor(0, 1);
        lcd.print(conveyorItems[index]);
        lcd.print("   ");
        delay(200);
      }
      if (digitalRead(selector[1]) == LOW && conveyorItems[index] < 10) {
        conveyorItems[index] = conveyorItems[index] + 1;
        lcd.setCursor(0, 1);
        lcd.print(conveyorItems[index]);
        lcd.print("   ");
        delay(200);
      }
      if (digitalRead(selector[3]) == LOW) {
        Serial.print("Fach Reihe 1 öffnen ");
        
      CompartementOpen(index);
        delay(200);
      }
      if (digitalRead(selector[4]) == LOW) {
        Serial.print("Fach Reihe 2 öffnen ");
        
      CompartementOpen(index+5);
        delay(200);
      }

      if (digitalRead(selector[2]) == LOW) {
        Serial.print("Neue Anzahl: ");
        Serial.println(conveyorItems[index]);

        scope++;
        delay(200);
      }
    }

  }



  // Setup Preise Button 1 = runterzählen, Button 2 = raufzählen in 10er Schritten, Button 3 = Ende und Anzeige

  Serial.print("while schleife Preise beginnt");
  for (int index = 0; index <= 4; index++) {
    // sest conveyor prices
    int scope = 0;
    int pass = index + 1;
    Serial.print("Preis für Spalte ");
    Serial.println(pass);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fach ");
    lcd.print(pass);
    lcd.print(" Preis");
    lcd.setCursor(0, 1);
    lcd.write(byte(0)); // display EURO symbol
    lcd.print(" ");
    lcd.print(conveyorPrice[index] / 100.00);
    lcd.print("     ");
    while (scope == 0) {
      if (digitalRead(selector[0]) == LOW && conveyorPrice[index] >= 10) {
        conveyorPrice[index] = conveyorPrice[index] - 10;
        displayConfPrice(index);
        delay(200);
      }
      if (digitalRead(selector[0]) == LOW && conveyorPrice[index] > 0 && conveyorPrice[index] < 10) {
        conveyorPrice[index] = 0;
        displayConfPrice(index);
        delay(200);
      }
      if (digitalRead(selector[1]) == LOW && conveyorPrice[index] < 1000) {
        conveyorPrice[index] = conveyorPrice[index] + 10;
        displayConfPrice(index);
        delay(200);
      }
      if (digitalRead(selector[2]) == LOW) {
        Serial.print("Neuer Preis:  €");
        Serial.println(conveyorPrice[index] / 100.00);
        scope++;
        delay(200);
      }
    }
  }

 coinsCurrentValue = 0;
 displayBalance();

  
  Serial.println("Gerätekonfiguration beendet");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gespeichert");
  delay(1500);
  Serial.println("Warten auf Münzprüfer");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bitte warten...");
  delay(1500);
  displayBalance();
}

// ########## LCD MESSAGES ##########

void displayBalance() {
  Serial.print("Guthaben:  ");
  Serial.println(coinsCurrentValue / 100.00);
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print("Guthaben");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)
  lcd.write(byte(0)); // display EURO symbol
  lcd.print(" ");
  lcd.print(coinsCurrentValue / 100.00); // display current balance
}

void displayPrice(int currentPrice) {
  Serial.print("Warenpreis: € ");
  Serial.println(currentPrice / 100.00);
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print("Preis");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)
  lcd.write(byte(0)); // display EURO symbol
  lcd.print(" ");
  lcd.print(currentPrice / 100.00);
  if (coinsCurrentValue > 0) {
    delay(1000);
    displayBalance();
  }
}

void displayEmpty() {
  Serial.println("Gewähltes Fach leer");
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print("Leider leer :(");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)
  lcd.print("Bitte neu w" AE "hlen");
  if (coinsCurrentValue > 0) {
    delay(1000);
    displayBalance();
  }
}

void displayConfPrice(int con) {
  lcd.setCursor(0, 1);
  lcd.write(byte(0)); // display EURO symbol
  lcd.print(" ");
  lcd.print(conveyorPrice[con] / 100.00);
  lcd.print("     ");
}


// ########## MAIN LOOP ##########

void loop() {
  if (digitalRead(configbutton) == LOW) {
    set_values();
  }
  //  Serial.println("vor Idle");
  idle();
  //  Serial.println("nach Idle");


  // disable coin acceptor when there are no more items left in stock
  //if (conveyorItems[0] + conveyorItems[1] + conveyorItems[2] + conveyorItems[3] + conveyorItems[4] == 0) {
  //  digitalWrite(relaisPin,HIGH);
  //} else {
  //  digitalWrite(relaisPin,LOW);
  //}


  // check if a coin has been inserted
 if (coinsChange == 1) {
   coinsChange = 0; // unflag that a coin has been inserted
   displayBalance(); // display current balance
  
  }

  // ********** BUTTON PRESSED **********



  for (int index = 0; index <= 4; index++) {
    if (digitalRead(selector[index]) == LOW) { // button pressed
      Serial.print("Spalte ");
      Serial.print(index + 1);
      Serial.println(" gedrückt");
      if (conveyorItems[index] > 0) {
        if (coinsCurrentValue < conveyorPrice[index]) { // insufficient balance - display price
          displayPrice(conveyorPrice[index]);
          idleTimerMillis = millis();
          delay(200);
        }
        if ((coinsCurrentValue > conveyorPrice[index] - 1) ) { // sufficient balance
          coinsCurrentValue = coinsCurrentValue - conveyorPrice[index]; // reduce balance
          conveyorItems[index]--; // reduce items
          if (coinsCurrentValue < 0) {
            coinsCurrentValue = 0;  // correct float rounding error
          }
          delay(200);
   
//          hier gehts weiter - welches Fach öffnen: oben oder unten, der Zähler wurde schon reduziert

    Serial.print("conveyorItems[index] ");
      Serial.print(conveyorItems[index]);
      Serial.print("index ");
      Serial.print(index);

          if ( conveyorItems[index] == 0) {
          Serial.print("index öffnen ");
          CompartementOpen(index); }
          
          if ( conveyorItems[index] == 1) {
          Serial.print("index plus fuenf öffnen ");
          CompartementOpen(index + 5); }

          displayBalance(); // display current balance
          
          idleTimerMillis = millis(); // reset idle timer
//          inform homematic about sold item
//            digitalWrite(homematic_pin, LOW); 
//            delay(1000);
//            digitalWrite(homematic_pin, HIGH); 
//            delay(200);
        }
      } else {
        displayEmpty();
        idleTimerMillis = millis(); // reset idle timer
        delay(200);
      }
    }
  
  // delay(200);
  //Serial.print("*");
 // pulsecount=0;
  
  
  }
 

}
