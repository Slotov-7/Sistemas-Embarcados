#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Inicializa o LCD com endereço I2C 0x20
LiquidCrystal_I2C lcd(0x20, 16, 2);

const int botaoEstacao = 2;   // Botão para chegada/saída do metrô
const int botaoPresenca = 3;  // Botão para detectar presença
const int botaoNovoDia = 4;   // Botão para simular novo dia
const int ledEstacao = 13;    // LED indicando metrô na estação
const int ledMovimento = 12;  // LED indicando metrô em movimento

// Mapeamento de endereços na EEPROM
const int ADDR_TODAY_COUNT = 0;      // Contagem do dia atual (2 bytes)
const int ADDR_YESTERDAY_COUNT = 2;  // Contagem do dia anterior (2 bytes)
const int ADDR_DAY_FLAG = 4;         // Indicador de dia (1 byte)
const int ADDR_STATION_STATE = 5;    // Estado da estação (1 byte)

// Variáveis
int contagem_hoje = 0;       // Contagem de passageiros hoje
int contagem_ontem = 0;      // Contagem de passageiros ontem
int estadoEstacao = 0;       // 0: não está na estação, 1: na estação
bool novoDia = false;        // Flag para mudança de dia
unsigned long lastDayCheck = 0;
const unsigned long dayCheckInterval = 60000; // Verifica novo dia a cada minuto

// Rastreamento do estado dos botões
int ultimoEstadoBotaoEstacao = HIGH;
int ultimoEstadoBotaoPresenca = HIGH;
int ultimoEstadoBotaoNovoDia = HIGH;
unsigned long ultimoTempoEstacao = 0;
unsigned long ultimoTempoPresenca = 0;
unsigned long ultimoTempoNovoDia = 0;
const int debounceDelay = 100;

void setup() {
  Serial.begin(9600);
  pinMode(botaoEstacao, INPUT_PULLUP);
  pinMode(botaoPresenca, INPUT_PULLUP);
  pinMode(botaoNovoDia, INPUT_PULLUP);
  pinMode(ledEstacao, OUTPUT);
  pinMode(ledMovimento, OUTPUT);

  lcd.init();
  lcd.backlight();

  // Carrega dados salvos da EEPROM
  contagem_hoje = readIntFromEEPROM(ADDR_TODAY_COUNT);
  contagem_ontem = readIntFromEEPROM(ADDR_YESTERDAY_COUNT);
  estadoEstacao = EEPROM.read(ADDR_STATION_STATE) != 0 ? 1 : 0;

  // Define estados iniciais dos LEDs com base no estado da estação
  digitalWrite(ledEstacao, estadoEstacao ? HIGH : LOW);
  digitalWrite(ledMovimento, estadoEstacao ? LOW : HIGH);

  // Verifica se este é um novo dia
  checkForNewDay();

  // Exibição inicial
  updateLCD();

  Serial.println("Sistema Metro UFS-FFA iniciado");
  Serial.println("Contagem hoje: " + String(contagem_hoje) + ", ontem: " + String(contagem_ontem));
  Serial.println("Estado do metro: " + String(estadoEstacao == 1 ? "Na estacao" : "Em movimento"));
}

