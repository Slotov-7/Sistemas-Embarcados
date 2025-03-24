#include <LiquidCrystal.h>
#include <EEPROM.h>

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd_1(7, 8, 9, 10, 11, 12);

// Pin definitions
const int motorPin = 6;
const int ledPin = 5;
const int resetButtonPin = 4;  // Pino para o botão de reset

// Global variables
int umi = 0;
int lastUmi = -1;
float temperatura = 0;
float lastTemperatura = -1;
int waterLevel = 0;
bool motorOn = false;
bool lastMotorOn = false;
bool irrigationTriggered = false;
bool showWaterWarning = false;

// Configuration saved in EEPROM
int umiThreshold = 45;
int waterLevelThreshold = 200;
int tempMax = 60;

// Timing variables
unsigned long previousCheckMillis = 0;
const long checkInterval = 10000;
unsigned long motorStartTime = 0;
const long motorDuration = 5000;

// Serial printing control variables
unsigned long lastSerialPrint = 0;
const unsigned long serialPrintInterval = 5000;

// Circular buffer for moving average
const int numReadings = 5;
int umiReadings[numReadings];
int tempReadings[numReadings];
int waterReadings[numReadings];
int readingIndex = 0;

// Debounce para o botão
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void saveToEEPROM() {
  EEPROM.write(0, umiThreshold);
  EEPROM.write(1, waterLevelThreshold / 4);  // Dividido por 4 para caber em 1 byte (0-255)
  EEPROM.write(2, tempMax);
  delay(5000);
}

void loadFromEEPROM() {
  umiThreshold = EEPROM.read(0);
  waterLevelThreshold = EEPROM.read(1) * 4;  // Multiplica por 4 para restaurar
  tempMax = EEPROM.read(2);
}

void processSerial() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "testmotor") {
      digitalWrite(motorPin, HIGH);
      Serial.println("Motor testado: LIGADO (5s)");
      delay(5000);
      digitalWrite(motorPin, LOW);
      Serial.println("Motor testado: DESLIGADO");
    }
    else if (command.startsWith("setUmi")) {
      int value = command.substring(7).toInt();
      if (value >= 0 && value <= 100) {
        umiThreshold = value;
        Serial.print("Umidade Threshold ajustado: ");
        Serial.println(umiThreshold);
        Serial.println("Configuracoes salvas na EEPROM!");
      } else {
        Serial.println("Valor invalido para umidade.");
      }
    }
    else if (command.startsWith("setWater")) {
      int value = command.substring(8).toInt();
      if (value >= 0 && value <= 1023) {
        waterLevelThreshold = value;
        Serial.print("Water Level Threshold ajustado: ");
        Serial.println(waterLevelThreshold);
        Serial.println("Configuracoes salvas na EEPROM!");
      } else {
        Serial.println("Valor invalido para nivel de agua.");
      }
    }
    else if (command.startsWith("setTemp")) {
      int value = command.substring(7).toInt();
      if (value >= 10 && value <= 80) {
        tempMax = value;
        Serial.print("Temp Max ajustado: ");
        Serial.println(tempMax);
        Serial.println("Configuracoes salvas na EEPROM!");
      } else {
        Serial.println("Valor invalido para tempMax. Use entre 10 e 80.");
      }
    }
    else if (command == "show") {
      Serial.println("=== Configuracoes Atuais ===");
      Serial.print("Umidade Threshold: ");
      Serial.print(umiThreshold);
      Serial.println("%");
      Serial.print("Water Level Threshold: ");
      Serial.print(waterLevelThreshold);
      Serial.println(" (0-1023)");
      Serial.print("Temperatura Maxima: ");
      Serial.print(tempMax);
      Serial.println(" C");
      Serial.println("==========================");
    }
  }
}

