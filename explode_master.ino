#include <TM1637Display.h>
#define SEG_DIO 8
#define SEG_CLK 7
#define BTN 4
#define POT A0
#define KEY A1

TM1637Display disp(SEG_CLK, SEG_DIO);

void setup() {
  // put your setup code here, to run once:
  disp.setBrightness(10);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(KEY, INPUT_PULLUP);
}

void loop() {
  uint8_t vypis[] = { 0, 0, 0, 0 };
  // vytvoření proměnné cas a uložení
  // aktuálního času od zapnutí Arduina
  // v sekundách

  long cas = millis()/1000;

  // výpočet číslic pro jednotlivé pozice
  // na displeji pro zobrazení času
  // např. první pozice udává desítky minut
  // pro 1000 vteřin: 1000s/60=16min/10=1
  // druhá pozice udává jednotky minut
  // pro 1000 vteřin: 1000s/60=16min%10=6
  // znaménko / je dělení, které vrací celou číslici
  // znaménko % je zbytek po dělení

  if(digitalRead(BTN) == LOW){
    disp.showNumberDec(analogRead(POT), false);
    delay(10);
  }
  else {
    vypis[0] = disp.encodeDigit((cas/60)/10);
    vypis[1] = disp.encodeDigit((cas/60)%10);
    vypis[2] = disp.encodeDigit((cas%60)/10);
    vypis[3] = disp.encodeDigit((cas%60)%10);

    // výpis informací na displej
    disp.setSegments(vypis);

    if(digitalRead(KEY) == LOW){
      analogWrite(5, 30);
    }
    else {
      analogWrite(5, 0);
    }

    analogWrite(6, 0);
    delay(500);

    // pro zobrazení dvojtečky mezi číslicemi
    // je nutné k pozici 1 přičíst hodnotu 128
    vypis[1] = vypis[1]+128;

    disp.setSegments(vypis);

    analogWrite(6, 30);
    delay(500);

  }
}