void loop() {
  unsigned long currentMillis = millis();

  // Verifica periodicamente se é um novo dia
  if (currentMillis - lastDayCheck >= dayCheckInterval) {
    checkForNewDay();
    lastDayCheck = currentMillis;
  }

  // Botão de chegada/saída do metrô
  int leituraBotaoEstacao = digitalRead(botaoEstacao);

  if (leituraBotaoEstacao != ultimoEstadoBotaoEstacao && currentMillis - ultimoTempoEstacao > debounceDelay) {
    if (leituraBotaoEstacao == LOW) {
      // Alterna o estado do metrô
      estadoEstacao = !estadoEstacao;

      if (estadoEstacao) {
        // Metrô chegou
        digitalWrite(ledEstacao, HIGH);
        digitalWrite(ledMovimento, LOW);
        Serial.println("NOTIFICACAO: Metro chegou na estacao");

        // Atualiza o display
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Metro na Estacao");
        lcd.setCursor(0, 1);
        lcd.print("Pessoas: ");
        lcd.print(contagem_hoje);
      } else {
        // Metrô partindo
        digitalWrite(ledEstacao, LOW);
        digitalWrite(ledMovimento, HIGH);

        // Envia notificação para sala de controle
        Serial.println("NOTIFICACAO: Metro saindo - Pessoas embarcadas hoje: " + String(contagem_hoje));

        // Salva a contagem atual e o estado da estação na EEPROM
        writeIntToEEPROM(ADDR_TODAY_COUNT, contagem_hoje);
        EEPROM.write(ADDR_STATION_STATE, estadoEstacao);

        updateLCD();
      }
    }
    ultimoTempoEstacao = currentMillis;
  }
  ultimoEstadoBotaoEstacao = leituraBotaoEstacao;

  // Botão de contagem de passageiros - só funciona quando o metrô está na estação
  int leituraBotaoPresenca = digitalRead(botaoPresenca);

  if (leituraBotaoPresenca != ultimoEstadoBotaoPresenca && currentMillis - ultimoTempoPresenca > debounceDelay) {
    if (leituraBotaoPresenca == LOW && estadoEstacao == 1) {  // Só conta quando o metrô está na estação
      contagem_hoje++;
      Serial.println("Pessoa detectada. Total hoje: " + String(contagem_hoje));

      // Atualiza o display com a nova contagem
      lcd.setCursor(0, 1);
      lcd.print("Pessoas: ");
      lcd.print(contagem_hoje);
      lcd.print("   ");
    }
    ultimoTempoPresenca = currentMillis;
  }
  ultimoEstadoBotaoPresenca = leituraBotaoPresenca;

  // Simula novo dia com botão
  int leituraBotaoNovoDia = digitalRead(botaoNovoDia);

  if (leituraBotaoNovoDia != ultimoEstadoBotaoNovoDia && currentMillis - ultimoTempoNovoDia > debounceDelay) {
    if (leituraBotaoNovoDia == LOW) {
      novoDia = true;
      Serial.println("Novo dia simulado!");
      checkForNewDay();
    }
    ultimoTempoNovoDia = currentMillis;
  }
  ultimoEstadoBotaoNovoDia = leituraBotaoNovoDia;
}

// Verifica se um novo dia começou
void checkForNewDay() {
  byte currentDayFlag = EEPROM.read(ADDR_DAY_FLAG);

  if (novoDia) {
    // Calcula e armazena a diferença antes de resetar
    int diferenca = contagem_hoje - contagem_ontem;

    // É um novo dia - reseta contador e atualiza registros
    contagem_ontem = contagem_hoje;
    contagem_hoje = 0;

    // Armazena os dados
    writeIntToEEPROM(ADDR_YESTERDAY_COUNT, contagem_ontem);
    writeIntToEEPROM(ADDR_TODAY_COUNT, contagem_hoje);

    // Alterna a flag de dia para próxima verificação
    EEPROM.write(ADDR_DAY_FLAG, currentDayFlag ^ 1);

    Serial.println("Novo dia detectado! Contagem de ontem: " + String(contagem_ontem));
    Serial.println("Diferenca entre dias: " + String(diferenca >= 0 ? "+" : "") + String(diferenca));

    novoDia = false;
    updateLCD();
  }
}

void updateLCD() {
  lcd.clear();

  if (estadoEstacao) {
    // Metrô está na estação
    lcd.setCursor(0, 0);
    lcd.print("Metro na Estacao");
    lcd.setCursor(0, 1);
    lcd.print("Pessoas: ");
    lcd.print(contagem_hoje);
  } else {
    // Metrô não está na estação
    lcd.setCursor(0, 0);
    lcd.print("Metro UFS-FFA");
    lcd.setCursor(0, 1);

    // Se for um novo dia, mostrar a comparação com o dia anterior
    // e não com o contador atual (que estará em 0)
    if (contagem_ontem > 0) {
      int diferenca = contagem_hoje - contagem_ontem;

      if (contagem_hoje == 0 && contagem_ontem > 0) {
        // Se for início do dia (contagem zerada)
        lcd.print("Ontem: ");
        lcd.print(contagem_ontem);
      } else {
        // Durante o dia, mostra contagem e diferença
        lcd.print("Hoje: ");
        lcd.print(contagem_hoje);
        lcd.print(" ");
        lcd.print(diferenca >= 0 ? "+" : "");
        lcd.print(diferenca);
      }
    } else {
      lcd.print("Pessoas: ");
      lcd.print(contagem_hoje);
    }
  }
}

// Funções auxiliares para operações na EEPROM
void writeIntToEEPROM(int address, int value) {
  EEPROM.write(address, value & 0xFF);
  EEPROM.write(address + 1, (value >> 8) & 0xFF);
}

int readIntFromEEPROM(int address) {
  return EEPROM.read(address) | (EEPROM.read(address + 1) << 8);
}