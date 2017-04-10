#include <SPI.h>
#include <TM1637Display.h>

#define min_time 300
#define max_time 600
#define steps 15
#define threshold 5

#define SEG_DIO 8
#define SEG_CLK 7
#define BTN 4
#define POT A0
#define KEY A1
#define LED1 5
#define LED2 6

// SPI pins
#define LATCH 10
#define DATA 11
#define RESERVED 12
#define CLOCK 13

TM1637Display disp(SEG_CLK, SEG_DIO);

long last_time = 0;
long time = 0;
int strikes;

bool button_released;
bool game_over = true;

int strikes_set = 2;
int number_of_modules;

byte frame[] = {0, // SS byte
                1, // OPCODE
                1, // SIZE (modules, serial, battery count, label)
                1  //    |_ PARAM[0] - number of modules
                };

void setup() {
  disp.setBrightness(10);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(KEY, INPUT_PULLUP);
  Serial.begin(9600);

  // SPI setup
  pinMode(LATCH, OUTPUT);
  digitalWrite(LATCH, LOW);
  SPI.begin();
  digitalWrite(LATCH, HIGH);
  digitalWrite(DATA, HIGH);
  digitalWrite(RESERVED, HIGH);
  digitalWrite(CLOCK, HIGH);

  delay(1000);
  send_frame(); // Send frame with default values
  number_of_modules = frame[3];
}

void change_strikes(){
  if(strikes_set == 2){
    strikes_set = 0;
  }
  else{
    strikes_set = 2;
  }
}

void send_frame(){
  /* TEMPORARY SERIAL COMMUNICATION */
  Serial.println("SENDING A FRAME");

  for (int i = 0; i < 3 + frame[2]; i++) { // 3 + SIZE(PARAM)
    digitalWrite(LATCH, LOW);

    //frame[i] = SPI.transfer(frame[i]);
    Serial.println(frame[i]);

    digitalWrite(LATCH, HIGH);
    delay(100);
  }

  frame[3] = 3;
  Serial.print("PARAM[0] = ");\
  Serial.println(frame[3]);
}

void loop() {
  uint8_t display_output[] = { 0, 0, 0, 0 };
  // vytvoření proměnné time a uložení
  // aktuálního času od zapnutí Arduina
  // v sekundách

  /* SETUP MODE */
  if(digitalRead(KEY) == LOW){

    if(strikes_set == 2){
      analogWrite(LED1, 30);
      analogWrite(LED2, 30);
    }
    else{
      analogWrite(LED1, 0);
      analogWrite(LED2, 0);
    }

    int new_time = (map(analogRead(POT), 1023, 0, min_time, max_time + 10) / steps) * steps;
    if(abs(new_time - time) > threshold){
      time = new_time;
    }

    //Serial.println(time);

    if(digitalRead(BTN) == LOW && button_released){
      button_released = false;
      change_strikes();
      if(strikes_set == 1) Serial.println(strikes_set);
    }
    else if(digitalRead(BTN) == HIGH){
      button_released = true;
    }

    game_over = false;
  }

  /* GAME MODE */
  else{
    delay(1000);

    if(time == 0 && !game_over){
      /* KABOOM! */
      game_over = true;
      frame[0] = 0; // explosion message
      frame[1] = 2;
      frame[2] = 0;
      send_frame();
      Serial.println("GAME OVER");
      return;
    }

    else if(time == 0) return;

    time = time - 1;
    // výpočet číslic pro jednotlivé pozice
    // na displeji pro zobrazení času
    // např. první pozice udává desítky minut
    // pro 1000 vteřin: 1000s/60=16min/10=1
    // druhá pozice udává jednotky minut
    // pro 1000 vteřin: 1000s/60=16min%10=6
    // znaménko / je dělení, které vrací celou číslici
    // znaménko % je zbytek po dělení

    // výpis informací na displej
    disp.setSegments(display_output);
    if(digitalRead(KEY) == LOW){
      analogWrite(5, 30);
    }
    else {
      analogWrite(5, 0);
    }
    // pro zobrazení dvojtečky mezi číslicemi
    // je nutné k pozici 1 přičíst hodnotu 128

    analogWrite(6, 30);

  }

  display_output[0] = disp.encodeDigit((time/60)/10);
  display_output[1] = disp.encodeDigit((time/60)%10);
  display_output[2] = disp.encodeDigit((time%60)/10);
  display_output[3] = disp.encodeDigit((time%60)%10);
  display_output[1] = display_output[1]+128;
  disp.setSegments(display_output);

}
