#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define os pinos para cada sensor ultrassônico
#define TRIG_PIN_1 30
#define ECHO_PIN_1 31
#define TRIG_PIN_2 34
#define ECHO_PIN_2 35
#define TRIG_PIN_3 32
#define ECHO_PIN_3 33
#define TRIG_PIN_4 36
#define ECHO_PIN_4 37

// Define os pinos para os LEDs
#define LED_VAGA_1 13
#define LED_VAGA_2 12
#define LED_VAGA_3 11
#define LED_VAGA_4 10

// Define os pinos para os sensores E18-D80NK
#define SENSOR_ENTRADA 9  // Primeiro sensor
#define SENSOR_SAIDA 8    // Segundo sensor

// Define os pinos para o RFID-RC522
#define SS_PIN 53    // SDA
#define RST_PIN 5    // RST
MFRC522 rfid(SS_PIN, RST_PIN);

// Define o pino e cria objeto do Servo Motor
#define SERVO_PIN 6
Servo catracaServo;

// Ângulos do servo motor
#define ANGULO_FECHADO 115
#define ANGULO_ABERTO 240


// Inicialização do LCD
LiquidCrystal_I2C lcd(0x3F, 20, 4);

// Variáveis para controle das catracas
bool catracaEntradaAberta = false;
bool catracaSaidaAberta = false;
unsigned long tempoAberturaCatracaEntrada = 0;
unsigned long tempoAberturaCatracaSaida = 0;
const unsigned long TEMPO_CATRACA = 5000; // 5 segundos em millisegundos
bool carroNaEntradaLogado = false;  // Nova variável para controlar o log do carro na entrada

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  Serial.println("Iniciando setup...");


 // Inicialização do LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema Iniciado!");
  delay(2000);  // Exibe a mensagem por 2 segundos
  lcd.clear();


  // Configura os pinos dos LEDs como saída
  pinMode(LED_VAGA_1, OUTPUT);
  pinMode(LED_VAGA_2, OUTPUT);
  pinMode(LED_VAGA_3, OUTPUT);
  pinMode(LED_VAGA_4, OUTPUT);
  
  // Configura os sensores E18-D80NK
  pinMode(SENSOR_ENTRADA, INPUT);
  pinMode(SENSOR_SAIDA, INPUT);
  
  // Configura o pino SS como saída e coloca em nível alto
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  
  // Inicializa SPI e RFID
  SPI.begin();
  Serial.println("SPI iniciado");
  
  rfid.PCD_Init();
  delay(500);
  
  if (!rfid.PCD_PerformSelfTest()) {
    Serial.println("Teste inicial falhou, tentando reiniciar...");
    rfid.PCD_Reset();
    delay(500);
    rfid.PCD_Init();
  }
  
  byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print("Versão do RFID: 0x");
  Serial.println(v, HEX);
  
  if (v == 0x00 || v == 0xFF) {
    Serial.println("ERRO: RFID não detectado!");
    Serial.println("Por favor, verifique as conexões");
    while (1);
  }
  
  // Configura os pinos dos sensores ultrassônicos
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
  pinMode(TRIG_PIN_3, OUTPUT);
  pinMode(ECHO_PIN_3, INPUT);
  pinMode(TRIG_PIN_4, OUTPUT);
  pinMode(ECHO_PIN_4, INPUT);
  
  // Inicializa o Servo Motor
  catracaServo.attach(SERVO_PIN);
  catracaServo.write(ANGULO_FECHADO); // Inicia com a catraca fechada
  
  Serial.println("Sistema de estacionamento iniciado!");
    delay(2000);
  lcd.clear();
  lcd.print("Insira seu cartao");
}

// Função para atualizar os LEDs baseado nas distâncias
void atualizarLEDs(int distance1, int distance2, int distance3, int distance4) {
  digitalWrite(LED_VAGA_1, distance1 < 15 ? HIGH : LOW);
  digitalWrite(LED_VAGA_2, distance2 < 15 ? HIGH : LOW);
  digitalWrite(LED_VAGA_3, distance3 < 15 ? HIGH : LOW);
  digitalWrite(LED_VAGA_4, distance4 < 15 ? HIGH : LOW);
}

