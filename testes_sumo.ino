// Código para testes no final de semana
//Sensor ultrassônico
const int TRIG = 8, ECHO = 7;
const unsigned long TIMEOUT_ECO = 6000;  
const long DIST_LIMITE = 10;             // cm - TESTES

//Sensor IR 
const int sensorIR1 = 2;  // esquerdo
const int sensorIR2 = 3;  // direito
const int IR_DETECTA = LOW;  
int readLeft = 0;
int readRight = 0;
//Snesor TCRT
const int TCRT1 = A0;  // frente
const int TCRT2 = A1;  // trás
const int LIMIAR_LINHA = 500;  // TESTE! - sensibilidade do TCRT
int readFront = 0;
int readBack = 0;

const int IN1 = 6, IN2 = 9;
const int IN3 = 10, IN4 = 11;

const unsigned long ATRASO_INICIO = 5000; 

// Função padrão - ultrassônico (devolve a distância em cm)
long lerDistancia(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  long duracao = pulseIn(pinEcho, HIGH, TIMEOUT_ECO);
  if (duracao == 0) return 999;  
  return duracao / 58;
}

void lerTCRT() {
  readFront = analogRead(TCRT1);
  readBack = analogRead(TCRT2);

  // Se a lógica estiver invertida na arena, trocar > por < (provável)
  Serial.print("FRENTE [Valor: ");
  Serial.print(readFront);
  Serial.print("] -> ");
  if (readFront > LIMIAR_LINHA) {
    Serial.println("linha verificada");
  } else {
    Serial.println("linha NÃO verificada");
  }

  Serial.print("TRÁS [Valor: ");
  Serial.print(readBack);
  Serial.print("] -> ");
  if (readBack > LIMIAR_LINHA) {
    Serial.println("linha verificada");
  } else {
    Serial.println("linha NÃO verificada");
  }
}

void lerIR() {
  readLeft = digitalRead(sensorIR1);
  readRight = digitalRead(sensorIR2);

  bool esq = (readLeft == IR_DETECTA);
  bool dir = (readRight == IR_DETECTA);

  Serial.print("IR esquerdo: ");
  Serial.print(esq ? "detectado" : "livre");
  Serial.print(" | IR direito: ");
  Serial.println(dir ? "detectado" : "livre");
}

void lerUltrassonico() {
  long dist = lerDistancia(TRIG, ECHO);

  bool obst = (dist <= DIST_LIMITE);

  Serial.print("Distância (cm): ");
  Serial.println(dist);

  if (obst) {
    Serial.println("Obstáculo à frente");
    // função de ato -> definir mudança de estado dependendo
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);  // sensor ultrassônico
  pinMode(sensorIR1, INPUT);
  pinMode(sensorIR2, INPUT);
  const uint8_t saidas[] = {IN1, IN2, IN3, IN4};
  for (uint8_t p : saidas) pinMode(p, OUTPUT);
}

void loop() {
  //lerIR();                
  // lerTCRT();           
  lerUltrassonico();  
  delay(500);            
}
