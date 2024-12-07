#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "SoftwareSerial.h"
#include <Keypad.h>     // Bibliothek für das Keypad
#include <Keypad_I2C.h> // Bibliothek für das Keypad über I2C

#define PIN_TX 51
#define PIN_RX 53
// ######### INIT SIM / SMS #########
// Cell phone number incl. country code "+49" for Germany and xxxxxxxxxxx = phone number
//   don't forget to change your phone number: !!!!!!!!!!!!!!
const char TELEFONE_NUMBER[] = "+xxxxxxxxxxxx"  ;
// 
bool sim = false;
bool refill_ongoing = false;

SoftwareSerial mySerial(PIN_TX, PIN_RX);

// ########## INIT DISPLAY ##########

// LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address UNO
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address MEGA2560 ; PIN 20 SDA, PIN 21SCL 
//                 or 0x3F correct? see https://www.youtube.com/watch?v=B8DNokj9LnY


// ############ specific configurations ####################

// max = number of buttons (default: 5; even with 1,2 or 3 rows of 5 compartments); if you want to have a button for each compartment increase up to 15 or more
// don't forget to look at EEPROM structure when changing value of max
const int max = 10; 
// number of rows (1 = 5 compartments, 2 = 10 compartments, 3 = 15 compartments)
// if maxrow <=2 the button #5 is used to start refill programm otherwise for open 3rd row during programming
const int maxrow = 1;

unsigned long idlePeriod = 120000; // time in ms between idle messages or shutdown e.g. 180000  
// powersave = 0 show text when Idle; powersave = 1 shutdown when IdlePeriod reached

// debug modus: if you need 10 vendors and one additonal PIN (TX0 / Digital PIN 1 which should normally not be used) on UNO turn debug mode to 0 which disables serial 
// I use this pin to send a message to my smarthome system for any purchased product
bool debug = true;
 //          if (!debug) {
          const int homematic_pin = 6;   
          const int homematic_pin_empty = 7;
 //          }
unsigned long idleTimerMillis = 0;
int randomNumber = 0;

// ### sms_available_products = 0 to send SMS only when sold out, otherwise 99
// sms_change_thresholds :   "threshold=0", "threshold=3", "threshold=99" (send this via sms, works until restart)
 
int sms_available_products = 3;
int last_available_products = 99;

const int buzzer_pin = 49;


// #### parameter for Paypal payment (QR-code incl. value to be generated via Paypal mobile app)
// email forward to sms including following string: 
//const char smsPayPal7       = "Sie haben 7,00 EUR erhalten";
const int  smsPayPal7value  = 700;

// ox cashless payment
bool ox = false;
int ox_credit_per_pulse = 100;
// volatile int ox_coinsCurrentValue = 0;
volatile int ox_difference = 0; 
int ox_Change = 0; // a coin has been inserted flag
unsigned long ox_currentMillis = 0;
unsigned long ox_oldMillis = 0;
volatile int ox_pulsecount = 0;
int ox_inhibit_relais_pin = 11;
int ox_act = 12;


// nv10 cashless payment
// NV10 - payment with bill now available for Mega 2560

// DIPs on NV10:
// 1 LOW
// 2 HIGH (this has been changed recently!)
// 3 LOW
// 4 HIGH
// DIP2 on HIGH to get 4 pulses instead of 1 because in some installations compartment opening impacted the pulse interrupt. Now all pulses less 4 are ignored.
// credit per pulse is now 125 instead of 500

bool nv10 = false;
int nv10_credit_per_pulse = 125;
// volatile int nv10_coinsCurrentValue = 0;
volatile int nv10_difference = 0; 
int nv10_Change = 0; // a coin has been inserted flag
unsigned long nv10_currentMillis = 0;
unsigned long nv10_oldMillis = 0;
volatile int nv10_pulsecount = 0;
int nv10_act = 13;
int nv10_ch1 = 9;
int nv10_ch2 = 10;

char last_paytype = '?';

// ########## keypad usage ##########
// in case of keypad usage: set maxrow = 1 and max to number of vending units!
// it is not possible to use select buttons and keypad in parallel.
// each vending unit can have a specific price
bool keypad = false;
const byte ROWS = 4;  
const byte COLS = 3;   
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {0, 1, 2, 3}; // connection to rows of keypads (PCF8574 P0-P3)
byte colPins[COLS] = {4, 5, 6};    // columns (PCF8574 P4-P6)

Keypad_I2C customKeypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, 0x20); // 0x20 is the I2C Address des PCF8574 (changeable, you find it with help of sketch "i2c_scanner")

unsigned long lastKeyPressTime = 0;
int knumber = 0;
int select = 999;
bool singleDigitEntered = false;
bool doubleDigitEntered = false;
bool tripleDigitEntered = false;

// ########## INIT BUTTONS ##########

//const int selector[5] = {17, 16, 15, 14, 0 }; // input pins for selector buttons (A0=14, A1=15, A2=16, A3=17, Dig0=0) UNO
//const int selector[15] = {A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15}; // input pins for selector buttons MEGA
const int selector[15] = {A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15}; // input pins for selector buttons MEGA
//const int selector[5] = {A1, A2, A3, A4, A5}; // input pins for selector buttons MEGA 5 buttons and 5/10 or 15 boxes

// ### don't uses dig1 PIN because it is related to the serial monitor and will causes issues with external periphery (UNO)
// PINS
const int configbutton = 5;   // 5 for Mega; 13 for Uno
const int refillbutton = 3;  // 3 for Mega; for Uno 12 (if only one row of compartments exists: maxrow = 1)
const int powersave_relais_pin = 4;  // 4 for Mega; for Uno 11
const int powersave = 0;  // powersave = 1 -> turnoff power after idle threshold; run at least onetime with 0 to ensure correct data on eeprom

// #### EEPROM structure
//  0          -> 1           : version of this structure : 1 
//  2          -> 3           : max = number of buttons
//  4          ->            
//  index * 2 + 4             : conveyorItems []
//  index * 2 + max * 2 + 4   : conveyorPrice []

const int EEPROM_version = 1;
 

// ########## INIT VALUES ##########
// #max
// #PREIS
int conveyorPrice[15] = {700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700}; // default price  
//int conveyorPrice[5] = {700, 700, 700, 700, 700}; // default price  
int conveyorItems[15] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
//int conveyorItems[5] = {2, 2, 2, 2, 2};

// ########## INIT COIN ACCEPTOR ##########

const int coinInt = 0; // attach coinInt to interrupt pin 0 = digital pin 2 = digitalPinToInterrupt(2) . (interrupt pin 1 = digital pin 3 = digitalPinToInterrupt(3))
const int coin_relais_pin = 17;
// ########## INIT Cashless payment ox interrupt ##############
const int ox_Int = 18; // attach coinInt to interrupt pin 
// ########## INIT bill payment nv10 interrupt ##############
const int nv10_Int = 19; // attach nv10 to interrupt pin 

// set the coinsCurrentValue to a volatile float
// volatile as this variable changes any time the Interrupt is triggered
volatile int coinsCurrentValue = 0;
int coinsChange = 0; // a coin has been inserted flag

unsigned long currentMillis = 0;
unsigned long oldMillis = 0;

int pulsecount;

const unsigned int wt_ms = 100; // wait ms

// const int relaisPin = xx; //
//const int relays[10] = {3, 4, 5, 6, 7, 8, 9, 10 , 11, 12}; //uno: 10 boxes
const int relays[15] = {22,24,26,28,30,32,34,36,38,40,42,44,46,48,50}; // mega: 15 boxes
 

