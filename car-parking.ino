#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

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
#define ANGULO_FECHADO 0
#define ANGULO_ABERTO 90

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
}

// Função para atualizar os LEDs baseado nas distâncias
void atualizarLEDs(int distance1, int distance2, int distance3, int distance4) {
  digitalWrite(LED_VAGA_1, distance1 < 15 ? HIGH : LOW);
  digitalWrite(LED_VAGA_2, distance2 < 15 ? HIGH : LOW);
  digitalWrite(LED_VAGA_3, distance3 < 15 ? HIGH : LOW);
  digitalWrite(LED_VAGA_4, distance4 < 15 ? HIGH : LOW);
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

// Controle de entrada com RFID
void checkRFIDStatus() {
  // Não processa entrada se a catraca de saída estiver aberta
  if (catracaSaidaAberta) {
    Serial.println("[SISTEMA] Entrada bloqueada - Aguardando conclusão da saída");
    return;
  }

  // Verifica se há carro no sensor de entrada
  bool carroPresente = (digitalRead(SENSOR_ENTRADA) == LOW);
  
  // Log da presença do carro na entrada (apenas uma vez)
  if (carroPresente && !carroNaEntradaLogado) {
    Serial.println("[SISTEMA] Veículo aguardando na entrada - Aguardando apresentação do cartão");
    carroNaEntradaLogado = true;
  } else if (!carroPresente) {
    carroNaEntradaLogado = false;
  }

  // Verifica se há novo cartão
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.println("\n[RFID] Cartão detectado!");
    
    if (carroPresente) {
      // Verifica se há vagas disponíveis
      if (checkVagasDisponiveis()) {
        int vagaDisponivel = encontrarVagaDisponivel();
        Serial.println("[RFID] Acesso AUTORIZADO - Há vagas disponíveis");
        Serial.print("[SISTEMA] Dirija-se à vaga ");
        Serial.println(vagaDisponivel);
        Serial.println("[SISTEMA] Abrindo catraca de entrada...");
        catracaEntradaAberta = true;
        tempoAberturaCatracaEntrada = millis();
        controlarServo(true); // Abre a catraca
      } else {
        Serial.println("[RFID] Acesso NEGADO - Todas as vagas estão ocupadas");
      }
    } else {
      Serial.println("[SISTEMA] Erro: Cartão detectado mas não há veículo no sensor de entrada");
    }
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

// Controle da catraca de entrada
void controleCatracaEntrada() {
  if (catracaEntradaAberta) {
    // Verifica se passou o tempo limite (5 segundos)
    if (millis() - tempoAberturaCatracaEntrada >= TEMPO_CATRACA) {
      Serial.println("[SISTEMA] Tempo expirado - Fechando catraca de entrada");
      catracaEntradaAberta = false;
      controlarServo(false); // Fecha a catraca
    }
  }
}

// Controle da catraca de saída
void controleCatracaSaida() {
  // Verifica se há carro no sensor de saída e se a catraca não está já aberta
  if (digitalRead(SENSOR_SAIDA) == LOW && !catracaSaidaAberta) {
    Serial.println("[SISTEMA] Carro detectado no sensor de saída");
    Serial.println("[SISTEMA] Abrindo catraca de saída...");
    catracaSaidaAberta = true;
    tempoAberturaCatracaSaida = millis();
    controlarServo(true); // Abre a catraca
  }
  
  // Controle do tempo de abertura da catraca de saída
  if (catracaSaidaAberta) {
    if (millis() - tempoAberturaCatracaSaida >= TEMPO_CATRACA) {
      Serial.println("[SISTEMA] Tempo expirado - Fechando catraca de saída");
      catracaSaidaAberta = false;
      controlarServo(false); // Fecha a catraca
    }
  }
}

void loop() {
  printVagasStatus();
  
  // Executa o controle de entrada apenas se a saída não estiver em processo
  if (!catracaSaidaAberta) {
    checkRFIDStatus();
    controleCatracaEntrada();
  }
  
  controleCatracaSaida();
  
  delay(1000);
}