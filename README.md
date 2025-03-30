# Sistema de Irrigação Automatizado com Arduino no Tinkercad

## Descrição do Projeto
Este projeto implementa um sistema de irrigação automatizado utilizando o simulador Tinkercad e o microcontrolador Arduino Uno. O sistema monitora a umidade do solo, a temperatura ambiente e o nível de água em um reservatório, ativando a irrigação automaticamente quando a umidade está abaixo de um limite configurável. Ele exibe informações em tempo real em um display LCD 16x2 e utiliza um LED para alertar sobre condições adversas, como nível de água baixo ou temperatura alta. Os limites de umidade, nível de água e temperatura podem ser ajustados via monitor serial e salvos na EEPROM, garantindo que as configurações persistam mesmo após o Arduino ser desligado.

O projeto foi desenvolvido para simular uma solução eficiente de irrigação, com foco em automação, economia de água e proteção contra condições inadequadas.

---

## Funcionalidades
- **Monitoramento de Sensores:**
  - Umidade do solo (0% a 100%).
  - Temperatura ambiente (em °C).
  - Nível de água no reservatório (0 a 1023).
- **Irrigação Automatizada:**
  - Ativa a irrigação quando a umidade está abaixo de um limite configurável no monitor serial, desde que o nível de água seja suficiente e a temperatura esteja dentro do limite.
  - Duração da irrigação: 5 segundos (ajustável no código).
- **Display LCD:**
  - Linha 1: Mostra a umidade atual e o estado do motor (ligado/desligado).
  - Linha 2: Mostra a temperatura, mensagens de irrigação ("Umidade Baixa"), ou alertas ("Sem Agua", "Temp Alta").
- **Alertas Visuais:**
  - LED acende se o nível de água estiver baixo ou a temperatura estiver alta.
- **Configuração via Monitor Serial:**
  - Comandos para ajustar limites: `setUmi [valor]`, `setWater [valor]`, `setTemp [valor]`.
  - Comando para visualizar os limites atuais: `show`.
  - Comando para teste manual do motor: `testmotor`.
- **Persistência de Dados:**
  - Limites de configuração são salvos na EEPROM para persistirem após o desligamento do Arduino.

---

## Componentes Utilizados
- **Arduino Uno R3:** Microcontrolador principal.
- **Display LCD 16x2:** Exibe informações (conectado aos pinos D7 a D12: RS, E, D4, D5, D6, D7).
- **Sensor de Umidade do Solo (simulado):** Conectado ao pino analógico A0.
- **Sensor de Temperatura TMP36 (simulado):** Conectado ao pino analógico A1.
- **Potenciômetro (simulando sensor de nível de água):** Conectado ao pino analógico A2.
- **Motor DC (simulando bomba de irrigação):** Conectado ao pino digital D6.
- **LED (vermelho):** Conectado ao pino digital D5 com um resistor de 220 ohms.
- **Resistores:** 220 ohms para o LED e para o backlight do LCD (pino K).
- **Fios de Conexão:** Para conectar os componentes ao Arduino.

---

## Como Usar o Sistema
1. **Inicialização:**
   - Ao iniciar, o LCD exibe "Sistema Conectado!" por 3 segundos.
   - O sistema carrega os valores padrão: umidade limite (45%), nível de água limite (200), temperatura máxima (60°C).
2. **Monitoramento:**
   - O LCD mostra a umidade e o estado do motor na linha 1 ("UMI: [valor]% MOT: [0/1]").
   - A linha 2 mostra a temperatura ("Temp: [valor] C") ou mensagens de status ("Umidade Baixa", "Sem Agua", "Temp Alta").
3. **Irrigação:**
   - A cada 10 segundos, o sistema verifica se a umidade está abaixo do limite, o nível de água está suficiente e a temperatura está dentro do limite.
   - Se as condições forem atendidas, o motor é ativado por 5 segundos.
4. **Alertas:**
   - O LED acende se o nível de água for ≤ 200 ou a temperatura for > 60°C.
   - O LCD exibe "Sem Agua" ou "Temp Alta" conforme a condição.
5. **Comandos do Monitor Serial:**
   - `setUmi [valor]`: Ajusta o limite de umidade (0 a 100). Ex.: `setUmi 50`.
   - `setWater [valor]`: Ajusta o limite de nível de água (0 a 1023). Ex.: `setWater 300`.
   - `setTemp [valor]`: Ajusta a temperatura máxima (10 a 60). Ex.: `setTemp 55`.
   - `show`: Mostra os limites atuais.
   - `testmotor`: Ativa o motor por 5 segundos para teste manual.

---

## Estrutura do Código
- **Bibliotecas:** `LiquidCrystal.h` para o LCD e `EEPROM.h` para salvar configurações.
- **Setup:** Configura os pinos, inicializa o LCD e a comunicação serial, força os valores padrão na EEPROM, e exibe uma mensagem inicial.
- **Loop:**
  - Lê os sensores e calcula médias móveis para umidade, temperatura e nível de água.
  - Verifica a umidade a cada 10 segundos e ativa o motor se necessário.
  - Atualiza o LCD com informações em tempo real.
  - Processa comandos do monitor serial para ajustar configurações.
- **EEPROM:** Salva os limites de umidade, nível de água e temperatura nos endereços 0, 1 e 2.

--- 

## Autores
- **Guilherme Araújo e Miguel Lucas**
- Estudantes de Ciência da Computação, UFS.

![Irrigador Automatico (2)](https://github.com/user-attachments/assets/b57f030e-cd38-4c8b-82f9-146ac03f20a8)