//const int poweroffrelais = 4;  // mega: 4; uno: only possible if less 10 compartments; in this case pin 12 recommended; remove this pin from relais list
// poweroffrelais replaced by powersave_relais_pin
// ########## END OF INIT ##########

// ########## SETUP ###################################################################################

void setup() {



if (debug) {
  Serial.begin(9600); // start serial communication
  Serial.println("Setup gestartet");
  }
 for (int index = 0; index < max; index++) {
   pinMode(selector[index], INPUT_PULLUP);
 }
 
  pinMode(configbutton, INPUT_PULLUP);
  pinMode(refillbutton, INPUT_PULLUP);  
  pulsecount = 0;
  digitalWrite(powersave_relais_pin, HIGH);
  pinMode (powersave_relais_pin, OUTPUT);

  digitalWrite(homematic_pin, HIGH);
  pinMode (homematic_pin, OUTPUT);

  digitalWrite(homematic_pin_empty, HIGH);
  pinMode (homematic_pin_empty, OUTPUT);

  digitalWrite(buzzer_pin, LOW);
  pinMode (buzzer_pin, OUTPUT);

 
   /// #### start coin acceptor & deactivate ox_inhibit
  // payment_on_off ();
      
 // lcd.begin(20, 4); // set up the LCD's number of columns and rows
  lcd.begin(16, 2); // set up the LCD's number of columns and rows
  lcd.print("Bitte warten..."); // Print wait message to the LCD
   if (debug) {
  Serial.println("Warten auf Muenzpruefer");
  }
  delay(500); // don't start main loop until we're sure that the coin selector has started

  lcd.clear();
  lcd.setCursor(0, 0); 

  // if coinInt goes HIGH (a Pulse), call the coinInserted function
  // an attachInterrupt will always trigger, even if your using delays
  //  attachInterrupt(digitalPinToInterrupt(2), coinInserted, RISING);
  attachInterrupt(coinInt, coinInserted, RISING);
   
  pinMode(ox_act, INPUT_PULLUP);
  if (digitalRead(ox_act) == LOW) {ox = true;} else {ox = false;}

if (debug) {
 if (ox) {
    Serial.print(" ox true ");
    }
    else
    {
    Serial.print(" ox false ");
    }
}     

if (ox) {
 ox_oldMillis = millis();
 if (debug) {
      Serial.print(" ox_oldMillis ");
      Serial.print(ox_oldMillis);
 }
 
}

  pinMode(nv10_act, INPUT_PULLUP);
  if (digitalRead(nv10_act) == LOW) {nv10 = true;} else {nv10 = false;}

if (debug) {
 if (nv10) {
    Serial.print(" nv10 true ");
    }
    else
    {
    Serial.print(" nv10 false ");
    }
}   
 
if (nv10) {
 nv10_oldMillis = millis();
 if (debug) {
      Serial.print(" nv10_oldMillis ");
      Serial.print(nv10_oldMillis);
 }
 
}
if (sim)
   {
    lcd.setCursor(0, 1);
  lcd.print("SIM init..10sek "); 
     delay(10000);
   // Serial.begin(9600);
	digitalWrite(buzzer_pin, HIGH); 
  delay(250);
  digitalWrite(buzzer_pin, LOW); 
  
  mySerial.begin(9600);


	Serial.println("Initializing sim..");
  mySerial.println("AT");
  updateSerial(wt_ms);
  Serial.println("AT+CMGF=1");
  mySerial.println("AT+CMGF=1"); // SMS text mode
  updateSerial(wt_ms);  

//## how to handle incoming SMS:
//### display sms
mySerial.println("AT+CNMI=1,2,0,0,0"); // Decides how newly arrived SMS messages should be handled
// save sms
//  mySerial.println("AT+CNMI=2,1,0,0,0"); // Decides how newly arrived SMS messages should be handled
  updateSerial(wt_ms);
  delay(1000);
  

  Serial.println(" AT+CMGL:");
 // mySerial.println("AT+CMGL=\"ALL\"\r"); // show all sms stored in SIM-card
 // updateSerial(wt_ms);
 //Serial.println(" del all sms:");
 //mySerial.println("AT+CMGL=\"DEL ALL\"\r"); // delete all sms from sim-card
 // updateSerial(wt_ms);
 Serial.println(" weiter..");
  updateSerial(wt_ms);
  delay(1000);

  last_available_products = sms_available_products;

   }

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("ox "); 
  lcd.print(ox);  
  lcd.print(" nv10 "); 
  lcd.print(nv10);  
  lcd.print(" sim "); 
  lcd.print(sim);  
  lcd.setCursor(0, 0);
//  lcd.print(" av "); 
 // lcd.print(available_products);  
  lcd.print(" mx "); 
  lcd.print(max);  
  lcd.print(" mxr "); 
  lcd.print(maxrow);  
  delay(3000); 

if (keypad) {
lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("key "); 
  lcd.print(keypad);  
 if (debug) {
      Serial.print(" keypad active! ");
       }
  customKeypad.begin();
  Wire.begin();
delay(200);
}


// Relais initialisieren

 for (int index = 0; index < (max * maxrow); index++) {
  digitalWrite(relays[index], HIGH); // Turn OFF
  pinMode(relays[index], OUTPUT);
 }

digitalWrite(powersave_relais_pin,HIGH);
 pinMode(powersave_relais_pin, OUTPUT);

 if ((readEEPROM (0) != EEPROM_version) or (readEEPROM (2) != max))
 {
   if (debug) {
    Serial.print("EEPROM_vers ");

    Serial.print (readEEPROM (0)); 
    Serial.print("Emax ");
    Serial.print (readEEPROM (2)); 
    Serial.print("PVer: ");
    Serial.print(EEPROM_version);  
    Serial.print("PrMax: "); 
    Serial.println(max);   
   }    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INIT EEPROM ");
    lcd.setCursor(0, 1);    
    lcd.print(" v ");
    lcd.print( EEPROM_version);
  //  lcd.print(" max ");    
  //  lcd.print( max );   
  if (debug) {
      Serial.print("writeEEprom.. ");     
  }      
   writeEEPROMcomplete();
   if (debug) {
    Serial.print("ende IF ");
   }    
 } 
 else
 {
// read EEPROM and update values
if (debug) {
  Serial.println(" else:read EEPROM and upd ");
}  
 readEEPROMcomplete();
   
 }
   
 //delay(200);
coinsCurrentValue = 0;
lcd.print("Guthaben reset");
delay(200);
if (debug) {
  Serial.println("Bereit");
}  
  lcd.clear();
  lcd.print("Bereit.");
 // if coinInt goes HIGH (a Pulse), call the coinInserted function
  // an attachInterrupt will always trigger, even if your using delays
  //  attachInterrupt(digitalPinToInterrupt(2), coinInserted, RISING);
  attachInterrupt(coinInt, coinInserted, RISING);

  payment_on_off ();
}

// ########## COIN INSERT ##########

