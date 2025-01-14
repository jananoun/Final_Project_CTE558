#include "DS1302.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin definitions
#define RTC_RST_PIN 8
#define RTC_IO_PIN 10
#define RTC_SCLK_PIN 9

#define MODE_BUTTON_PIN 3
#define INCREMENT_BUTTON_PIN 4
#define CONFIRM_BUTTON_PIN 5
#define BACK_BUTTON_PIN 6

#define LED1_PIN 2
#define LED2_PIN 11
#define LED3_PIN 12
#define LED4_PIN 7
#define BUZZER_PIN 13

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// RTC and LCD initialization
DS1302 rtc(RTC_RST_PIN, RTC_IO_PIN, RTC_SCLK_PIN);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// Storage for scheduled times
int ledHours[4] = {6, 12, 18, 22};  // Default times for slots
int ledMinutes[4] = {0, 0, 0, 0};   // Default times for slots

int selectedSlot = 0;       
bool isConfigMode = false;  
bool isConfigMinutes = false;
bool isTimeSetMode = true;
bool configurationComplete = false; // New flag to track if all slots are configured

// Time setting variables
int setHour = 0;
int setMinute = 0;
int setSecond = 0;
bool isSettingHour = true;
bool isSettingMinute = false;
bool isSettingSecond = false;

void setup() {
  Serial.begin(9600);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);

  // Initialize RTC
  rtc.halt(false);
  rtc.writeProtect(false);

  // Check if RTC has valid time

  // Initialize pins
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(INCREMENT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CONFIRM_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BACK_BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  if (isTimeSetMode) {
    lcd.clear();
    lcd.print("Set Current Time");
    delay(1000);
  } else {
    lcd.clear();
    lcd.print("Ready");
    delay(1000);
  }
}

void loop() {
  if (isTimeSetMode) {
    handleTimeSet();
  } else if (!isConfigMode) {
    displayTimeAndNextSlot();
    handleNormalOperation();
    if (digitalRead(MODE_BUTTON_PIN) == LOW) {
      enterConfigMode();
    }
  } else {
    handleConfigMode();
  }
}

void handleTimeSet() {
  lcd.setCursor(0, 0);
  lcd.print("Set Time:       ");
  lcd.setCursor(0, 1);
  
  // Display current setting
  lcd.print(setHour < 10 ? "0" : "");
  lcd.print(setHour);
  lcd.print(":");
  lcd.print(setMinute < 10 ? "0" : "");
  lcd.print(setMinute);
  lcd.print(":");
  lcd.print(setSecond < 10 ? "0" : "");
  lcd.print(setSecond);

  // Increment button
  if (digitalRead(INCREMENT_BUTTON_PIN) == LOW) {
    if (isSettingHour) {
      setHour = (setHour + 1) % 24;
    } else if (isSettingMinute) {
      setMinute = (setMinute + 1) % 60;
    } else if (isSettingSecond) {
      setSecond = (setSecond + 1) % 60;
    }
    delay(200);
  }

  // Confirm button - move to next field or save
  if (digitalRead(CONFIRM_BUTTON_PIN) == LOW) {
    if (isSettingHour) {
      isSettingHour = false;
      isSettingMinute = true;
    } else if (isSettingMinute) {
      isSettingMinute = false;
      isSettingSecond = true;
    } else if (isSettingSecond) {
      // Save the time
      rtc.setTime(setHour, setMinute, setSecond);
      rtc.setDate(1, 1, 2024);
      
      lcd.clear();
      lcd.print("Time Set!");
      delay(1000);
      
      isTimeSetMode = false;
      isSettingHour = true;
      isSettingMinute = false;
      isSettingSecond = false;
      configurationComplete = false; // Reset configuration flag
    }
    delay(300);
  }

  // Back button to exit time setting
  if (digitalRead(BACK_BUTTON_PIN) == LOW) {
    isTimeSetMode = false;
    isSettingHour = true;
    isSettingMinute = false;
    isSettingSecond = false;
    delay(300);
  }
}

