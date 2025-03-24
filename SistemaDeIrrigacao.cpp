//
// Created by Guilherme Araújo on 24/03/2025.
//
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Pinos do LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd_1(7, 8, 9, 10, 11, 12);

// Definicao de pinos
const int motorPin = 6;   // Pino do motor
const int ledPin = 5;     // Pino do LED

// Variaveis globais
int umi = 0;              // Umidade atual
int lastUmi = -1;         // Ultima umidade exibida no LCD
float temperatura = 0;    // Temperatura atual
float lastTemperatura = -1; // Ultima temperatura exibida no LCD
int waterLevel = 0;       // Nivel de agua atual
int lastWaterLevel = -1;  // Ultimo nivel de agua exibido
bool lastMotorOn = false; // Ultimo estado do motor
bool motorOn = false;     // Estado atual do motor
bool irrigationTriggered = false; // Indica se a irrigacao foi ativada por umidade baixa

// Configuracoes salvas em EEPROM
int umiThreshold = 45;    // Limite de umidade
int waterLevelThreshold = 200; // Limite de nivel de agua
int tempMax = 60;         // Temperatura maxima (ajustado para 60)

// Controle de tempo
unsigned long previousCheckMillis = 0;
const long checkInterval = 10000; // 10 segundos
unsigned long motorStartTime = 0;
const long motorDuration = 5000; // 5 segundos para teste

// Controle da mensagem "Sem Agua"
bool showWaterWarning = false;

// Arrays para media movel
const int numReadings = 5;
int umiReadings[numReadings];
int tempReadings[numReadings];
int waterReadings[numReadings];
int readingIndex = 0;