// This function is called by interrupt every time we receives a pulse from the coin acceptor
void coinInserted() {
coinsChange = 1; // flag that there has been a coin inserted
last_paytype = 'C';


  currentMillis = millis();
  int difference = currentMillis - oldMillis;

//  Serial.print("difference: ");
if (debug) {
  Serial.println(difference); 
}
  oldMillis = currentMillis;
  pulsecount++;

// new coin? start to count from beginning....   
if (difference > 140 or difference < 50 ) {
    if (pulsecount > 1) {
       pulsecount = 1;
     }
   }

if (difference < 141) {
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
    idleTimerMillis = millis();
if (debug) {    
  Serial.print ("(/coinsInserted)neuer Wert: ");
  Serial.println (  coinsCurrentValue);
  Serial.print("last Paytype: ");
  Serial.println (last_paytype);
}
}


// ########## OX PULSE - cashless payment ##########

// This function is called by interrupt every time we receives a pulse from the coin acceptor
void ox_pulse() {
ox_Change = 1; // flag that there has been cashless payment done
last_paytype = 'O';

ox_pulsecount++;
ox_currentMillis = millis();
 int ox_difference = ox_currentMillis - ox_oldMillis;
     
  ox_oldMillis  = ox_currentMillis; 

if (debug) {    
  Serial.print ("(/ox_pulse) neuer Wert: ");
  Serial.println (  ox_pulsecount);
}

// new ox payment? start to count from beginning
 if (ox_difference > 310)
 {
   if (ox_pulsecount > 1)
   {
     ox_pulsecount = 1;
   }
 } // diff > xxx

 if ((ox_difference < 309) and (ox_difference > 0))
 {


  // {
     coinsCurrentValue = coinsCurrentValue + ox_pulsecount * ox_credit_per_pulse;
     ox_pulsecount = 0;
   }
 //}
if (debug) {    
  Serial.print ("(/ox_pulse) coinsCurrentValue ");
  Serial.println (  coinsCurrentValue);
  Serial.print ("(/ox_pulse) ox_difference: ");
  Serial.println (  ox_difference);
  Serial.print("last Paytype: ");
  Serial.println (last_paytype);
}

}
// ################ ox back - inform Naxay about cash payments #############################
void ox_pulse_back(int oxCurrentValue ) {
 
if (debug) {    
  Serial.print ("(oxCurrentValue: ");
  Serial.println (  oxCurrentValue );
  Serial.print ("ox_credit_per_pulse: ");
  Serial.print (ox_credit_per_pulse );
}
// now we want to use the ox pin for output for a moment...
   detachInterrupt(digitalPinToInterrupt(ox_Int));   
   delay(200);
   pinMode (ox_Int, OUTPUT);
   digitalWrite(ox_Int, HIGH);
   delay(100);
   digitalWrite(ox_Int, LOW);
   delay(500);
 
// ### back to normal usage of ox-pin
   delay (200);
   pinMode(ox_Int, INPUT_PULLUP);
   delay (1000);
   attachInterrupt(digitalPinToInterrupt(ox_Int), ox_pulse, RISING);

 if (debug) {    
  Serial.print ("(/ox_pulse_back end)");  
  }

}


// ########## nv10 PULSE - bill payment ##########

// This function is called by interrupt every time we receives a pulse from the bill acceptor
void nv10_pulse() {
nv10_Change = 1; // flag that there has been cashless payment done
last_paytype = 'N';

nv10_pulsecount++;
nv10_currentMillis = millis();
 int nv10_difference = nv10_currentMillis - nv10_oldMillis;
     
nv10_oldMillis  = nv10_currentMillis; 

if (debug) {    
  Serial.print ("(/nv10_pulse) neuer Wert: ");
  Serial.println (  nv10_pulsecount);
  Serial.print ("(/nv10_pulse) nv10_difference: ");
  Serial.println (  nv10_difference);
  Serial.print("last Paytype: ");
  Serial.println (last_paytype);
}

// new bill? start to count from beginning
 if (nv10_difference > 310)
 {
   if (nv10_pulsecount > 1)
   {
     nv10_pulsecount = 1;
   }
 } // diff > 140

 if (nv10_difference < 309)
 {

switch (nv10_pulsecount) {
case 4: coinsCurrentValue  = coinsCurrentValue + nv10_pulsecount * nv10_credit_per_pulse;  
        nv10_pulsecount    = 0;
        break; 
case 8: coinsCurrentValue  = coinsCurrentValue + nv10_pulsecount * nv10_credit_per_pulse;  
        nv10_pulsecount    = 0;
        break; 
case 12: coinsCurrentValue = coinsCurrentValue + nv10_pulsecount * nv10_credit_per_pulse;
        nv10_pulsecount    = 0;
        break; 
   } // switch

 }
if (debug) {    
  Serial.print ("(/nv10_pulse) coinsCurrentValue ");
  Serial.println (  coinsCurrentValue);
  Serial.print ("(/nv10_pulse) nv10_difference: ");
  Serial.println (  nv10_difference);
}
}
 
// ########## Open Compartement ##########

// Fach Öffnen
void CompartementOpen(int j) {

// detach interrupts to avoid impact (false ccountings)

if (ox)   
 {
   detachInterrupt(digitalPinToInterrupt(ox_Int));   
    } // ox
if (nv10) 
    {
    detachInterrupt(digitalPinToInterrupt(nv10_Int));
    digitalWrite(nv10_ch1, HIGH);
    pinMode (nv10_ch1, OUTPUT);
    digitalWrite(nv10_ch2, HIGH);
    pinMode (nv10_ch2, OUTPUT);
    }

  detachInterrupt(coinInt);
 if (debug) {
  Serial.print("Fachnummer ");
  Serial.print(j+1);
  Serial.println(" oeffnen:");
 }
 
 // lcd.clear();
 // lcd.setCursor(0, 0);  
 // lcd.print("Bitte warten.");
  delay(100);
 
  digitalWrite(relays[j], LOW); // Fach öffnen
  delay(500);
  digitalWrite(relays[j], HIGH); // Relais wieder aus
 delay(1000);
  attachInterrupt(coinInt, coinInserted, RISING);  
// attach interrupts again if products are still available 
if (!refill_ongoing)
{
 payment_on_off();
}
}


// ############## payment system on or off depending on if products still available
 
 void payment_on_off () {
   if (debug) {
    Serial.print("payment_on_off ");
   }    
  
 int available_products = 0;
for (int avail_index = 0; avail_index < max; avail_index++) {
 available_products = available_products + conveyorItems[avail_index];
  
  }   
 if (debug)  
   {
    Serial.print("anz avail ");
    Serial.print(available_products);
   }  
 
  //lcd.clear();
  lcd.setCursor(14, 0);
  lcd.print(" av "); 
  lcd.print(available_products);  
  //delay (2000);
 
 if (available_products > 0)
  {
   lcd.clear();
   lcd.setCursor(0, 0);  
   lcd.print("Bitte warten.");
 //  delay(100);

   digitalWrite(coin_relais_pin,HIGH);
   pinMode(coin_relais_pin, OUTPUT);
   
 //  delay (500);
if (ox)    {
    if (debug) {
      Serial.print("attachOX ");
      }    
   digitalWrite(ox_inhibit_relais_pin,HIGH);
   pinMode(ox_inhibit_relais_pin, OUTPUT);
   attachInterrupt(digitalPinToInterrupt(ox_Int), ox_pulse, RISING);
      } // ox
if (nv10) {
   if (debug) {
      Serial.print("attachNV10 ");
      }    
    
   digitalWrite(nv10_ch1, LOW);
   pinMode (nv10_ch1, OUTPUT);
   digitalWrite(nv10_ch2, LOW);
   pinMode (nv10_ch2, OUTPUT);
   attachInterrupt(digitalPinToInterrupt(nv10_Int), nv10_pulse, RISING);
   if (debug) {
      Serial.print("attachNV10 done");
      }    
   
   }

  displayBalance(); // display current balance      

          if (sim)
          {

           if ((last_available_products == available_products) && ( (available_products == 3) || (available_products == 0) )
            || ((sms_available_products == 99) && (available_products > 3)))
           {
          updateSerial(wt_ms);
          mySerial.println("AT+CMGF=1");
          updateSerial(wt_ms);
          if (debug) {    
           Serial.print("AT+CMGF=1 ");    
          }
          if (debug) {    
           Serial.println("AT+CMGS=\""+String(TELEFONE_NUMBER)+"\"");    
           }
	         mySerial.println("AT+CMGS=\""+String(TELEFONE_NUMBER)+"\"");
	         updateSerial(wt_ms);
          if (debug) {    
          Serial.print("sms prep sold item ");  
          Serial.print(available_products);
      }
        mySerial.print("noch verfuegbar: " + String(available_products));
        updateSerial(wt_ms);
	      mySerial.write(26);
        mySerial.println("");
        updateSerial(wt_ms);
        last_available_products = available_products;
        }
          }



   } //if available_products > 0
 else  //available_product == 0
 {   
   if (ox)   
   {
   detachInterrupt(digitalPinToInterrupt(ox_Int));   
   digitalWrite(ox_inhibit_relais_pin,LOW);
   pinMode(ox_inhibit_relais_pin, OUTPUT);
   
 //  delay (1000);   
    } // ox
    if (nv10) {
    detachInterrupt(digitalPinToInterrupt(nv10_Int));
    digitalWrite(nv10_ch1, HIGH);
    pinMode (nv10_ch1, OUTPUT);
    digitalWrite(nv10_ch2, HIGH);
    pinMode (nv10_ch2, OUTPUT);
    
    }


// ##### inform smart home that no further product available
          digitalWrite(homematic_pin_empty, LOW); 
          delay(1500);
          digitalWrite(homematic_pin_empty, HIGH); 
          delay(200);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ware leer! ");
    delay (5000);
   digitalWrite(coin_relais_pin,LOW);
   pinMode(coin_relais_pin, OUTPUT);       

 if (sim)
  {
// Configuring TEXT mode
	 updateSerial(wt_ms);
   mySerial.println("AT+CMGF=1");
   updateSerial(wt_ms);
  if (debug) {    
     Serial.print("AT+CMGF=1 ");    
     }
  if (debug) {    
     Serial.println("AT+CMGS=\""+String(TELEFONE_NUMBER)+"\"");    
     }
	mySerial.println("AT+CMGS=\""+String(TELEFONE_NUMBER)+"\"");
	updateSerial(wt_ms);
    if (debug) {    
     Serial.print("sms prep ");  
     Serial.print(available_products);
     }
   mySerial.print("ausverkauft!");
	 updateSerial(wt_ms);
	 // following is not length of string, it writes "STR-Z" which is needed to send sms:
   mySerial.write(26);
   mySerial.println("");
   updateSerial(wt_ms);
  }

 }  // else
  
 }

// ########## LCD IDLE ##########

// messages on LCD while idle
void idle() {
  // only run when idlePeriod has been reached since last run and no coins have been thrown
  
  if ((millis() > idleTimerMillis + idlePeriod) && (coinsCurrentValue > 1) ) {
   
     idleTimerMillis = millis();     
     coinsCurrentValue = 0;
     lcd.print("Guthaben reset...");
     delay(200);
     lcd.clear();
     lcd.print("Bereit..");
     payment_on_off ();    
//     displayBalance(); // display current balance      
     
  }
    
  if ((millis() > idleTimerMillis + idlePeriod) && (coinsCurrentValue < 1) ) {
  // idle display text messages only if not in powersave mode
   if (powersave == 0)
 {   

    idleTimerMillis = millis();
    randomNumber = random(0, 5); // random number from 0 to 4
    lcd.clear();
    if (randomNumber == 0) {
      lcd.setCursor(0, 0);
 // #PREIS
      lcd.print("Preis");
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
      lcd.print("Willkommen!");
      lcd.setCursor(0, 1);
      lcd.print("   ");
    } else if (randomNumber == 4) {
      lcd.setCursor(0, 0);
      lcd.print("Leergut bitte");
      lcd.setCursor(0, 1);
      lcd.print("zurueck. Danke.");
    }
 
   // clear lcd 1000 msec before idle periode expires and no coins have been thrown
  if ((millis() > idleTimerMillis + idlePeriod - 1000) && (coinsCurrentValue < 1)) {
    lcd.clear();
  }
 
 }
else
 {
// shutdown when powersave == 1
  if (debug) {
  Serial.print("power off..");
  }  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("power off...");
  delay(2000);
   digitalWrite(powersave_relais_pin, LOW);
  delay(5000);  
  // power relais not available?... continue..
  idleTimerMillis = millis();
  lcd.clear();
  digitalWrite(powersave_relais_pin, HIGH);
  lcd.print("Bereit..");
  delay(2000);
 }
  }

}


// ########## SET VALUES ##########

void set_values() {
  if (debug) {
  Serial.println("Gerätekonfiguration gestartet");
  }
  

  
if (sim) {
    digitalWrite(buzzer_pin, HIGH); 
    delay(250);
    digitalWrite(buzzer_pin, LOW); 
      last_available_products = sms_available_products;
      lcd.clear();
      lcd.setCursor(0, 0);
      if (debug) {
       Serial.println(" set sms_available_products: ");
       Serial.println(sms_available_products);
               }
       lcd.print("sms ab Anzahl: ");
       lcd.print(sms_available_products);
       delay(500);
        if (keypad)
           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.setCursor(0, 1);
          {lcd.print("*=open #=weiter");
          }
       delay(1000);
    }


    int scope = 0;
for (int index = 0; index < max; index++) {
     idleTimerMillis = millis();
      // set conveyor items
    scope = 0;      
    int pass = index + 1;
   if (debug) {
    Serial.print("Anz");
    Serial.println(pass);
   }    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fachnr. ");
    lcd.print(pass);
    lcd.print(" Anzahl ");
    lcd.setCursor(0, 1);
    lcd.print(conveyorItems[index]);
    lcd.print("  ");


    // Setup Warenmenge Button 1 = runterzählen, Button 2 = raufzählen, Button 3 = weiter, Button 4 = alle öffnen, Button 5 = refill (öffne alle leeren & setzte Werte)
if (debug) {
    Serial.print("scope ");
    Serial.println(scope);
   
 }    
   // set of product counter only without keypad (with keypad maxrow is always one)
while ((scope == 0) && (keypad == false)) { 
//while (scope == 0) {
      if (digitalRead(selector[0]) == LOW && conveyorItems[index] > 0) {
        conveyorItems[index] = conveyorItems[index] - 1;
 //       lcd.clear();        
        lcd.setCursor(0, 1);
        lcd.print(conveyorItems[index]);
        lcd.print("   ");
        delay(200);
      } //selector [0]
      if (digitalRead(selector[1]) == LOW && conveyorItems[index] < 10) {
        conveyorItems[index] = conveyorItems[index] + 1;
 //       lcd.clear();        
        lcd.setCursor(0, 1);
        lcd.print(conveyorItems[index]);
        lcd.print("   ");
        delay(200);
      } //selector [1]
      
      if (digitalRead(selector[3]) == LOW) {
if (debug) {
        Serial.print("alle oeffnen ");
}
        //if (maxrow == 1) {
           CompartementOpen(index);          
           delay(500);           
        //   } //maxrow == 1
        if (maxrow > 1) {
           CompartementOpen(index+max);          
           delay(500);
           } //maxrow ==2
        if (maxrow > 2) {
           CompartementOpen(index+max+max);          
           delay(500);           
           } // maxrow ==3

           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.print("Fach geoeffnet.");
           lcd.setCursor(0, 1);
           lcd.print("(3) druecken!");
           delay(200);

      } //selector [3]
       
      if (digitalRead(selector[4]) == LOW) {
 if (debug) {
        Serial.print("refill.. ");
 }        
           refill();  
           writeEEPROMcomplete();           
           scope = max + 1;           
           delay(200);
      } //selector [4]


      if (digitalRead(selector[2]) == LOW) {
if (debug) {        
        Serial.print("Neue Anz: ");
        Serial.println(conveyorItems[index]);
}
        scope++;
        delay(400);
      } //selector [2]
    } // while (scope == 0) and keypad not used

  } //for (int index = 0;

  // Setup Preise Button 1 = runterzählen, Button 2 = raufzählen in 10er Schritten, Button 3 = Ende und Anzeige
if (debug) {
  Serial.print("while Prs ");
}  
if (scope <= max)
 {
  for (int index = 0; index < max; index++) {
    // set conveyor prices with buttons
    scope = 0;
    int pass = index + 1;
 if (debug) {
    Serial.print("Preis Fach ");
    Serial.println(pass);
 }    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fach ");
    lcd.print(pass);
    lcd.print(" Preis");
    lcd.setCursor(0, 1);
    
    lcd.print(" ");
    lcd.print(conveyorPrice[index] / 100.00);
    lcd.print("     ");
    while ((scope == 0) and (!keypad)) {
      if (digitalRead(selector[0]) == LOW && conveyorPrice[index] >= 10) {
// enter value up/down via button 1 or 2
        conveyorPrice[index] = conveyorPrice[index] - 10;
        displayConfPrice(index);
        delay(300);
      }
      if (digitalRead(selector[0]) == LOW && conveyorPrice[index] > 0 && conveyorPrice[index] < 10) {
        conveyorPrice[index] = 0;
        displayConfPrice(index);
        delay(300);
      }
      if (digitalRead(selector[1]) == LOW && conveyorPrice[index] < 2000) {
        conveyorPrice[index] = conveyorPrice[index] + 10;
        displayConfPrice(index);
        delay(300);
      }
      if (digitalRead(selector[2]) == LOW) {
// -- next entry via button 3 
  if (debug) {
        Serial.print("Neuer Preis:  ");
        Serial.println(conveyorPrice[index] / 100.00);
  }        
        scope++;
        delay(400);
   }
      } // while scope and not keypad

// ########   enter price with help of keypad
  while ((scope == 0) and (keypad))

  {

   if (keypad) {
      char key = customKeypad.getKey();
 
      if (key) {
      if (debug) {
      Serial.print("pr_keypad_2 ");
      }    
    unsigned long currentTime = millis();
    digitalWrite(buzzer_pin, HIGH); 
    delay(150);
    digitalWrite(buzzer_pin, LOW); 

if (tripleDigitEntered && (currentTime - lastKeyPressTime <= 1000)) {
      if (debug) {
    Serial.print("pr_four digits entered! ");
   }    
      // 4th press within short time
       knumber = knumber * 10 + (key - '0');
      singleDigitEntered = false;
      doubleDigitEntered = false;
      tripleDigitEntered = false;
      Serial.println(knumber);
    }
 else {

if (doubleDigitEntered && (currentTime - lastKeyPressTime <= 1000)) {
      if (debug) {
    Serial.print("pr_three digits entered! ");
   }    
      // third press within short time
       knumber = knumber * 10 + (key - '0');
      singleDigitEntered = false;
      doubleDigitEntered = false;
      tripleDigitEntered = true;
      Serial.println(knumber);
    }
 else {
 
 
    if (singleDigitEntered && (currentTime - lastKeyPressTime <= 1000)) {
      if (debug) {
    Serial.print("pr_keypad_3 ");
   }    
      // second press within short time
       knumber = knumber * 10 + (key - '0');
      singleDigitEntered = false;
      doubleDigitEntered = true;
      tripleDigitEntered = false;
      lastKeyPressTime = currentTime;
      Serial.println(knumber);
    } else {
 
      // first key pressed
      knumber = key - '0';
      singleDigitEntered = true;
      doubleDigitEntered = false;
      tripleDigitEntered = false;
      lastKeyPressTime = currentTime;
  if (debug) {
         Serial.print("pr_keypad_4 ");
         Serial.println(knumber);
        } // debug
    } // else single
   } // else double
  } // else triple

  } // keypad
  

  // last press long time ago...
  if ((singleDigitEntered && (millis() - lastKeyPressTime > 1000))) {
    if (debug) {
    Serial.print("pr_keypad_5 ");
   }    
    singleDigitEntered = false;
    doubleDigitEntered = false;
    tripleDigitEntered = false;
    Serial.println(knumber);
    digitalWrite(buzzer_pin, HIGH); 
    delay(100);
    digitalWrite(buzzer_pin, LOW); 
  }
}
// valid keys selected 
 // select = 999;
if ((knumber > 0) && (millis() - lastKeyPressTime > 1000))
{
 if (debug)
  {
  Serial.println("valid keypad price entry! ");
  Serial.println("knumber: ");
  Serial.println(knumber);
  Serial.println("lastKeyPressTime: ");
  Serial.println(lastKeyPressTime);
  Serial.println("singleDigitEntered: ");
  Serial.println(singleDigitEntered);
  Serial.println("select b: ");
  Serial.println(select);
  }
//       lcd.clear();
//       lcd.setCursor(0, 0);
//       if (select != 999){
//       lcd.print(knumber);
//       lcd.setCursor(0, 1);
//       lcd.print("* save # weiter");
//       }
//       else 
//        {
//       lcd.print(conveyorPrice[index] / 100.00);
       //lcd.print ("*");
//       }
       delay(500);
       select = knumber;
       knumber = 0;

       if (debug)
  {
  if (select != 999){
   Serial.println("LCD: *");  
  
  Serial.println("upd: ");
  Serial.println(" knumber: ");
  Serial.println(knumber);
  Serial.println(" select c: ");
  Serial.println(select);
   lcd.clear();
  lcd.setCursor(0, 0);
  //lcd.print(knumber);
  lcd.print(select);
}
  }

}

 if (knumber == -13 )
  {
if (debug)
  {
  Serial.println("save pressed: ");
  Serial.println("knumber: ");
  Serial.println(knumber);
  Serial.println("select b: ");
  Serial.println(select);
  Serial.println("conveyorPrice OLD: ");
  Serial.println(conveyorPrice[index]);
  }


  scope ++;
    if (select != 999){
    conveyorPrice[index] = select;
    select = 999;
    // or knumber? no it is -13 at this point.
   // }
    //else {
    if (debug) {
        Serial.print("conveyorPrice new (formatted):  ");
        Serial.println(conveyorPrice[index] / 100.00);
    } 
    }
       
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fach ");
    lcd.print(pass);
    lcd.print(" Preis");
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.print(conveyorPrice[index] / 100.00);
    delay (800);
    knumber = 0;
  } // * entered, next value with

  if (knumber == -6 )
  {
  scope ++;
    if (debug) {
        Serial.print("Fach oeffnen:  ");
        Serial.println("knumber: ");
        Serial.println(knumber);
        Serial.println("select b: ");
        Serial.println(select);
        Serial.println("conveyorPrice OLD: ");
        Serial.println(conveyorPrice[index]);        
  }        
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fach ");
    lcd.print(pass);
    lcd.print(" oeffnen");
    lcd.setCursor(0, 1);
    lcd.print(" ");

    CompartementOpen(index);          
    delay(500);      
    knumber = 0;     

  } // # compartment open via keypad *

  } // while scope and keypad

    }
  }

 coinsCurrentValue = 0;
 //displayBalance();
if (debug) {  
  Serial.println("Neu Eeprom.");
}  
  writeEEPROMcomplete();
if (debug) {
  Serial.println("Konfig beendet");
}  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gespeichert");
  delay(200);

   // check if products available and activate/deactivate payments systems
  payment_on_off ();
  coinsCurrentValue = 0;
  lcd.print("Guthaben reset....");
  delay(200);
  lcd.clear();
  displayBalance();
  }

// ########## Refill ##########
// open all empty boxes and set their value to maxrow

void refill() {
 refill_ongoing = true;
 if (debug) {
  Serial.print(" Refill ");
 }  
  lcd.clear();
if (debug) {
      Serial.print("maxrow ");
      Serial.print(maxrow);
}
   
 for (int rindex = 0; rindex < max; rindex++) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Refill# ");
     if (debug) {
      Serial.print("Refill Col ");
      Serial.print(rindex+1);
      Serial.print("conyeyorItems[rindex]: ");
      Serial.print(conveyorItems[rindex]);
     } 

      
      lcd.print(rindex+1);

    if ( conveyorItems[rindex] == 0) {
      if (debug) {
          Serial.print("Öffnen ");
      }          
          lcd.setCursor(0, 1);
          lcd.print (rindex+1); 
          CompartementOpen(rindex); 
          conveyorItems[rindex] ++;
          delay(200);
                 }
          
    if (( conveyorItems[rindex] == 1) and (maxrow > 1)) {
       if (debug) {
          Serial.print("rindex + max ");
       }          
          lcd.setCursor(0, 1);
          CompartementOpen(rindex + max); 
          lcd.print (rindex+max +1); 
          conveyorItems[rindex] ++;          
          delay(200);
          }

    if (( conveyorItems[rindex] == 2) and (maxrow > 2)) {
        if (debug) {
          Serial.print("rindex + 10 ");
        }
          lcd.setCursor(0, 1);
          lcd.print (rindex+max + max +1);
          CompartementOpen(rindex + max + max); 
          delay(200);
          conveyorItems[rindex] ++;
          }
         
    delay(100);
    } 
    
    if (sim) {
      last_available_products = sms_available_products; 
      lcd.clear();
      lcd.setCursor(0, 0);
      if (debug) {
       Serial.println(" set sms_available_products: ");
       Serial.println(sms_available_products);
               }
       lcd.print("sms_available_products: ");
       lcd.print(sms_available_products);
       delay(2000);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    if (debug) {
    Serial.println(" Refill ende. ");
    lcd.print("Refill beendet.");
    }    
    delay(2000);
    refill_ongoing = false;
    payment_on_off ();
    coinsCurrentValue = 0;
    lcd.print("Guthaben reset");
    delay(200);

    lcd.clear();
    displayBalance();
    }


// ########## LCD MESSAGES ##########

void displayBalance() {
  if (debug) {
  Serial.print("Value:  ");
  Serial.println(coinsCurrentValue / 100.00);
  }  
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print(" Guthaben");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)

  lcd.print(" ");
  lcd.print(coinsCurrentValue / 100.00); // display current balance

//  if you have a display with more than two lines:
//  lcd.setCursor(0, 2);  
//  lcd.print("Fragen? Tel:");
//  lcd.setCursor(0, 3);  
//  lcd.print(String(TELEFONE_NUMBER));
}