void handleConfigMode() {
  static int hour = ledHours[selectedSlot];
  static int minute = ledMinutes[selectedSlot];

  lcd.setCursor(0, 0);
  lcd.print("Slot ");
  lcd.print(selectedSlot + 1);
  lcd.setCursor(0, 1);

  if (!isConfigMinutes) {
    lcd.print("Hour: ");
    lcd.print(hour < 10 ? "0" : "");
    lcd.print(hour);
    lcd.print("   ");
  } else {
    lcd.print("Minute: ");
    lcd.print(minute < 10 ? "0" : "");
    lcd.print(minute);
    lcd.print("   ");
  }

  // Mode button logic to switch slots or enter time setting
  if (digitalRead(MODE_BUTTON_PIN) == LOW) {
    if (configurationComplete) {
      // If all slots are configured, enter time setting mode
      isConfigMode = false;
      isTimeSetMode = true;
      configurationComplete = false;
      selectedSlot = 0;
      lcd.clear();
      lcd.print("Enter Time Set");
      delay(1000);
    } else {
      selectedSlot = (selectedSlot + 1) % 4;
      hour = ledHours[selectedSlot];
      minute = ledMinutes[selectedSlot];
      
      if (selectedSlot == 0) {
        configurationComplete = true; // All slots have been configured
      }
      
      lcd.clear();
      lcd.print("Slot ");
      lcd.print(selectedSlot + 1);
    }
    delay(300);
  }

  // Increment button logic
  if (digitalRead(INCREMENT_BUTTON_PIN) == LOW) {
    if (!isConfigMinutes) {
      hour = (hour + 1) % 24;
    } else {
      minute = (minute + 1) % 60;
    }
    delay(200);
  }

  // Confirm button logic
  if (digitalRead(CONFIRM_BUTTON_PIN) == LOW) {
    if (!isConfigMinutes) {
      isConfigMinutes = true;
      delay(200);
    } else {
      ledHours[selectedSlot] = hour;
      ledMinutes[selectedSlot] = minute;

      lcd.clear();
      lcd.print("Saved Slot ");
      lcd.print(selectedSlot + 1);
      delay(1000);

      isConfigMinutes = false;
      
      // Automatically move to next slot
      selectedSlot = (selectedSlot + 1) % 4;
      if (selectedSlot == 0) {
        configurationComplete = true;
      }
      hour = ledHours[selectedSlot];
      minute = ledMinutes[selectedSlot];
    }
  }

  // Back button logic
  if (digitalRead(BACK_BUTTON_PIN) == LOW) {
    lcd.clear();
    lcd.print("Exiting");
    delay(1000);
    isConfigMode = false;
    configurationComplete = false;
    selectedSlot = 0;
  }
}

void displayTimeAndNextSlot() {
  Time now = rtc.getTime();
  int nextSlot = getNextSlot(now.hour, now.min);

  // Display current time on first line
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(now.hour < 10 ? "0" : "");
  lcd.print(now.hour);
  lcd.print(":");
  lcd.print(now.min < 10 ? "0" : "");
  lcd.print(now.min);
  lcd.print(":");
  lcd.print(now.sec < 10 ? "0" : "");
  lcd.print(now.sec);

  // Display next slot on second line
  lcd.setCursor(0, 1);
  lcd.print("med:");
  lcd.print(nextSlot + 1);
  lcd.print(" ");
  lcd.print(ledHours[nextSlot] < 10 ? "0" : "");
  lcd.print(ledHours[nextSlot]);
  lcd.print(":");
  lcd.print(ledMinutes[nextSlot] < 10 ? "0" : "");
  lcd.print(ledMinutes[nextSlot]);
}

void enterConfigMode() {
  isConfigMode = true;
  isConfigMinutes = false;
  configurationComplete = false;
  selectedSlot = 0;
  lcd.clear();
  lcd.print("Set med ");
  lcd.print(selectedSlot + 1);
  delay(1000);
}

// Rest of the functions (getNextSlot, handleNormalOperation, activateLEDAndBuzzer) remain the same
int getNextSlot(int currentHour, int currentMinute) {
  for (int i = 0; i < 4; i++) {
    if ((ledHours[i] > currentHour) || 
        (ledHours[i] == currentHour && ledMinutes[i] > currentMinute)) {
      return i;
    }
  }
  return 0;
}

void handleNormalOperation() {
  Time now = rtc.getTime();
  for (int i = 0; i < 4; i++) {
    if (now.hour == ledHours[i] && now.min == ledMinutes[i] && now.sec == 0) {
      activateLEDAndBuzzer(i);
    }
  }
}

void activateLEDAndBuzzer(int slot) {
  int ledPin;
  switch (slot) {
    case 0: ledPin = LED1_PIN; break;
    case 1: ledPin = LED2_PIN; break;
    case 2: ledPin = LED3_PIN; break;
    case 3: ledPin = LED4_PIN; break;
  }

  digitalWrite(ledPin, HIGH);
  tone(BUZZER_PIN, 1000);
  delay(5000);
  digitalWrite(ledPin, LOW);
  noTone(BUZZER_PIN);
}