// Função para contar o número de vagas disponíveis
int contarVagasDisponiveis() {
  int vagas = 0;
  vagas += readDistance(TRIG_PIN_1, ECHO_PIN_1) >= 15 ? 1 : 0;
  vagas += readDistance(TRIG_PIN_2, ECHO_PIN_2) >= 15 ? 1 : 0;
  vagas += readDistance(TRIG_PIN_3, ECHO_PIN_3) >= 15 ? 1 : 0;
  vagas += readDistance(TRIG_PIN_4, ECHO_PIN_4) >= 15 ? 1 : 0;
  return vagas;
}

void atualizarLCD(const char* mensagemLinha1, const char* mensagemLinha2, int vagasDisponiveis) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(mensagemLinha1);
  lcd.setCursor(0, 1);
  lcd.print(mensagemLinha2);
  lcd.setCursor(0, 3);
  lcd.print("Vagas presentes: ");
  lcd.print(vagasDisponiveis);
}

// Função para leitura da distância dos sensores ultrassônicos
int readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;
  return distance;
}

// Função para encontrar uma vaga disponível
int encontrarVagaDisponivel() {
  int distance1 = readDistance(TRIG_PIN_1, ECHO_PIN_1);
  int distance2 = readDistance(TRIG_PIN_2, ECHO_PIN_2);
  int distance3 = readDistance(TRIG_PIN_3, ECHO_PIN_3);
  int distance4 = readDistance(TRIG_PIN_4, ECHO_PIN_4);
  
  // Atualiza os LEDs
  atualizarLEDs(distance1, distance2, distance3, distance4);
  
  // Array com as vagas disponíveis
  int vagasDisponiveis[4] = {0, 0, 0, 0};
  int numVagasDisponiveis = 0;
  
  // Verifica quais vagas estão disponíveis
  if (distance1 >= 15) vagasDisponiveis[numVagasDisponiveis++] = 1;
  if (distance2 >= 15) vagasDisponiveis[numVagasDisponiveis++] = 2;
  if (distance3 >= 15) vagasDisponiveis[numVagasDisponiveis++] = 3;
  if (distance4 >= 15) vagasDisponiveis[numVagasDisponiveis++] = 4;
  
  // Se houver vagas disponíveis, escolhe uma aleatoriamente
  if (numVagasDisponiveis > 0) {
    int indiceAleatorio = random(numVagasDisponiveis);
    return vagasDisponiveis[indiceAleatorio];
  }
  
  return 0; // Retorna 0 se não houver vagas disponíveis
}

// Verifica se há vagas disponíveis no estacionamento
bool checkVagasDisponiveis() {
  int distance1 = readDistance(TRIG_PIN_1, ECHO_PIN_1);
  int distance2 = readDistance(TRIG_PIN_2, ECHO_PIN_2);
  int distance3 = readDistance(TRIG_PIN_3, ECHO_PIN_3);
  int distance4 = readDistance(TRIG_PIN_4, ECHO_PIN_4);
  
  // Atualiza os LEDs
  atualizarLEDs(distance1, distance2, distance3, distance4);
  
  Serial.println("\nDistâncias medidas:");
  Serial.print("Vaga 1: "); Serial.print(distance1); Serial.println("cm");
  Serial.print("Vaga 2: "); Serial.print(distance2); Serial.println("cm");
  Serial.print("Vaga 3: "); Serial.print(distance3); Serial.println("cm");
  Serial.print("Vaga 4: "); Serial.print(distance4); Serial.println("cm");

  return (distance1 >= 15 || distance2 >= 15 || distance3 >= 15 ||  distance4 >= 15);
}

