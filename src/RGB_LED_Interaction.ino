#include <LiquidCrystal_I2C.h>
#include <Wire.h>

const uint8_t blueLedPin = 2;
const uint8_t greenLedPin = 3;
const uint8_t redLedPin = 4;

const uint8_t RGBLedRedPin = 12;
const uint8_t RGBLedGreenPin = 11;
const uint8_t RGBLedBluePin = 10;

const uint8_t LEFT_ARROW_BUTTON = 9;
const uint8_t RIGHT_ARROW_BUTTON = 8;
const uint8_t UP_ARROW_BUTTON = 6;
const uint8_t DOWN_ARROW_BUTTON = 7;
const uint8_t ENTER_BUTTON = 5;

uint8_t RedLedValue, GreenLedValue, BlueLedValue = 0;
uint8_t RedLedValueSolution, GreenLedValueSolution, BlueLedValueSolution;
uint8_t selectedLedIndex = 0;
uint8_t LedPairs[3][4] = {
  {redLedPin, RGBLedRedPin, RedLedValue, RedLedValueSolution},
  {greenLedPin, RGBLedGreenPin, GreenLedValue, GreenLedValueSolution},
  {blueLedPin, RGBLedBluePin, BlueLedValue, BlueLedValueSolution}
};

bool INTERACTION_SOLVED, INTERACTION_RUNNING;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.blink();
  lcd.print("#R000 G000 B000#\0");
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(RGBLedRedPin, OUTPUT);
  pinMode(RGBLedGreenPin, OUTPUT);
  pinMode(RGBLedBluePin, OUTPUT);
  pinMode(LEFT_ARROW_BUTTON, INPUT);
  pinMode(RIGHT_ARROW_BUTTON, INPUT);
  pinMode(UP_ARROW_BUTTON, INPUT);
  pinMode(DOWN_ARROW_BUTTON, INPUT);
  pinMode(ENTER_BUTTON, INPUT);
}

void loop() {
  if (!Serial) {
    Serial.begin(9600);
  }
  if (Serial.available()) {
    processSerialMessage();
  }
  if (INTERACTION_SOLVED == false && INTERACTION_RUNNING == true) {
    gameLoop();
  }
}

void gameLoop() {
  if (digitalRead(LEFT_ARROW_BUTTON) == HIGH) {
    changeSelectedLED(-1);
    writeToLCD();
    delay(250);
  }
  if (digitalRead(RIGHT_ARROW_BUTTON) == HIGH) {
    changeSelectedLED(1);
    writeToLCD();
    delay(250);
  }
  if (digitalRead(UP_ARROW_BUTTON) == HIGH) {
    setLEDValue(1);
    writeToLCD();
    delay(100);
  }
  if (digitalRead(DOWN_ARROW_BUTTON) == HIGH) {
    setLEDValue(-1);
    writeToLCD();
    delay(100);
  }
  if (digitalRead(ENTER_BUTTON) == HIGH) {
    checkWinning();
    delay(100);
  }
}

void changeSelectedLED(int8_t indexVariation) {
  selectedLedIndex += indexVariation;
  if (selectedLedIndex > 2) {
    selectedLedIndex = 0;
  } else if (selectedLedIndex < 0) {
    selectedLedIndex = 2;
  }
}

void setLEDValue(int8_t pwmValueVariation) {
  int16_t newPWMValue = LedPairs[selectedLedIndex][2] + pwmValueVariation;
  if (newPWMValue > 255) {
    newPWMValue = 0;
  } else if (newPWMValue < 0) {
    newPWMValue = 255;
  }
  analogWrite(LedPairs[selectedLedIndex][0], newPWMValue);
  analogWrite(LedPairs[selectedLedIndex][1], newPWMValue);
  LedPairs[selectedLedIndex][2] = newPWMValue;
}

void writeToLCD() {
  if (selectedLedIndex == 0) {
    lcd.setCursor(2, 0);
    lcd.print("   ");
    lcd.setCursor(2, 0);
    lcd.print(LedPairs[selectedLedIndex][2]);
  } else if (selectedLedIndex == 1) {
    lcd.setCursor(7, 0);
    lcd.print("   ");
    lcd.setCursor(7, 0);
    lcd.print(LedPairs[selectedLedIndex][2]);
  } else {
    lcd.setCursor(12, 0);
    lcd.print("   ");
    lcd.setCursor(12, 0);
    lcd.print(LedPairs[selectedLedIndex][2]);
  }
}

