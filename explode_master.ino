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

// serial pins
#define RX 0
#define TX 1

// SPI pins
#define LATCH 10
#define DATA 11
#define RESERVED 12
#define CLOCK 13

TM1637Display disp(SEG_CLK, SEG_DIO);

long last_time = 0;
long time = 0;
int strikes;
int solved;

bool button_released;
bool init_sent = false;
bool game_over = true;

int strikes_set = 2;
int number_of_modules;

int something_connected = true;

byte frame[] = {0, // SS byte
                1, // OPCODE
                1, // SIZE (modules, serial, battery count, label)
                1  //    |_ PARAM[0] - number of modules
                };

void setup() {
  disp.setBrightness(10);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(KEY, INPUT_PULLUP);
  
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT);  
  digitalWrite(TX, HIGH);
  delay(3000);
  
  if(digitalRead(RX) == LOW){
    something_connected = false;
  }
  
  digitalWrite(TX, LOW);
  Serial.begin(9600);

  /* SPI setup
  pinMode(LATCH, OUTPUT);
  digitalWrite(LATCH, LOW);
  SPI.begin();
  digitalWrite(LATCH, HIGH);
  digitalWrite(DATA, HIGH);
  digitalWrite(RESERVED, HIGH);
  digitalWrite(CLOCK, HIGH);*/
    
}

void change_strikes(){
  if(strikes_set == 2){
    strikes_set = 0;
  }
  else{
    strikes_set = 2;
  }
}
void send_init(){
  frame[0] = 0; // SS byte
  frame[1] = 1; // OPCODE
  frame[2] = 1; // SIZE (modules, serial, battery count, label)
  frame[3] = 1;  //    |_ PARAM[0] - number of modules
  
  send_frame(); // Send frame with default values
  while(1){ // wait for response
    if(Serial.available() == 4){
      Serial.readBytes(frame, 4);
      number_of_modules = frame[3];
      break;
    }
  }
}

void send_update(){
  frame[0] = 0;
  frame[1] = 5; // OPCODE = 5 -> UPDATE
  frame[2] = 2;
  frame[3] = strikes;
  frame[4] = solved;
  send_frame();
}

void send_explosion(){
  frame[0] = 0; // explosion message
  frame[1] = 2;
  frame[2] = 0;
  send_frame();
}

void send_frame(){
  for (int i = 0; i < 3 + frame[2]; i++) { // 3 + SIZE(PARAM)
    Serial.write(frame[i]);
  }
}

void receive_and_send_messages(){
  if(Serial.available() >= 3){
    Serial.readBytes(frame, 3);
    
    if(frame[1] == 4){ // OPCODE = 4 -> STRIKE
      strikes++;
      Serial.readBytes(frame, 1);
      update_strikes();
      send_update();
    }
    else if(frame[1] == 6){ // OPCODE = 6 -> SOLVED
      solved++;
      Serial.readBytes(frame, 1);
      send_update();
    }
    else{ // discard the rest of the message
      Serial.readBytes(frame, frame[2]);
    }
  }
}

void update_strikes(){
  if(strikes == 0){
    analogWrite(LED1, 0);
    analogWrite(LED2, 0);
  }
  else if(strikes == 1 && strikes_set == 3){
    analogWrite(LED1, 30);
    analogWrite(LED2, 0);
  }
  else if(strikes == 2 || strikes_set == 3){
    analogWrite(LED1, 30);
    analogWrite(LED2, 30);
  }
  else if(strikes == 3 && strikes_set == 3 
       || strikes == 1 && strikes_set == 1){
    analogWrite(LED1, 30);
    analogWrite(LED2, 30);
  }
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
    init_sent = false;
    strikes = 0;
  }

  /* GAME MODE */
  else{
    delay(1000);

    receive_and_send_messages();

    if(!init_sent && something_connected){
      update_strikes();
      send_init();
      init_sent = true;
      return;
    }

    if((time == 0 || strikes == strikes_set) && !game_over && init_sent){
      /* KABOOM! */
      game_over = true;
      
      display_output[0] = 0x71; // displays "FAIL"
      display_output[1] = 0x77;
      display_output[2] = 0x06;
      display_output[3] = 0x38;
      disp.setSegments(display_output);
      
      send_explosion();
      return;
    }

    else if(time == 0) return;

    else if(solved == number_of_modules){
      display_output[0] = 0x5e; // displays "DONE"
      display_output[1] = 0x5c;
      display_output[2] = 0x54;
      display_output[3] = 0x79;
      disp.setSegments(display_output);
      time = 0;
    }

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
