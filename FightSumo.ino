//NãO se esqueça de checar antes a bendita da linha 55!
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
const int IN1 = 6, IN2 = 9;
const int IN3 = 10, IN4 = 11;
//Declarações
const int LIMIAR_LINHA = 500; //TESTE! - SENSIBILIDADE TCRT 
const bool LINHA_ALTA = true; // true: linha = valor MAIOR que o limiar (como no teste); false: inverte - TESTE 
const int IR_DETECTA = 1;  
const unsigned long ATRASO_INICIO = 5000; //VERIFICADO NO ARQUIVO DE REGRAS 
const unsigned long DIST_ATAQUE = 30;
const unsigned long TIMEOUT_ECO = 6000;     // µs (cerca de 1 m); curto para não travar o programa
const unsigned long INTERVALO_ULTRA = 40;  // ms entre leituras do ultrassônico

//Velocidade
const int VEL_ATAQUE = 255;
const int VEL_GIRO = 200;
const int VEL_BUSCA = 120;
const int VEL_RE = 255;

// Tempos dos estados de borda (ms)
const unsigned long T_RE = 250;
const unsigned long T_GIRO = 200;

//  Previsão de ataque lateral 
const float ALFA = 0.3;                   // peso dos eventos recentes (0 a 1)
const int MIN_EVENTOS = 5;                // eventos mínimos antes de confiar na previsão
const float MARGEM = 0.10;                // dentro de 0,5 +- MARGEM, considera empate
const bool LADO_PADRAO_ESQ = true;        // lado usado sem dados ou em empate
const unsigned long DEBOUNCE_IR = 150;    // ms: ignora novo evento do mesmo sensor

float pEsq = 0.5;       // probabilidade do próximo ataque vir da ESQUERDA (direita = 1 - pEsq)
int totalEventos = 0;   // ataques laterais registrados na luta

// Depuração - vulgo limpa
const bool DEBUG = true;                  // desligue (false) antes de lutar
const unsigned long INTERVALO_DEBUG = 300;
unsigned long tDebug = 0;

//  Estado dos sensores
bool bFrente = false, bTras = false;      // borda detectada (frente / trás)
bool irEsq = false, irDir = false;        // IR detectando agora
bool irEsqAnt = false, irDirAnt = false;  // IR no ciclo anterior
unsigned long tEvEsq = 0, tEvDir = 0;     // instante do último evento de cada lado
long distFrente = 999;                    // última distância do ultrassônico (cm)
bool oponenteFrente = false;
unsigned long tUltraLeitura = 0;

enum Estado { //Estou pensando em declarar direto no switch
  ESPERA,
  BUSCA,
  BORDA_RE,
  BORDA_GIRO
}; 

Estado estado = ESPERA;
unsigned long tEstado = 0;

void mudar(Estado e) {
  estado = e;
  tEstado = millis();
}

//Motores -255 e 255 - PWM
void motor(uint8_t a, uint8_t b, int vel){ 
  int pwm = constrain(abs(vel), 0, 255); //Função padrão de PWM
  analogWrite(a, vel > 0 ?  pwm: 0);
  analogWrite(b, vel < 0 ? pwm: 0); 
}

void mover(int esq, int dir){
  motor(IN1, IN2, esq); //CHECAR SE É ESQUERDA MESMO
  motor(IN3, IN4, dir); //CHECAR SE É DIREITA MESMO 
}

// Gira no lugar: true = esquerda, false = direita
void girar(bool esquerda, int vel) {
  if (esquerda) mover(-vel, vel);
  else mover(vel, -vel);
}

// Sensores 

// Função padrão
long lerDistancia(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  long duracao = pulseIn(pinEcho, HIGH, TIMEOUT_ECO));
  if (duracao == 0) return 999;
  return duracao / 58;
}

bool naBorda(uint8_t pino){
  int v = analogRead(pino);
  return LINHA_ALTA ? (v > LIMIAR_LINHA) : (v < LIMIAR_LINHA);
}

void lerTCRT(){
  bFrente = naBorda(TCRT1);
  bTras = naBorda(TCRT2);
}

// Previsão - prévia do IR
// Chamada a cada ataque lateral novo. Atualiza a média móvel exponencial.
void registrarAtaque(bool esquerda) {
  if (estado == ESPERA) return;  // não conta antes da luta começar
  float evento = esquerda ? 1.0 : 0.0;
  pEsq = ALFA * evento + (1.0 - ALFA) * pEsq;
  totalEventos++;
}