void displayPrice(int currentPrice) {
  if (debug) {
  Serial.print("Warenpreis:  ");
  Serial.println(currentPrice / 100.00);
  }  
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print("Preis");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)

  lcd.print(" ");
  lcd.print(currentPrice / 100.00);
  if (coinsCurrentValue > 0) {
    delay(500);
    displayBalance();
  }
}

void displayEmpty() {
  if (debug) {
  Serial.println("Fach leer");
  }  
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print("Leider leer :(");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)
  lcd.print("Bitte neu waehlen");
  if (coinsCurrentValue > 0) {
    delay(1000);
    displayBalance();
  }
}

void displayConfPrice(int con) {
  lcd.setCursor(0, 1);
  lcd.print(" ");
  lcd.print(conveyorPrice[con] / 100.00);
  lcd.print("     ");
}

// ########## write eeprom ########## see https://www.youtube.com/watch?v=5x0nUy36Trs
void writeEPROM (int address, int number)
{
     
   EEPROM.write(address, (number >> 8) & 0xFF);
   EEPROM.write(address + 1, number & 0xFF);
}

// ########## read eeprom ##########
int readEEPROM (int address)
 {  
   return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
  } 

// #########  write eeprom complete #########
void writeEEPROMcomplete ()
 { 
   if (debug) {
    Serial.print("writeEEPcomp..");
    Serial.print("vers ");
    Serial.print(EEPROM_version);  
    Serial.print("max "); 
    Serial.print(max);   
   }
   writeEPROM (0, EEPROM_version);
   writeEPROM (2, max);
   if (debug) {
   Serial.print("write und max done.");
   }
    for (int index = 0; index < max; index++) {
//    3          -> 4 + 2 * index : conveyorItems []
//    4          -> index * 2 + max * 2 + 4 : conveyorPrice []
   
   if (debug) {
     Serial.print("index * 2 + 4  = ");
     Serial.print(index * 2 + 4);     
     Serial.print("Items [index]: "); 
     Serial.print(conveyorItems[index]);
   }
     writeEPROM (index * 2 + 4, conveyorItems[index]);

if (debug) {
     Serial.print("index * 2 + 2 * max + 4 = ");
     Serial.print(index * 2 + 2 * max + 4);     
     Serial.print("Price [index]: "); 
     Serial.print(conveyorPrice[index]);
}
     writeEPROM (index * 2 + max * 2 + 4, conveyorPrice[index]);
   
    }  

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("saved. ");
    delay (200);
 displayBalance();
   

  }

