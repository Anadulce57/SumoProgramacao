// ===================== ROBÔ SUMÔ (v4) =====================
const int TRIG = 8, ECHO = 7;   // ultrassônico frontal
const int sensorIR1 = 2;        // IR lateral ESQUERDO
const int sensorIR2 = 3;        // IR lateral DIREITO
const int TCRT1 = A0;           // linha - frente
const int TCRT2 = A1;           // linha - trás
const int IN1 = 6, IN2 = 9;     // motor esquerdo
const int IN3 = 10, IN4 = 11;   // motor direito
// ---------- Calibração (TESTE) ----------
const int LIMIAR_LINHA = 500;           // sensibilidade do TCRT
const bool LINHA_ALTA = true;           // true: linha = valor MAIOR que o limiar; false: inverte
const int IR_DETECTA = 1;               // nível do IR quando detecta (confirmar no módulo)
const unsigned long ATRASO_INICIO = 5000;   // verificado no arquivo de regras
const long DIST_ATAQUE = 15;            // cm: oponente à frente abaixo disso = atacar
const unsigned long TIMEOUT_ECO = 6000;     // us (cerca de 1 m); curto para não travar o programa
const unsigned long INTERVALO_ULTRA = 40;   // ms entre leituras do ultrassônico
// Velocidades (0 a 255)
const int VEL_ATAQUE = 255;   // ultrassônico e IR: sempre 255
const int VEL_GIRO = 200;
const int VEL_BUSCA = 120;
const int VEL_RE = 255;
// Tempos dos estados de borda (ms)
const unsigned long T_RE = 250;
const unsigned long T_GIRO = 200;
// ---------- Previsão de ataque lateral ----------
const float ALFA = 0.3;                 // peso dos eventos recentes (0 a 1)
const int MIN_EVENTOS = 5;              // eventos mínimos antes de confiar na previsão
const float MARGEM = 0.10;              // dentro de 0,5 +- MARGEM, considera empate
const bool LADO_PADRAO_ESQ = true;      // lado usado sem dados ou em empate
const unsigned long DEBOUNCE_IR = 150;  // ms: ignora novo evento do mesmo sensor

float pEsq = 0.5;       // probabilidade do próximo ataque vir da ESQUERDA (direita = 1 - pEsq)
int totalEventos = 0;   // ataques laterais registrados na luta
// ---------- Estado dos sensores ----------
bool bFrente = false, bTras = false;      // borda detectada (frente / trás)
bool irEsq = false, irDir = false;        // IR detectando agora
bool irEsqAnt = false, irDirAnt = false;  // IR no ciclo anterior
unsigned long tEvEsq = 0, tEvDir = 0;     // instante do último evento de cada lado
long distFrente = 999;                    // última distância do ultrassônico (cm)
bool oponenteFrente = false;
unsigned long tUltraLeitura = 0;
// ---------- Máquina de estados ----------
enum Estado {
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
// ---------- Motores ----------
// vel de -255 a 255: o pino que recebe o PWM define o sentido, o outro fica em 0
void motor(uint8_t a, uint8_t b, int vel) {
  int pwm = constrain(abs(vel), 0, 255);  // módulo da velocidade, limitado a 0-255
  analogWrite(a, vel > 0 ? pwm : 0);
  analogWrite(b, vel < 0 ? pwm : 0);
}

void mover(int esq, int dir) {
  motor(IN1, IN2, esq);
  motor(IN3, IN4, dir);
}

// Gira no lugar: true = esquerda, false = direita
void girar(bool esquerda, int vel) {
  if (esquerda) mover(-vel, vel);
  else mover(vel, -vel);
}

// Ataque lateral: avança a 255 em curva para o lado do oponente (roda de dentro a 1/3)
void atacarLateral(bool esquerda) {
  if (esquerda) mover(VEL_ATAQUE / 3, VEL_ATAQUE);
  else mover(VEL_ATAQUE, VEL_ATAQUE / 3);
}

// ---------- Sensores ----------
// Função padrão - ultrassônico (devolve a distância em cm)
long lerDistancia(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  long duracao = pulseIn(pinEcho, HIGH, TIMEOUT_ECO);
  if (duracao == 0) return 999;  // sem eco: nada à frente
  return duracao / 58;
}

bool naBorda(uint8_t pino) {
  int v = analogRead(pino);
  return LINHA_ALTA ? (v > LIMIAR_LINHA) : (v < LIMIAR_LINHA);
}

void lerTCRT() {
  bFrente = naBorda(TCRT1);
  bTras = naBorda(TCRT2);
}

// ---------- Previsão ----------
// Chamada a cada ataque lateral novo. Atualiza a média móvel exponencial.
void registrarAtaque(bool esquerda) {
  if (estado == ESPERA) return;  // não conta antes da luta começar
  float evento = esquerda ? 1.0 : 0.0;
  pEsq = ALFA * evento + (1.0 - ALFA) * pEsq;
  totalEventos++;
}

// true se o próximo ataque tende a vir da esquerda
bool ladoProvavelEsq() {
  if (totalEventos < MIN_EVENTOS) return LADO_PADRAO_ESQ;   // poucos dados
  if (fabs(pEsq - 0.5) < MARGEM) return LADO_PADRAO_ESQ;    // empate
  return pEsq > 0.5;
}

// Lê os IR e conta só a TRANSIÇÃO para "detectado" (um ataque = um evento)
void lerIR() {
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

// Lê o ultrassônico de tempos em tempos e guarda o último valor
void lerUltrassonico() {
  unsigned long agora = millis();
  if (agora - tUltraLeitura < INTERVALO_ULTRA) return;
  tUltraLeitura = agora;
  distFrente = lerDistancia(TRIG, ECHO);
  oponenteFrente = (distFrente <= DIST_ATAQUE);
}

// ---------- Programa principal ----------
void setup() {
  Serial.begin(9600);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);  // sensor ultrassônico
  pinMode(sensorIR1, INPUT);
  pinMode(sensorIR2, INPUT);
  const uint8_t saidas[] = {IN1, IN2, IN3, IN4};
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
      if (bFrente) { mudar(BORDA_RE); break; }                // borda: prioridade máxima
      if (bTras)   { mover(VEL_ATAQUE, VEL_ATAQUE); break; }  // linha atrás: avança

      if (oponenteFrente)      mover(VEL_ATAQUE, VEL_ATAQUE);  // ultrassônico: ataca a 255 em frente
      else if (irEsq && irDir) mover(VEL_ATAQUE, VEL_ATAQUE);  // dos dois lados: avança a 255
      else if (irEsq)          atacarLateral(true);            // IR esquerdo: ataca a 255 para a esquerda
      else if (irDir)          atacarLateral(false);           // IR direito: ataca a 255 para a direita
      else                     girar(ladoProvavelEsq(), VEL_BUSCA);  // varredura pelo lado mais provável
      break;

    case BORDA_RE:
      if (bTras) { mudar(BORDA_GIRO); break; }  // linha atrás durante a ré: não recua mais
      mover(-VEL_RE, -VEL_RE);
      if (agora - tEstado >= T_RE) mudar(BORDA_GIRO);
      break;

    case BORDA_GIRO:
      girar(ladoProvavelEsq(), VEL_GIRO);       // vira para o lado do ataque mais provável
      if (agora - tEstado >= T_GIRO) mudar(BUSCA);
      break;
  }
}
