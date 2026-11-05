//Declaração sensores
//Sensor ultrassônico
const int TRIG = 8, ECHO = 7; 
//Sensor IR obstáculos
const int sensorIR1 = 2;
const int sensorIR2 = 3;
//Sensor TCRT5000
const int TCRT1 = A0;
const int TCRT2 = A1;
//Declaração de motores 
//Motor esquerdo: IN1/IN2 | Motor direito: IN3/IN4 - Adição
const int IN1 = 10, IN2 = 9;
const int IN3 = 6, IN4 = 5;
//Declarações
const int LIMIAR_LINHA = 500 //TESTE! - SENSIBILIDADE TCRT 
const unsigned long ATRASO_INICIO = 5000 //VERIFICADO NO ARQUIVO DE REGRAS 
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

enum Estado { //Estou pensando em declarar direto no switch
  ESPERA,
  BUSCA,
  BORDA_RE,
  BORDA_GIRO
}

Estado estado = ESPERA
unsigned long tEstado = 0;

void mudar(Estado e) {
  estado = e;
  tEstado = milis();
}

//Velocidade de -255 e 255 - PWM
void motor(uint8_t a, uint8_t b, int vel){ 
  int pwm = constrain(abs(vel), 0, 255); //Função padrão de PWM
  analogWrite(a, vel > 0 ?  pwm: 0);
  analogWrite(b, vel < 0 ? pwm: 0); 
}

void mover(int esq, int dir){
  motor(IN1, IN2, esq); //CHECAR SE É ESQUERDA MESMO
  motor(IN3, IN4, dir); //CHECAR SE É DIREITA MESMO 
}

bool naBorda(uint8_t piso){
  return analogRead(piso) < LIMIAR_LINHA;
}

void lerTCRT(){

}

void lerIR(){

}

void lerUltrassonico(){
  
}
void setup() {
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT); //Sensor ultrassônico
  pinMode(sensorIR1, INPUT); pinMode(sensorIR2, INPUT);

  const uint8_t saidas [] = {IN1, IN2, IN3, IN4};
  for (uint8_t p : saidas) pinMode(OUTPUT);

  mudar(ESPERA);
}

void loop() {

}