// #########  read eeprom complete #########
void readEEPROMcomplete ()
 { 
 if (debug) {
    Serial.print("readEEPcompl..");
 }    

 //   EEPROM_version = readEEPROM (0);
 //   max            = readEEPROM (2);

if (debug) {    
    Serial.print("vers ");
    Serial.print(EEPROM_version);  
    Serial.print("max "); 
    Serial.print(max);   
}

    for (int index = 0; index < max; index++) {
//    3          -> 4 + 2 * index : conveyorItems []
//    4          -> index * 2 + max * 2 + 4 : conveyorPrice []
//        delay (500);
 if (debug) {
     Serial.print("index = ");
     Serial.println(index);     
 
     Serial.print(" index * 2 + 4  = ");
     Serial.print(index * 2 + 4);     
     Serial.print(" Items [index]: "); 
     Serial.print(conveyorItems[index]);
 }     
     conveyorItems[index] = readEEPROM(index * 2 + 4);
  if (debug) {
     Serial.print(" Items new [index]: "); 
     Serial.print(conveyorItems[index]);

     Serial.print(" index * 2 + 2 * max + 4 = ");
     Serial.print(index * 2 + 2 * max + 4);     
     Serial.print(" Price old [index]: "); 
     Serial.print(conveyorPrice[index]);
  }
     conveyorPrice[index] = readEEPROM(index * 2 + max * 2 + 4);
     if (debug) {
     Serial.print(" Price new [index]: "); 
     Serial.print(conveyorPrice[index]);
     Serial.println(" ----- "); 
     }
    }  

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("loaded. ");
    if (debug) {
    Serial.print(" loaded EEPR. "); 
    }    
    delay (200);
   

  }