// Imprime o status atual das vagas
void printVagasStatus() {
  int distance1 = readDistance(TRIG_PIN_1, ECHO_PIN_1);
  int distance2 = readDistance(TRIG_PIN_2, ECHO_PIN_2);
  int distance3 = readDistance(TRIG_PIN_3, ECHO_PIN_3);
  int distance4 = readDistance(TRIG_PIN_4, ECHO_PIN_4);
  
  // Atualiza os LEDs
  atualizarLEDs(distance1, distance2, distance3, distance4);
  
  Serial.println("\n=== Status das Vagas ===");
  Serial.println(distance1 < 15 ? "Vaga 1: Ocupada" : "Vaga 1: Livre");
  Serial.println(distance2 < 15 ? "Vaga 2: Ocupada" : "Vaga 2: Livre");
  Serial.println(distance3 < 15 ? "Vaga 3: Ocupada" : "Vaga 3: Livre");
  Serial.println(distance4 < 15 ? "Vaga 4: Ocupada" : "Vaga 4: Livre");
  Serial.println("=====================");
}

// Controle do servo motor
void controlarServo(bool abrir) {
  if (abrir) {
    catracaServo.write(ANGULO_ABERTO);
    Serial.println("[SERVO] Catraca aberta");
  } else {
    catracaServo.write(ANGULO_FECHADO);
    Serial.println("[SERVO] Catraca fechada");
  }
}

// Controle de entrada com RFID (com atualização do LCD)
void checkRFIDStatus() {
  if (catracaSaidaAberta) {
    atualizarLCD("Aguardando saida", "Entrada bloqueada", contarVagasDisponiveis());
    return;
  }

  bool carroPresente = (digitalRead(SENSOR_ENTRADA) == LOW);
  if (carroPresente && !carroNaEntradaLogado) {
    atualizarLCD("Aguardando Cartao", "", contarVagasDisponiveis());
    carroNaEntradaLogado = true;
  } else if (!carroPresente) {
    carroNaEntradaLogado = false;
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (carroPresente) {
      if (checkVagasDisponiveis()) {
        int vagaDisponivel = encontrarVagaDisponivel();
        atualizarLCD("Acesso AUTORIZADO", "Dirija-se a vaga", contarVagasDisponiveis());
        catracaEntradaAberta = true;
        tempoAberturaCatracaEntrada = millis();
        controlarServo(true);
      } else {
        atualizarLCD("Acesso NEGADO", "Vagas ocupadas", contarVagasDisponiveis());
      }
    } else {
      atualizarLCD("Erro: Cartao", "Sem veiculo na entrada", contarVagasDisponiveis());
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

// Controle da catraca de entrada (com atualização do LCD)
void controleCatracaEntrada() {
  if (catracaEntradaAberta) {
    if (millis() - tempoAberturaCatracaEntrada >= TEMPO_CATRACA) {
      atualizarLCD("Tempo expirado", "Fechando catraca", contarVagasDisponiveis());
      catracaEntradaAberta = false;
      controlarServo(false);
    }
  }
}


// Controle da catraca de saída (com atualização do LCD)
void controleCatracaSaida() {
  if (digitalRead(SENSOR_SAIDA) == LOW && !catracaSaidaAberta) {
    atualizarLCD("Saida Liberada", "Aguarde...", contarVagasDisponiveis());
    catracaSaidaAberta = true;
    tempoAberturaCatracaSaida = millis();
    controlarServo(true);
  }

  if (catracaSaidaAberta) {
    if (millis() - tempoAberturaCatracaSaida >= TEMPO_CATRACA) {
      atualizarLCD("Saida concluida", "Fechando catraca", contarVagasDisponiveis());
      catracaSaidaAberta = false;
      controlarServo(false);
    }
  }
}

void loop() {
  checkRFIDStatus();
  controleCatracaEntrada();
  controleCatracaSaida();
  
  // Atualize o LCD periodicamente com o status das vagas e mensagem padrão
  int vagasDisponiveis = contarVagasDisponiveis();
  atualizarLCD("Sistema Operando", "", vagasDisponiveis);
  
  delay(1000); // Pequeno atraso para evitar muitas atualizações no LCD
}