void setup() {
  // Configura pinos como entradas e saidas
  pinMode(A0, INPUT);  // Sensor de umidade
  pinMode(A1, INPUT);  // Sensor de temperatura
  pinMode(A2, INPUT);  // Sensor de nivel de agua
  pinMode(motorPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  digitalWrite(ledPin, LOW);

  // Inicializa LCD e comunicacao serial
  lcd_1.begin(16, 2);
  Serial.begin(9600);

  // Forca valores padrao na EEPROM
  umiThreshold = 45;
  waterLevelThreshold = 200;
  tempMax = 60; // Ajustado para 60
  EEPROM.write(0, umiThreshold); // Salva umiThreshold
  EEPROM.write(1, waterLevelThreshold); // Salva waterLevelThreshold
  EEPROM.write(2, tempMax); // Salva tempMax

  // Depuracao inicial para confirmar os valores
  Serial.print("Valores forcados - umiThreshold: "); Serial.println(umiThreshold);
  Serial.print("Valores forcados - waterLevelThreshold: "); Serial.println(waterLevelThreshold);
  Serial.print("Valores forcados - tempMax: "); Serial.println(tempMax);

  // Inicializa arrays de media movel com zeros
  for (int i = 0; i < numReadings; i++) {
    umiReadings[i] = 0;
    tempReadings[i] = 0;
    waterReadings[i] = 0;
  }

  // Exibe mensagem inicial no LCD
  lcd_1.setCursor(0, 0);
  lcd_1.print("Sistema");
  lcd_1.setCursor(0, 1);
  lcd_1.print("Conectado!");
  Serial.println("Sistema Conectado!");
  delay(3000);
  lcd_1.clear();

  // Configura LCD com valores padrao
  lcd_1.setCursor(0, 0);
  lcd_1.print("UMI:   % MOT: 0  ");
  lcd_1.setCursor(0, 1);
  lcd_1.print("Temp:    C");

  Serial.println("Comandos: setUmi [valor], setWater [valor], setTemp [valor], show, testmotor");

  // Inicializa previousCheckMillis
  previousCheckMillis = 0;
}

void loop() {
  unsigned long currentMillis = millis();

  // Le sensores e calcula media movel
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

  // Converte leituras para valores uteis
  umi = map(rawUmi, 0, 1023, 0, 100);
  float voltage = rawTemp * (5.0 / 1023.0);
  temperatura = (voltage - 0.5) * 100;
  waterLevel = rawWater;

  // Depuracao no monitor serial
  Serial.print("Valor bruto do sensor de temperatura (A1): ");
  Serial.println(rawTemp);
  Serial.print("Voltagem: "); Serial.print(voltage, 2); Serial.println(" V");
  Serial.print("Temperatura calculada: ");
  Serial.print(temperatura, 1);
  Serial.println(" C");
  Serial.print("Umidade: "); Serial.print(umi); Serial.println("%");
  Serial.print("Nivel de agua: ");
  Serial.println(waterLevel);
  Serial.print("Temp Max: "); Serial.println(tempMax);
  Serial.print("currentMillis: "); Serial.println(currentMillis);
  Serial.print("previousCheckMillis: "); Serial.println(previousCheckMillis);
  Serial.print("Tempo desde ultima verificacao: "); Serial.println(currentMillis - previousCheckMillis);

  // Controle do LED de alerta
  if (waterLevel <= waterLevelThreshold || temperatura > 60) { // Ajustado para 60
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }

  // Verificacao do motor
  if (!motorOn) {
    Serial.println("Dentro do bloco !motorOn");
    if (currentMillis - previousCheckMillis >= checkInterval) {
      Serial.println("Verificando condicoes para ativar o motor...");
      Serial.print("Condicoes: umi < umiThreshold ("); Serial.print(umi); Serial.print(" < "); Serial.print(umiThreshold);
      Serial.print(") && waterLevel > waterLevelThreshold ("); Serial.print(waterLevel); Serial.print(" > "); Serial.print(waterLevelThreshold);
      Serial.print(") && temperatura <= tempMax ("); Serial.print(temperatura); Serial.print(" <= "); Serial.println(tempMax); Serial.print(")");
      if (umi < umiThreshold && waterLevel > waterLevelThreshold && temperatura <= tempMax) {
        motorOn = true;
        motorStartTime = currentMillis;
        irrigationTriggered = true;
        Serial.println("Irrigacao ativada: Umidade Baixa");
        digitalWrite(motorPin, HIGH);
      }
      previousCheckMillis = currentMillis;
    }
  }

  // Controle da mensagem "Sem Agua"
  if (waterLevel <= waterLevelThreshold) {
    showWaterWarning = true;
    Serial.println("Nivel de agua baixo: exibindo 'Sem Agua'");
  } else {
    showWaterWarning = false;
    Serial.println("Nivel de agua suficiente: removendo 'Sem Agua'");
  }

  // Teste manual do motor
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "testmotor") {
      digitalWrite(motorPin, HIGH);
      Serial.println("Motor testado: LIGADO (desligara apos 5 segundos)");
      delay(5000);
      digitalWrite(motorPin, LOW);
      Serial.println("Motor testado: DESLIGADO");
    }
  }

  // Controla a duracao do motor
  if (motorOn) {
    unsigned long elapsedTime = currentMillis - motorStartTime;
    Serial.print("Tempo decorrido do motor: "); Serial.println(elapsedTime);
    if (elapsedTime >= motorDuration) {
      digitalWrite(motorPin, LOW);
      motorOn = false;
      Serial.println("Motor desativado");
    }
  }

  // Atualiza a linha 1 do LCD (umidade e status do motor)
  if (umi != lastUmi) {
    lcd_1.setCursor(4, 0);
    if (umi < 10) lcd_1.print("  ");
    else if (umi < 100) lcd_1.print(" ");
    lcd_1.print(umi);
    lastUmi = umi;
  }

  if (motorOn != lastMotorOn) {
    lcd_1.setCursor(9, 0);
    lcd_1.print("MOT: ");
    lcd_1.print(motorOn ? "1  " : "0  ");
    lastMotorOn = motorOn;

    // Forca a exibicao da temperatura quando o motor desliga
    if (!motorOn && lastMotorOn) {
      Serial.println("Motor desligado, forcando exibicao da temperatura no LCD");
      lcd_1.setCursor(0, 1);
      lcd_1.print("Temp:       C");
      lcd_1.setCursor(6, 1);
      lcd_1.print(temperatura, 1);
      lastTemperatura = temperatura;
    }
  }

  // Linha 2: Temperatura, "Sem Agua", "Temp Alta" ou razao da irrigacao
  if (motorOn) {
    lcd_1.setCursor(0, 1);
    lcd_1.print(irrigationTriggered ? "Umidade Baixa   " : "24h Prog    ");
  } else if (showWaterWarning) {
    lcd_1.setCursor(0, 1);
    lcd_1.print("Sem Agua      ");
  } else if (temperatura > 60) { // Nova condicao para "Temp Alta"
    lcd_1.setCursor(0, 1);
    lcd_1.print("Temp Alta      ");
    Serial.println("Temperatura alta detectada: exibindo 'Temp Alta'");
  } else {
    lcd_1.setCursor(0, 1);
    lcd_1.print("Temp:       C");
    lcd_1.setCursor(6, 1);
    lcd_1.print(temperatura, 1);
    lastTemperatura = temperatura;
  }

  // Depuracao do estado do motor
  Serial.print("Motor: ");
  Serial.println(motorOn ? "ON" : "OFF");

  // Processa comandos do monitor serial
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.startsWith("setUmi")) {
      int value = command.substring(7).toInt();
      if (value >= 0 && value <= 100) {
        umiThreshold = value;
        EEPROM.write(0, umiThreshold);
        Serial.print("Limite de umidade ajustado para: ");
        Serial.println(umiThreshold);
      }
    } else if (command.startsWith("setWater")) {
      int value = command.substring(8).toInt();
      if (value >= 0 && value <= 1023) {
        waterLevelThreshold = value;
        EEPROM.write(1, waterLevelThreshold);
        Serial.print("Limite de nivel de agua ajustado para: ");
        Serial.println(waterLevelThreshold);
      }
    } else if (command.startsWith("setTemp")) {
      int value = command.substring(7).toInt();
      if (value >= 10 && value <= 60) {
        tempMax = value;
        EEPROM.write(2, tempMax);
        Serial.print("Temperatura maxima ajustada para: ");
        Serial.println(tempMax);
      } else {
        Serial.println("Valor invalido para tempMax. Use entre 10 e 60.");
      }
    } else if (command == "show") {
      Serial.print("Umidade Threshold: "); Serial.println(umiThreshold);
      Serial.print("Water Level Threshold: "); Serial.println(waterLevelThreshold);
      Serial.print("Temp Max: "); Serial.println(tempMax);
    }
  }

  delay(1000);
}