// ######################## update serial / sms ######################
void updateSerial(unsigned int wait_ms)
{

  String dataString = "";
  String part1 = "";
  String part2 = "";
  
  int numval = 0;
   int position = 0;
  delay(wait_ms);
  if(mySerial.available()) {
    dataString = mySerial.readString();
    Serial.println(dataString);
  }

  if(dataString != ""){
    if (debug) {
 //   Serial.print(" datastring != ** "); 
 //   Serial.println(dataString);
    Serial.println("lenght: ");
    Serial.println(dataString.length());
    Serial.println("indexOf Paypal_payed7: ");
  //  Serial.println(dataString.indexOf("Sie haben 7,00 EUR erhalten"));
    Serial.println(dataString.indexOf("*EUR erhalten* detected."));
 }    
  
  //#### SMS received about Paypal paymend (forwarded email):
  //  if (dataString.indexOf (" EUR erhalten") >0) or (dataString.indexOf (" EUR mit PayPal gesendet.") >0) {
    if (dataString.indexOf (" EUR erhalten") >0)  {
  //  currentMillis = millis();
      idleTimerMillis = millis();
      digitalWrite(buzzer_pin, HIGH); 
      delay(500);
      digitalWrite(buzzer_pin, LOW); 


  // clarify the position within sms string how much money to consider  
   if (debug) {    
   Serial.print("dataString *_EUR erhalten* / length: ");  
   Serial.println(dataString); 
   Serial.println(dataString.length());
   position = dataString.indexOf("EUR"); 
   Serial.print("Position EUR: "); 
   Serial.println(position); 
   } // debug
   part2 = dataString.substring( (position - 6), (position - 4)); 
   part1 = dataString.substring ( (position - 3 ), (position - 1)  );  
   numval = ( (part2.toInt() * 100) + part1.toInt() );
  
  if (debug) {    
   Serial.print("part1: ");  
   Serial.println(part1);     
   Serial.print("part2: ");  
   Serial.println(part2);   
   Serial.print("numval: ");  
   Serial.print(numval);  
  } // debug

// paypal changed sms message in 9/24 - sometimes separator . instead of comma; one digit after separator
  if ((part2.substring (0,1) == "n") && (part1.substring (0,1) == "."))
   {
    if (debug) {    
     Serial.print("separator point instea of comma!");  
      } //debug
     part2 = dataString.substring( (position - 5), (position - 3)); 
     part1 = dataString.substring ( (position - 2 ), (position - 1)  );  
   //  numval = ( (part2.toInt() * 100) + part1.toInt() );
 // upd 12/24: payment 2.5 was considered as 2.05 - corrected:
     numval = ( (part2.toInt() * 100) + (part1.toInt() * 10) );
     if (debug) {    
        Serial.println("part1 a: ");  
        Serial.println(part1);     
        Serial.print("part2 a: ");  
        Serial.println(part2);   
        Serial.print("numval: ");  
        Serial.print(numval);  
       } // debug
   }  // point as separator / on digit after separator
 //     coinsCurrentValue = coinsCurrentValue + smsPayPal7value;
      coinsCurrentValue = coinsCurrentValue + numval;
       displayBalance();
  //   if (debug) {
    Serial.print(" sms Zahlung erfolgt. " ); 
  //  } 
    } // forwarded email for paypal business accounts

 //#### SMS received about Paypal payment (direct sms from Paypal - for privat (non business) paypal accounts):
  // sometimes two spaces on front of text "EUR", sometimes one space!!!
        if ((dataString.indexOf ("  EUR mit PayPal gesendet.") >0)  or
        (dataString.indexOf (" EUR mit PayPal gesendet.") >0))  {
  //  currentMillis = millis();
      idleTimerMillis = millis();
      digitalWrite(buzzer_pin, HIGH); 
      delay(500);
      digitalWrite(buzzer_pin, LOW); 


  // clarify the position within sms string how much money to consider  
   if (debug) {    
   Serial.print("dataString *..EUR mit PayPal gesendet.*/ length: ");  
   Serial.println(dataString); 
   Serial.println(dataString.length());
   position = dataString.indexOf("EUR"); 
   Serial.print("Position EUR: "); 
   Serial.println(position); 
   } // debug
   part2 = dataString.substring( (position - 6), (position - 4)); 
   part1 = dataString.substring ( (position - 3 ), (position - 1)  );  
   numval = ( (part2.toInt() * 100) + part1.toInt() );
  
  if (debug) {    
   Serial.print("part1 b: ");  
   Serial.println(part1);     
   Serial.print("part2 b: ");  
   Serial.println(part2);   
   Serial.print("numval: ");  
   Serial.print(numval);  
  } // debug
// paypal changed sms message in 9/24 - sometimes separator . instead of comma; one digit after separator
  if ((part2.substring (0,1) == "n") && (part1.substring (0,1) == "."))
   {
    if (debug) {    
     Serial.print("separator point instead of comma!");  
      } //debug
     part2 = dataString.substring( (position - 5), (position - 3)); 
     part1 = dataString.substring ( (position - 2 ), (position - 1)  );  
  //   numval = ( (part2.toInt() * 100) + part1.toInt() );
 // upd 12/24: payment 2.5 was considered as 2.05 - corrected:
     numval = ( (part2.toInt() * 100) + (part1.toInt() * 10) );
     if (debug) {    
        Serial.println("part1 c: ");  
        Serial.println(part1);     
        Serial.print("part2 c: ");  
        Serial.println(part2);   
        Serial.print("numval: ");  
        Serial.print(numval);  
       } // debug
   }  // point as separator / on digit after separator

 //     coinsCurrentValue = coinsCurrentValue + smsPayPal7value;
      coinsCurrentValue = coinsCurrentValue + numval;
       displayBalance();
  //   if (debug) {
    Serial.print(" sms Zahlung erfolgt. " ); 
  //  } 
    } // direct SMS for privat paypal account


//#### SMS received to change sms threshold (inform with smsm about sold/remaining items):


    if (dataString.indexOf ("threshold=0") >0) { 
      sms_available_products = 0;
      digitalWrite(buzzer_pin, HIGH); 
      delay(500);
      digitalWrite(buzzer_pin, LOW); 
      if (debug) {
       Serial.println(" sms threshold changed to 0 "); 
       }
       lcd.clear();
       lcd.setCursor(0, 0);
       lcd.print("sms = 0");
       sms_available_products = 0;
       delay(1000);
       displayBalance();
    }

if (dataString.indexOf ("threshold=3") >0) { 
      sms_available_products = 3;
      digitalWrite(buzzer_pin, HIGH); 
      delay(500);
      digitalWrite(buzzer_pin, LOW); 
      if (debug) {
       Serial.println(" sms threshold changed to 3 "); 
       }
       lcd.clear();
       lcd.setCursor(0, 0);
       lcd.print("sms = 3");
       sms_available_products = 3;
       delay(1000);
       displayBalance();
    }

if (dataString.indexOf ("threshold=99") >0) { 
      digitalWrite(buzzer_pin, HIGH); 
      delay(500);
      digitalWrite(buzzer_pin, LOW); 
      if (debug) {
       Serial.println(" sms threshold changed to 99 "); 
       }
       lcd.clear();
       lcd.setCursor(0, 0);
       lcd.print("sms = 99");
       sms_available_products = 99;
       delay(1000);
       displayBalance();
    }

if (dataString.indexOf ("refill") >0) { 
      digitalWrite(buzzer_pin, HIGH); 
      delay(500);
      digitalWrite(buzzer_pin, LOW); 
      if (debug) {
       Serial.println(" sms refill "); 
       }
       lcd.clear();
       lcd.setCursor(0, 0);
       lcd.print("refill");
         refill();
         writeEEPROMcomplete(); 
       delay(1000);
       displayBalance();
    }


  } //## datastring ...

  while(Serial.available()) {
    mySerial.write(Serial.read());
  }
} //# updateSerial



