//Programação robô sumô
//Declaração sensores
//Sensor ultrassônico
const int TRIG = 8, ECHO = 7; 
//Sensor IR obstáculos
const int sensorIR1 = 13;
const int sensorIR2 = 12;
//Sensor TCRT5000
const int TCRT1 = 4;
const int TCRT2 = 2;
//Declaração de motores 
//Motor esquerdo: IN1/IN2 | Motor direito: IN3/IN4 - Adição
const int IN1 = 10, IN2 = 9;
const int IN3 = 6, IN4 = 5;
// Função padrão
long lerDistancia(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  long duracao = pulseIn(pinEcho, HIGH, 25000);
  if (duracao == 0) return 999;
  return duracao / 58;
}

void setup() {
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT); //Sensor ultrassônico
  pinMode(TCRT1, INPUT); pinMode(TCRT2, INPUT);
  pinMode(sensorIR1, INPUT); pinMode(sensorIR2, INPUT);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); //Motores
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
}

void loop() {

}
