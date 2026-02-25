#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const byte ROWS = 4; 
const byte COLS = 5; 
char keys[ROWS][COLS] = {
  {'L', '7', '4', '1' ,'f'}, 
  {'0', '8', '5', '2','F'},
  {'R', '9', '6', '3','#'},
  {'E', 'X', 'D', 'U','*'}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; 
byte colPins[COLS] = {6, 7, 8, 9, 10}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String inputString = "";     
float valorSetado = 4.00;    
bool modoVoltagem = false;   
int telaAtual = 0;           
const int pinBuzzer = A3;

void enviarParaDAC(int valor10bits) {
  Wire.beginTransmission(0x09);
  Wire.write(highByte(valor10bits)); // Parte alta do número (os bits de cima)
  Wire.write(lowByte(valor10bits));  // Parte baixa do número (os bits de baixo)
  Wire.endTransmission();
}

void emitirBip(int frequencia, int duracao) {
  tone(pinBuzzer, frequencia, duracao);
}

void atualizarSistema() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (telaAtual == 1) { // --- TELA F1: AJUDA ---
    display.setTextSize(1);
    display.setCursor(25, 0);
    display.println(F("AJUDA (MENU)"));
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    display.setCursor(0, 18);
    display.println(F("L: mA  |  R: VCC"));
    display.println(F("U/D: +/- 0.1"));
    display.println(F("#/*: +/- 0.01"));
    display.println(F("E: Confirmar Digito"));
    display.setCursor(0, 54);
    display.print(F("Pressione X p/ Sair"));
  } 
  else if (telaAtual == 2) { // --- TELA F2: INFO ---
    display.setTextSize(1);
    display.setCursor(20, 0);
    display.println(F("INFORMACOES"));
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println(F("Modelo: MAF-INST V1.0"));
    display.println(F("marcelorbpi@gmail.com"));
    display.println(F("Hardware: Dual-Mini"));
    display.setCursor(0, 54);
    display.print(F("Pressione X p/ Sair"));
  } 
  else { // --- TELA 0: TRABALHO ---
    // Procure esta linha dentro da função atualizarSistema()
    int valorDAC = modoVoltagem ? (valorSetado / 10.0) * 1023.0 : (valorSetado - 4.0) / 16.0 * 1023.0;

    display.drawRect(0, 0, 128, 14, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(8, 3);
    display.print(modoVoltagem ? F("MODO: TENSAO (VCC)") : F("MODO: CORRENTE (mA)"));

    display.setTextSize(2);
    display.setCursor(20, 20);
    display.print(valorSetado, 2);
    display.print(modoVoltagem ? F(" V") : F(" mA"));

    // Exibe o que está sendo digitado
    display.setTextSize(1);
    display.setCursor(4, 42);
    display.print(F("SET: "));
    if (inputString.length() > 0) {
        display.print(inputString);
        display.print(F(" (Pressione E)"));
    } else {
        display.print(F("L<mA|R>V"));
    }

    int larguraBarra = modoVoltagem ? map(valorSetado * 100, 0, 1000, 0, 120) : map(valorSetado * 100, 400, 2000, 0, 120);
    display.drawRect(4, 52, 120, 7, SSD1306_WHITE);
    display.fillRect(4, 52, larguraBarra, 7, SSD1306_WHITE);
  }
  display.display();
}

void setup() {
  Wire.begin();
  pinMode(pinBuzzer, OUTPUT);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  atualizarSistema();
}

void loop() {
  char key = keypad.getKey();
  
  if (key) {
    emitirBip(3500, 15);
    
    if (key == 'f') { telaAtual = 1; inputString = ""; }
    else if (key == 'F') { telaAtual = 2; inputString = ""; }
    else if (key == 'X') { telaAtual = 0; inputString = ""; }
    else if (key == 'R' && !modoVoltagem) { modoVoltagem = true; valorSetado = 0.00; inputString = ""; }
    else if (key == 'L' && modoVoltagem) { modoVoltagem = false; valorSetado = 4.00; inputString = ""; }
    
    else if (telaAtual == 0) {
      if (key >= '0' && key <= '9') { 
        if (inputString.length() < 4) inputString += key;
      } 
      else if (key == 'U') valorSetado += 0.1;  
      else if (key == 'D') valorSetado -= 0.1;  
      else if (key == '#') valorSetado += 0.01; 
      else if (key == '*') valorSetado -= 0.01; 
      else if (key == 'E') { 
        if (inputString.length() > 0) {
          float vDigitado = inputString.toFloat() / 100.0;
          if (!modoVoltagem) {
            if (vDigitado >= 4.0 && vDigitado <= 20.0) valorSetado = vDigitado;
            else emitirBip(200, 500);
          } else {
            if (vDigitado >= 0.0 && vDigitado <= 10.0) valorSetado = vDigitado;
            else emitirBip(200, 500);
          }
          inputString = "";
        }
      }
    }

    if (!modoVoltagem) { 
      if (valorSetado > 20.0) valorSetado = 20.0; if (valorSetado < 4.0) valorSetado = 4.0; 
    } else { 
      if (valorSetado > 10.0) valorSetado = 10.0; if (valorSetado < 0.0) valorSetado = 0.0; 
    }
    atualizarSistema();
  }
}