void processSerialMessage() {
  const uint8_t BUFF_SIZE = 64; // make it big enough to hold your longest command
  static char buffer[BUFF_SIZE + 1]; // +1 allows space for the null terminator
  static uint8_t length = 0; // number of characters currently in the buffer

  char c = Serial.read();
  if ((c == '\r') || (c == '\n')) {
    // end-of-line received
    if (length > 0) {
      tokenizeReceivedMessage(buffer);
    }
    length = 0;
  } else {
    if (length < BUFF_SIZE) {
      buffer[length++] = c; // append the received character to the array
      buffer[length] = 0; // append the null terminator
    }
  }
}

void tokenizeReceivedMessage(char *msg) {
  const uint8_t COMMAND_PAIRS = 10;
  char* tokenizedString[COMMAND_PAIRS + 1];
  uint8_t index = 0;

  char* command = strtok(msg, ";");
  while (command != 0) {
    char* separator = strchr(command, ':');
    if (separator != 0) {
      *separator = 0;
      tokenizedString[index++] = command;
      ++separator;
      tokenizedString[index++] = separator;
    }
    command = strtok(0, ";");
  }
  tokenizedString[index] = 0;

  processReceivedMessage(tokenizedString);
}

void processReceivedMessage(char** command) {
  if (strcmp(command[1], "START") == 0) {
    startSequence(command[3]);
  } else if (strcmp(command[1], "PAUSE") == 0) {
    pauseSequence(command[3]);
  } else if (strcmp(command[1], "STOP") == 0) {
    stopSequence(command[3]);
  } else if (strcmp(command[1], "INTERACTION_SOLVED_ACK") == 0) {
    setInteractionSolved();
  } else if (strcmp(command[1], "PING") == 0) {
    ping(command[3]);
  } else if (strcmp(command[1], "BAUD") == 0) {
    setBaudRate(atoi(command[3]), command[5]);
  } else if (strcmp(command[1], "SETUP") == 0) {
    Serial.println("COM:SETUP;INT_NAME:RGB LED Interaction;BAUD:9600");
    Serial.flush();
  }
}

//TODO: Review This Method once Interaction Is Completed
void startSequence(char* TIMESTAMP) {
  INTERACTION_SOLVED = false;
  INTERACTION_RUNNING = true;
  for (uint8_t i = 0; i < 3; i++) {
    LedPairs[i][3] = random(0, 256);
  }
  Serial.print("COM:START_ACK;MSG:R-");
  Serial.print(LedPairs[0][3]);
  Serial.print(" G-");
  Serial.print(LedPairs[1][3]);
  Serial.print(" B-");
  Serial.print(LedPairs[2][3]);
  Serial.print(";ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void pauseSequence(char* TIMESTAMP) {
  INTERACTION_RUNNING = !INTERACTION_RUNNING;
  if (INTERACTION_RUNNING) {
    Serial.print("COM:PAUSE_ACK;MSG:Device is now running;ID:");
  } else {
    Serial.print("COM:PAUSE_ACK;MSG:Device is now paused;ID:");
  }
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void stopSequence(char* TIMESTAMP) {
  INTERACTION_RUNNING = false;
  Serial.print("COM:STOP_ACK;MSG:Device is now stopped;ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void setInteractionSolved() {
  INTERACTION_SOLVED = true;
  INTERACTION_RUNNING = false;
}

void ping(char* TIMESTAMP) {
  Serial.print("COM:PING;MSG:PING;ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void setBaudRate(int baudRate, char* TIMESTAMP) {
  Serial.flush();
  Serial.begin(baudRate);
  Serial.print("COM:BAUD_ACK;MSG:The Baud Rate was set to ");
  Serial.print(baudRate);
  Serial.print(";ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

//TODO: Change this method in order to validate if the interaction has been solved
bool checkWinning() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  if (LedPairs[0][2] == LedPairs[0][3] && LedPairs[1][2] == LedPairs[1][3] && LedPairs[2][2] == LedPairs[2][3]) {
    lcd.setCursor(2, 1);
    lcd.print("Correct RGB\0");
    Serial.println("COM:INTERACTION_SOLVED;MSG:Correct RGB Combination;PNT:2345");
    Serial.flush();
  } else {
    lcd.setCursor(1, 1);
    lcd.print("Incorrect RGB\0");
  }
}
