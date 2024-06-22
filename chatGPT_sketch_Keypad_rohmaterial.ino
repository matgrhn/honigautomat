#include <Wire.h>       // Standardbibliothek für I2C
#include <Keypad.h>     // Bibliothek für das Keypad
#include <Keypad_I2C.h> // Bibliothek für das Keypad über I2C

const byte ROWS = 4; // Vier Reihen
const byte COLS = 3; // Drei Spalten

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {0, 1, 2, 3}; // Verbindungen zu den Zeilen des Keypads (PCF8574 P0-P3)
byte colPins[COLS] = {4, 5, 6};    // Verbindungen zu den Spalten des Keypads (PCF8574 P4-P6)

Keypad_I2C customKeypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, 0x20); // 0x20 ist die I2C Adresse des PCF8574

unsigned long lastKeyPressTime = 0;
int number = 0;
bool singleDigitEntered = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  customKeypad.begin();
}

void loop() {
  char key = customKeypad.getKey();

  if (key) {
    unsigned long currentTime = millis();

    if (singleDigitEntered && (currentTime - lastKeyPressTime <= 1000)) {
      // Zweite Taste innerhalb einer Sekunde gedrückt
      number = number * 10 + (key - '0');
      singleDigitEntered = false;
      Serial.println(number);
    } else {
      // Erste Taste gedrückt
      number = key - '0';
      singleDigitEntered = true;
      lastKeyPressTime = currentTime;
    }
  }

  // Überprüfen, ob mehr als eine Sekunde seit der letzten Eingabe vergangen ist
  if (singleDigitEntered && (millis() - lastKeyPressTime > 1000)) {
    singleDigitEntered = false;
    Serial.println(number);
  }
}