// ########## MAIN LOOP ##########

void loop() {
  if (digitalRead(configbutton) == LOW) {
if (debug) {
   Serial.print("DigitalRead(configb): LOW");
}
   set_values();
   delay(1000);
  }

 if (digitalRead(refillbutton) == LOW) {
if (debug) {   
   Serial.print("DigitalRead(refillbutton): LOW");
}   
   refill();
 writeEEPROMcomplete(); 
   delay(1000);
  }  
//if (debug) {
//    Serial.print("idle ");
// }    
  idle();

  // check if a coin has been inserted
 if (coinsChange == 1) {
  if (debug) {
    Serial.print("CoinsChange ");
   }    
   coinsChange = 0; // unflag that a coin has been inserted
   idleTimerMillis = millis();
   displayBalance(); // display current balance
  
  }



// check if a bill has been inserted
 if (nv10_Change == 1) {
  if (debug) {
    Serial.print("NV10_Change ");
   }    
   nv10_Change = 0; // unflag that a coin has been inserted
   idleTimerMillis = millis();
   displayBalance(); // display current balance
  
  }

// check if a card or mobile payment has been done
 if (ox_Change == 1) {
  if (debug) {
    Serial.print("ox_Change ");
   }    
   ox_Change = 0; // unflag that a card or mobile payment has been done
   idleTimerMillis = millis();
   displayBalance(); // display current balance
  
  }
 // *********** keypad pressed **********

 if (keypad) {
char key = customKeypad.getKey();
 
  if (key) {
    if (debug) {
    Serial.print("keypad_2 ");
   }    
    unsigned long currentTime = millis();
    digitalWrite(buzzer_pin, HIGH); 
    delay(150);
    digitalWrite(buzzer_pin, LOW); 

    if (singleDigitEntered && (currentTime - lastKeyPressTime <= 1000)) {
      if (debug) {
    Serial.print("keypad_3 ");
   }    
      // second press within short time
       knumber = knumber * 10 + (key - '0');
      singleDigitEntered = false;
      Serial.println(knumber);
    } else {
 
      // first key pressed
      knumber = key - '0';
      singleDigitEntered = true;
      lastKeyPressTime = currentTime;
  if (debug) {
         Serial.print("keypad_4 ");
         Serial.println(knumber);
        } // debug
  } // else
  } // keypad
  

  // last press long time ago...
  if ((singleDigitEntered && (millis() - lastKeyPressTime > 1000))) {
    if (debug) {
    Serial.print("keypad_5 ");
   }    
    singleDigitEntered = false;
    Serial.println(knumber);
    digitalWrite(buzzer_pin, HIGH); 
    delay(100);
    digitalWrite(buzzer_pin, LOW); 
  }
}
// valid keys selected 
  select = 999;
if ((knumber > 0) && (millis() - lastKeyPressTime > 1000) and (knumber <= (max * maxrow)))
{
 if (debug)
  {
  Serial.println("valid keypad entry! ");
  Serial.println("knumber: ");
  Serial.println(knumber);
  Serial.println("lastKeyPressTime: ");
  Serial.println(lastKeyPressTime);
  Serial.println("singleDigitEntered: ");
  Serial.println(singleDigitEntered);
  Serial.println("select a: ");
  Serial.println(select);
  }
       lcd.clear();
       lcd.setCursor(0, 0);
       lcd.print(knumber);
       delay(500);
       select = knumber -1;
       knumber = 0;

}
else {
  if (knumber > (max * maxrow))
  {
if (debug) {
    Serial.println("keypad entry invalid ");
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(knumber);
    lcd.setCursor(0, 1);
    lcd.print("ungueltig!");
    digitalWrite(buzzer_pin, HIGH); 
    delay(500);
    digitalWrite(buzzer_pin, LOW); 
    knumber = 0;
 }
}
 
  // ********** BUTTON PRESSED **********

   
   for (int index = 0; index < max; index++) {    
    if 
     (((digitalRead(selector[index]) == LOW) and (!keypad)) or ((select == index) and (keypad)))
  
          { // button pressed
if (debug) {
      Serial.print("Spalte ");
      Serial.print(index + 1);
      Serial.println(" gedrueckt ");
      Serial.println(" items: ");
      Serial.print(conveyorItems[index]);
      Serial.println("  ");
      Serial.println(" INDEX selected:");
      Serial.println(select);
}      
      if (conveyorItems[index] > 0) {
        if (coinsCurrentValue < conveyorPrice[index]) { // insufficient balance - display price
          displayPrice(conveyorPrice[index]);
          idleTimerMillis = millis();
          delay(2000);
        }
       // #### SELL ITEM ##### 
        if ((coinsCurrentValue > conveyorPrice[index] - 1) ) { // sufficient balance
          coinsCurrentValue = coinsCurrentValue - conveyorPrice[index]; // reduce balance
          conveyorItems[index]--; // reduce items
    if (debug) {
    Serial.print("update EEPROM conveyorItem ");
    Serial.print(index * 2 + 4, conveyorItems[index]);          
    }
          writeEPROM (index * 2 + 4, conveyorItems[index]);
          if (coinsCurrentValue < 0) {
            coinsCurrentValue = 0;  // correct float rounding error
          }
          delay(200);
      
//          which compartment to open?
if (debug) {
    Serial.print("conveyorItems[index] ");
    Serial.print(conveyorItems[index]);
    Serial.print("index ");
    Serial.print(index);
    }
          if ( conveyorItems[index] == 0) {
           if (debug) {
          Serial.print("index open ");
           }
           CompartementOpen(index); }   
          if ( conveyorItems[index] == 1) {
          if (debug) {
          Serial.print("index plus max öffnen ");
          }          
          CompartementOpen(index + max); }
          if ( conveyorItems[index] == 2) {
           if (debug) {
          Serial.print("index + 2 * max ");
           }
          CompartementOpen(index + max + max); }
          displayBalance(); // display current balance
          idleTimerMillis = millis(); // reset idle timer

// turn off payment (coin acceptor or cashless system - ox ) if no further product available
// 

//          inform homematic about sold item only if serial.print not used (using digital pin1 which is dedicaded to serial) (issue with uno only)
          delay(100);
          digitalWrite(homematic_pin, LOW); 
          delay(1500);
          digitalWrite(homematic_pin, HIGH); 
          delay(200);
// inform Nayax Onyx about sold items but only if it was paid cash (possible last_paytype = O (onyx), N (nv10) or C (cash) )
  if (debug) {
    Serial.println("lastPaytype - C=Coin, N=NV10, O=Onyx, ?=not set: ");
    Serial.print(last_paytype);
    Serial.println("<-lastPaytype");
   }    


         if(ox) {
          if  (last_paytype != 'O') {
          ox_pulse_back (conveyorPrice[index]);
          } // not for ox payments
          } 


        }
        // ##### END SELL ITEM ##########
      } else {
        displayEmpty();
        idleTimerMillis = millis(); // reset idle timer
        delay(200);
      }
       //reset key 
       select = 999;
       knumber = 0;
    }
    
  }

//##### check if SMS incoming....
if (sim)
{
  if (debug)
  {
   // Serial.println ("check .. " ); // be care this produces huge amount of messages
  }
updateSerial(0);
}


}
