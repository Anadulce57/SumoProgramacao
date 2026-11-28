// Estados: ESPERA, BUSCA, BORDA_RE, BORDA_GIRO
// Ataca a 255 ao notar algo nos sensores laterais (IR) ou frontal (ultrassônico).
// Desvia ao ver a borda (TCRT).

// ---------- Pinos ----------
const int TRIG = 8, ECHO = 7;   // ultrassônico frontal
const int sensorIR1 = 2;        // IR lateral ESQUERDO
const int sensorIR2 = 3;        // IR lateral DIREITO
const int TCRT1 = A0;           // linha - frente
const int TCRT2 = A1;           // linha - trás
const int IN1 = 6, IN2 = 9;     // motor esquerdo
const int IN3 = 10, IN4 = 11;   // motor direito

// ---------- Calibração (TESTE) ----------
const int LIMIAR_LINHA = 500;           // sensibilidade do TCRT
const bool LINHA_ALTA = true;           // true: linha = valor MAIOR que o limiar (como no teste); false: inverte
const int IR_DETECTA = LOW;             // nível do IR quando detecta (confirmado no teste)
const long DIST_LIMITE = 10;            // cm: oponente à frente abaixo disso = atacar
const unsigned long TIMEOUT_ECO = 6000;     // us (cerca de 1 m); curto para não travar o programa
const unsigned long INTERVALO_ULTRA = 40;   // ms entre leituras do ultrassônico
const unsigned long ATRASO_INICIO = 5000;   // verificado no arquivo de regras

// Velocidades (0 a 255)
const int VEL_ATAQUE = 255;   // ultrassônico e IR: sempre 255
const int VEL_GIRO = 200;
const int VEL_BUSCA = 120;
const int VEL_RE = 255;

// Tempos dos estados de borda (ms)
const unsigned long T_RE = 250;
const unsigned long T_GIRO = 200;

// Lado da varredura e do giro depois da borda
const bool GIRAR_ESQ = true;            // true: esquerda; false: direita

// ---------- Depuração ----------
const bool DEBUG = true;                // desligue (false) antes de lutar
const unsigned long INTERVALO_DEBUG = 300;
unsigned long tDebug = 0;

// ---------- Estado dos sensores ----------
int readFront = 0, readBack = 0;          // leitura crua dos TCRT (para calibrar)
bool bFrente = false, bTras = false;      // borda detectada (frente / trás)
bool irEsq = false, irDir = false;        // IR detectando agora
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

bool naBorda(int valor) {
  return LINHA_ALTA ? (valor > LIMIAR_LINHA) : (valor < LIMIAR_LINHA);
}

void lerTCRT() {
  readFront = analogRead(TCRT1);
  readBack = analogRead(TCRT2);
  bFrente = naBorda(readFront);
  bTras = naBorda(readBack);
}

void lerIR() {
  irEsq = (digitalRead(sensorIR1) == IR_DETECTA);
  irDir = (digitalRead(sensorIR2) == IR_DETECTA);
}

// Lê o ultrassônico de tempos em tempos e guarda o último valor
void lerUltrassonico() {
  unsigned long agora = millis();
  if (agora - tUltraLeitura < INTERVALO_ULTRA) return;
  tUltraLeitura = agora;
  distFrente = lerDistancia(TRIG, ECHO);
  oponenteFrente = (distFrente <= DIST_LIMITE);
}

// Impressão não bloqueante, só para testes
void depurar() {
  if (!DEBUG) return;
  unsigned long agora = millis();
  if (agora - tDebug < INTERVALO_DEBUG) return;
  tDebug = agora;

  Serial.print("estado=");
  Serial.print(estado);   // 0 espera, 1 busca, 2 borda_re, 3 borda_giro
  Serial.print(" TCRT F/T=");
  Serial.print(readFront);
  Serial.print("/");
  Serial.print(readBack);
  Serial.print(" borda F/T=");
  Serial.print(bFrente);
  Serial.print("/");
  Serial.print(bTras);
  Serial.print(" IR E/D=");
  Serial.print(irEsq);
  Serial.print("/");
  Serial.print(irDir);
  Serial.print(" dist=");
  Serial.println(distFrente);
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
      else                     girar(GIRAR_ESQ, VEL_BUSCA);    // varredura em lado fixo
      break;

    case BORDA_RE:
      if (bTras) { mudar(BORDA_GIRO); break; }  // linha atrás durante a ré: não recua mais
      mover(-VEL_RE, -VEL_RE);
      if (agora - tEstado >= T_RE) mudar(BORDA_GIRO);
      break;

    case BORDA_GIRO:
      girar(GIRAR_ESQ, VEL_GIRO);               // vira para dentro da arena
      if (agora - tEstado >= T_GIRO) mudar(BUSCA);
      break;
  }

  depurar();
}