// true se o próximo ataque tende a vir da esquerda
bool ladoProvavelEsq() {
  if (totalEventos < MIN_EVENTOS) return LADO_PADRAO_ESQ;     // poucos dados
  if (fabs(pEsq - 0.5) < MARGEM) return LADO_PADRAO_ESQ;      // empate
  return pEsq > 0.5;
}

void lerIR(){
   unsigned long agora = millis();
  irEsq = (digitalRead(sensorIR1) == IR_DETECTA);
  irDir = (digitalRead(sensorIR2) == IR_DETECTA);

  if (irEsq && !irEsqAnt && (agora - tEvEsq > DEBOUNCE_IR)) {
    registrarAtaque(true);
    tEvEsq = agora;
  }
  if (irDir && !irDirAnt && (agora - tEvDir > DEBOUNCE_IR)) {
    registrarAtaque(false);
    tEvDir = agora;
  }

  irEsqAnt = irEsq;  // atualiza sempre, inclusive em ESPERA
  irDirAnt = irDir;
}

void lerUltrassonico(){
  unsigned long agora = millis();
  if (agora - tUltraLeitura < INTERVALO_ULTRA) return;
  tUltraLeitura = agora;
  distFrente = lerDistancia(TRIG, ECHO);
  oponenteFrente = (distFrente <= DIST_ATAQUE);
}

// Impressão não bloqueante, só para testes
void depurar() {
  if (!DEBUG) return;
  unsigned long agora = millis();
  if (agora - tDebug < INTERVALO_DEBUG) return;
  tDebug = agora;

  Serial.print("estado=");
  Serial.print(estado);
  Serial.print(" borda F/T=");
  Serial.print(bFrente);
  Serial.print("/");
  Serial.print(bTras);
  Serial.print(" IR E/D=");
  Serial.print(irEsq);
  Serial.print("/");
  Serial.print(irDir);
  Serial.print(" dist=");
  Serial.print(distFrente);
  Serial.print(" pEsq=");
  Serial.print(pEsq, 2);
  Serial.print(" eventos=");
  Serial.println(totalEventos);
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT); //Sensor ultrassônico
  pinMode(sensorIR1, INPUT); pinMode(sensorIR2, INPUT);

  const uint8_t saidas [] = {IN1, IN2, IN3, IN4};
  for (uint8_t p : saidas) pinMode(p, OUTPUT);

  mudar(ESPERA);
}

void loop() {
  unsigned long agora = millis();

  // 1. Ler tudo, em todo ciclo, antes de decidir
  lerTCRT();
  lerIR();
  lerUltrassonico();

  // 2. Decidir conforme o estado
  switch (estado) {
    case ESPERA:
      mover(0, 0);
      if (agora - tEstado >= ATRASO_INICIO) mudar(BUSCA);
      break;

    case BUSCA:
      if (bFrente) { mudar(BORDA_RE); break; }                  // borda: prioridade máxima
      if (bTras)   { mover(VEL_ATAQUE, VEL_ATAQUE); break; }    // linha atrás: avança

      if (oponenteFrente)      mover(VEL_ATAQUE, VEL_ATAQUE);   // alinhado: investe
      else if (irEsq && irDir) mover(VEL_ATAQUE, VEL_ATAQUE);   // dos dois lados: avança e sai do meio
      else if (irEsq)          girar(true, VEL_GIRO);           // oponente à esquerda
      else if (irDir)          girar(false, VEL_GIRO);          // oponente à direita
      else                     girar(ladoProvavelEsq(), VEL_BUSCA);  // varredura pelo lado mais provável
      break;

    case BORDA_RE:
      if (bTras) { mudar(BORDA_GIRO); break; }   // linha atrás durante a ré: não recua mais
      mover(-VEL_RE, -VEL_RE);
      if (agora - tEstado >= T_RE) mudar(BORDA_GIRO);
      break;

    case BORDA_GIRO:
      girar(ladoProvavelEsq(), VEL_GIRO);        // vira para o lado do ataque mais provável
      if (agora - tEstado >= T_GIRO) mudar(BUSCA);
      break;
  }

  depurar();
}
