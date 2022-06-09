// Receptor - Paridade Par - Grupo

#define PINO_RX 7
#define RTS 6  // emissor->receptor
#define CTS 5  // receptor->emissor
#define BAUD_RATE 1
#define HALF_BAUD 1000 / (2 * BAUD_RATE)

void configuraTemporizador(int baud_rate) {
    int frequencia;
    frequencia = constrain(baud_rate, 1, 1500);
    // set timer1 interrupt
    TCCR1A = 0;  // set entire TCCR1A register to 0
    TCCR1B = 0;  // same for TCCR1B
    TCNT1 = 0;   // initialize counter value to 0
    // OCR1A = contador;// = (16*10^6) / (10*1024) - 1 (must be <65536)
    OCR1A = ((16 * pow(10, 6)) / (1024 * frequencia)) - 1;
    // turn on CTC mode (clear time on compare)
    TCCR1B |= (1 << WGM12);
    // Turn T1 off
    TCCR1B &= 0xF8;
    // Disable interrupts
    TIMSK1 = 0;
    TIFR1 = 0;
}

void iniciaTemporizador() {
    Serial.println("T1 iniciado");
    TCNT1 = 0;  // initialize counter value to 0
    TIFR1 = 0;
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);
    // Set CS10 and CS12 bits for 1024 prescaler
    TCCR1B |= (1 << CS12) | (1 << CS10);
}

void paraTemporizador() {
    Serial.println("T1 parado");
    // Turn T1 off
    TCCR1B &= 0xF8;
    TIMSK1 = 0;
}

int current_bit = 0;  // bit sendo tratado durante a execução
bool flagTemporizador = false;
char currentChar = '\0';  // variável auxiliar para ler o carácter que está
                          // sendo transmitido durante a execução

int buffer[8];
bool stop_bit, parity, start_bit, check_pb;

bool bitParidade(char);
void transmissionError();
char interpretBuffer(int[]);

// Rotina de interrupção do timer1
// O que fazer toda vez que 1s passou?

// 0    1    2    3    4    5    6    7    8    9    10   (len = 11)
// SB  LSB   D2   D3   D4   D5   D6   D7   MSB  PB   EB
ISR(TIMER1_COMPA_vect) {
    // Primeira iteração
    if (current_bit == 0){
        while (digitalRead(PINO_RX) == LOW){
          start_bit = digitalRead(PINO_RX);
          Serial.print("Start bit: ");
          Serial.println(digitalRead(PINO_RX));
          };
    }
    
    // verificando os dados
    else if (current_bit >= 1 && current_bit < 9){
        buffer[current_bit-1] = digitalRead(PINO_RX);
        Serial.print("Dado ");
        Serial.print(current_bit);
        Serial.print(": ");
        Serial.println(digitalRead(PINO_RX));
        }

    // bit de paridade
    else if (current_bit == 9) {
        parity = digitalRead(PINO_RX);
         Serial.print("Paridade ");
         Serial.println(digitalRead(PINO_RX));
        // check_pb = bitParidade(buffer);
        // if (!check_pb) {
        //     transmissionError() /
        // }
    }

    // verifica se houve o encerramento da transmissão
    else if (current_bit == 10) {
        stop_bit = digitalRead(PINO_RX);
         Serial.print("Stop bit ");
         Serial.println(digitalRead(PINO_RX));
        paraTemporizador();
    }
    //
    if (current_bit <= 10) {
        current_bit++;
    } else {
        current_bit = 0;
    }
}
int binaryToDecimal(char binaryNumber[]) {
    int dec = 0;
    int pot = 1;
    for ( int i = 0; i < 8; i ++ ) {
      if( binaryNumber[7-i] == '1' ) {
        for (int y = 0; y < i; y++ ) {
          pot = pot * 2;
        }
          dec = dec + pot;
          pot = 1;
      }
    }   
    return dec;
}

char interpretBuffer(int buffer[]) {
    char aux[8];
    // Swap data
    for (int i = 0; i < 8; i++) aux[i] = buffer[7-i] + 48;
    // Get char
    char result = (char)binaryToDecimal(aux);
    // Verify data integrity
//    if ((int)bitParidade(result) == buffer[9]) {
//        Serial.println("Brabo");
//        return result;
//    } else {
//        Serial.println("Not brabo");
//        transmissionError();
//        return (char)21;  // [NEGATIVE ACKNOWLEDGE]
//    }
    return result;
}

int t0 = millis();

// Calcula bit de paridade - Par ou impar
bool bitParidade(char dado) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        count += bitRead(dado, i);
    }
    return !(count % 2);  // par -> resto 0 -> complemento de 0 -> 1
}

// Para transmissão
void transmissionError() {
    Serial.println("Erro de transmissão: Bit de paridade");
    digitalWrite(CTS, LOW);
    paraTemporizador();
    current_bit = 0;
}


// Executada uma vez quando o Arduino reseta
void setup() {
    // Removendo as interrupções de clock
    noInterrupts();

    // Setando a porta serial (Serial Monitor - Ctrl + Shift + M)
    Serial.begin(9600);

    // Inicializando os pinos
    pinMode(PINO_RX, INPUT);

    // pins de handshake:
    pinMode(RTS, INPUT);
    pinMode(CTS, OUTPUT);

    digitalWrite(CTS, LOW);

    // Setando o temporizador
    configuraTemporizador(BAUD_RATE);

    // habilitando novamente interrupções
    interrupts();
}

// O loop() eh executado continuamente (como um while(true))
void loop() {
//    float t = millis();
//    if (t-t0>500){
//        Serial.print("RTS: ");
//        Serial.println(digitalRead(RTS));
//        Serial.print("CTS: ");
//        Serial.println(digitalRead(CTS));
//        t0 = millis();
//    }
    // RTS em HIGH????
    if (digitalRead(RTS) == HIGH && digitalRead(CTS) == LOW) {
        Serial.println("Transmission initialized. RTS is True. Setting CTS to True.");
        digitalWrite(CTS, HIGH);
        // Configura o contador de bit
        current_bit = 0;
        // Envia a confirmação do Request To Send
        // Inicia a leitura
        iniciaTemporizador();
    }
    // Fim da transmissão
    else if (digitalRead(RTS) == LOW && digitalRead(CTS) == HIGH) {
        Serial.print("Message received: ");
        Serial.println(interpretBuffer(buffer));
        for (int i = 0; i<8; i++) buffer[i] = 0;
        digitalWrite(CTS, LOW);
        Serial.println("CTS set to False");
        Serial.println("Transmission finished");
        paraTemporizador();
        current_bit = 0;
    }
}