void setup() {
  // Configure sensor and actuator pins
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(resetButtonPin, INPUT_PULLUP);  // Botão com pull-up interno
  digitalWrite(motorPin, LOW);
  digitalWrite(ledPin, LOW);

  lcd_1.begin(16, 2);
  Serial.begin(9600);

  // Carrega valores da EEPROM ao iniciar
  loadFromEEPROM();

  // Initialize moving average buffers
  for (int i = 0; i < numReadings; i++) {
    umiReadings[i] = 0;
    tempReadings[i] = 0;
    waterReadings[i] = 0;
  }

  // Show initial LCD screen
  lcd_1.setCursor(0, 0);
  lcd_1.print("Sistema");
  lcd_1.setCursor(0, 1);
  lcd_1.print("Conectado!");
  Serial.println("Sistema Conectado!");
  delay(3000);
  lcd_1.clear();

  // Set initial LCD layout
  lcd_1.setCursor(0, 0);
  lcd_1.print("UMI:   % MOT: 0  ");
  lcd_1.setCursor(0, 1);
  lcd_1.print("Temp:    C");

  Serial.println("Comandos: setUmi [valor], setWater [valor], setTemp [valor], show, testmotor");
  previousCheckMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // Verifica o botão de reset com debounce
  int buttonState = digitalRead(resetButtonPin);
  static int lastButtonState = HIGH;
  
  if (buttonState != lastButtonState) {
    lastDebounceTime = currentMillis;
  }
  
  if ((currentMillis - lastDebounceTime) > debounceDelay) {
    if (buttonState == LOW) {  // Botão pressionado (LOW por causa do pull-up)
      saveToEEPROM();
    }
  }
  lastButtonState = buttonState;

  // Read sensors and update moving average buffers
  int rawUmi = analogRead(A0);
  int rawTemp = analogRead(A1);
  int rawWater = analogRead(A2);

  umiReadings[readingIndex] = rawUmi;
  tempReadings[readingIndex] = rawTemp;
  waterReadings[readingIndex] = rawWater;
  readingIndex = (readingIndex + 1) % numReadings;

  long umiTotal = 0, tempTotal = 0, waterTotal = 0;
  for (int i = 0; i < numReadings; i++) {
    umiTotal += umiReadings[i];
    tempTotal += tempReadings[i];
    waterTotal += waterReadings[i];
  }
  rawUmi = umiTotal / numReadings;
  rawTemp = tempTotal / numReadings;
  rawWater = waterTotal / numReadings;

  // Convert readings
  umi = map(rawUmi, 0, 1023, 0, 100);
  float voltage = rawTemp * (5.0 / 1023.0);
  temperatura = (voltage - 0.5) * 100;
  waterLevel = rawWater;

  // Update LED alert
  digitalWrite(ledPin, (waterLevel <= waterLevelThreshold || temperatura > tempMax) ? HIGH : LOW);

  // Check motor control when off
  if (!motorOn && (currentMillis - previousCheckMillis >= checkInterval)) {
    if (umi < umiThreshold && waterLevel > waterLevelThreshold && temperatura <= tempMax) {
      motorOn = true;
      irrigationTriggered = true;
      motorStartTime = currentMillis;
      digitalWrite(motorPin, HIGH);
      Serial.println("Irrigacao ativada: Umidade Baixa");
    }
    previousCheckMillis = currentMillis;
  }

  // Motor duration control
  if (motorOn && (currentMillis - motorStartTime >= motorDuration)) {
    digitalWrite(motorPin, LOW);
    motorOn = false;
    Serial.println("Motor desativado");
  }

  // LCD update - first line (humidity and motor status)
  if (umi != lastUmi) {
    lcd_1.setCursor(4, 0);
    lcd_1.print(umi < 10 ? "  " : (umi < 100 ? " " : ""));
    lcd_1.print(umi);
    lastUmi = umi;
  }

  if (motorOn != lastMotorOn) {
    lcd_1.setCursor(9, 0);
    lcd_1.print("MOT: ");
    lcd_1.print(motorOn ? "1  " : "0  ");
    lastMotorOn = motorOn;
    if (!motorOn && lastMotorOn) {
      lcd_1.setCursor(0, 1);
      lcd_1.print("Temp:       C");
      lcd_1.setCursor(6, 1);
      lcd_1.print(temperatura, 1);
      lastTemperatura = temperatura;
    }
  }

  // LCD update - second line depending on conditions
  if (motorOn) {
    lcd_1.setCursor(0, 1);
    lcd_1.print(irrigationTriggered ? "Umidade Baixa   " : "24h Prog    ");
  }
  else if (showWaterWarning || waterLevel <= waterLevelThreshold) {
    lcd_1.setCursor(0, 1);
    lcd_1.print("Sem Agua      ");
  }
  else if (temperatura > tempMax) {
    lcd_1.setCursor(0, 1);
    lcd_1.print("Temp Alta     ");
  }
  else {
    lcd_1.setCursor(0, 1);
    lcd_1.print("Temp:       C");
    lcd_1.setCursor(6, 1);
    lcd_1.print(temperatura, 1);
    lastTemperatura = temperatura;
  }

  // Print to Serial only every serialPrintInterval milliseconds
  if (currentMillis - lastSerialPrint >= serialPrintInterval) {
    Serial.print("Umidade: ");
    Serial.print(umi);
    Serial.print("% | Temp: ");
    Serial.print(temperatura, 1);
    Serial.print(" C (Max: ");
    Serial.print(tempMax);
    Serial.print(") | Water Level: ");
    Serial.println(waterLevel);
    lastSerialPrint = currentMillis;
  }

  // Process serial commands
  processSerial();

  delay(1000);
}