/**
HAKKO A1321
for 900M 900L 907 908 913 914
https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
*/

#include <EEPROM.h> // Для работы со встроенной памятью ATmega

// #define data_pin     4  // DS     14-Pin
// #define clk_pin      5  // SH_CP  11-Pin
// #define latch_pin    6  // ST_CP  12-Pin
#define termopara_solder      A0
#define termopara_hot         A2
#define analog_speed_hot      A1
#define hot_power             8
#define solder_pwm            9
#define FEN_FAN_PIN           10
#define hot_rele              11
#define hot_button            12

// solder termopara scaler
float scaler_solder = 2.0;

// hot termopara scaler
float scaler_hot = 4.0;

// массив для выбора октетов
const uint8_t positionSEG[] = {8, 4, 2, 64, 32, 16}; // 15 нога не задействована

// массив для преобразования цыфр
const uint8_t digitSEG[] = {
  254,  // 0
  56,   // 1
  183,  // 2
  189,  // 3
  121,  // 4
  221,  // 5
  223,  // 6
  184,  // 7
  255,  // 8
  253,  // 9
  1,    // -
  195,  // F
};

unsigned long currentTime,loopTime;
int16_t SetSolderTemp = 290, SetHotTemp = 100, SolderTemp = 0, HotTemp = 0;
int8_t EncMove = 0, EncFlag, EncLast, EncCurrent;
uint8_t p_count = 0, dispSetTemp = 0, speed_hot = 0, speed_tmp = 0, hot_enable = 0;
boolean btn = false, hot_flag = false, btn_flag = false, cooler_flag = false;;
uint16_t pressCount = 0;

void setup() {
  Read_EEPROM();

  // set up fast ADC mode
   ADCSRA = (ADCSRA & 0xf8) | 0x04; // set 16 times division
   
  // Timer 1 62 500 Гц Pin 9 10
  TCCR1A = TCCR1A & 0xe0 | 1;
  TCCR1B = TCCR1B & 0xe0 | 0x09;

  // 4,5,6 порты для 74HC595 настраиваем на вывод 
  DDRD |= B01110000;

   // 2,3,7 на ввод
  DDRD &= ~B10001100;
  // Подтягиваем ножки к 1
  PORTD |= B10001100;

  pinMode(hot_rele, OUTPUT);
  pinMode(hot_button, INPUT);
  digitalWrite(hot_button, HIGH);

  currentTime = millis();
  loopTime = currentTime;

  analogWrite(solder_pwm, 50);
  analogWrite(FEN_FAN_PIN, 0);
  noTone(hot_power);
}

void loop() {
while (1) { 
  currentTime = millis();
  //-------------------Проверка каждые 5 мс
  if(currentTime >= (loopTime + 5)) {
    CheckEncoder();
    CheckBtn();
    // Счетчик прошедшего времени      
    loopTime = currentTime;
    p_count++;
  }

  if (p_count > 30) {
    p_count = 0;
    if (dispSetTemp > 0) dispSetTemp--;
    
    SolderTemp = ((AvrValue(termopara_solder))/4) * scaler_solder;
    HotTemp = ((AvrValue(termopara_hot))/4) * scaler_hot;
    speed_tmp = (int)(analogRead(analog_speed_hot)/6);
    
    if ((speed_hot != speed_tmp) && hot_flag) {
      speed_hot = speed_tmp;
      analogWrite(FEN_FAN_PIN, (speed_hot + 15 > 255 ? 255 : speed_hot + 15));
    }
    
    SetSolder();
    SetHot();
  }

  if (EncFlag) {
    if (btn == true) {
      SetHotTemp += EncMove;
      if (SetHotTemp < 0) SetHotTemp = 0;
      if (SetHotTemp > 480) SetHotTemp = 480;
    }
    else {
      SetSolderTemp  += EncMove;
      if (SetSolderTemp < 0 ) SetSolderTemp = 0;
      if (SetSolderTemp > 400) SetSolderTemp = 400;
    }
    EncFlag = false;
    dispSetTemp = 10;
  }

  if (dispSetTemp > 0) {
    PrintTemp(SetSolderTemp, SetHotTemp);
  }
  else {
    PrintTemp(SolderTemp, HotTemp);
  }
}}//End loop

/*
Выводит одной строкой температуру паяльника и фена
*/
void PrintTemp( uint16_t stemp, uint16_t htemp ) {
  uint8_t digitArr[] = {0,0,0,0,0,0};
  uint8_t i;

  // преобразуем числа в отдельные цифры
  // в начале шкалы темепература паяльника
  digitArr[5] = htemp/100;
  i = htemp % 100;
  digitArr[4] = i/10;
  digitArr[3] = i%10;

  digitArr[2]   = stemp/100;
  i = stemp % 100;
  digitArr[1] = i/10;
  digitArr[0] = i%10;

  for (i = 0; i < 6; i++) {
    PORTD &= ~B01000000;
    ShuftOut(~digitSEG[ digitArr[i] ]);
    ShuftOut(positionSEG[i]);
    PORTD |= B01000000;
  }
}

/*
  Побитово выводит цифру через 74HC595
  Но если надо выставить 0 так, чтобы не прибить остальные биты в 0,
  нужен оператор "И", обозначающийся &. Так же к нему понадобится оператор "НЕ" - обозначается ~.
  PORTD &= ~B01000000;
  Отправляем байт на последнею микросхему в цепочке
  инвертирование битов исключающим или ^
  ShuftOut( SEG[ solder_s[i] ] ^ 0xFF );
  Отправляем байт на вторую микросхему в цепочке
  ShuftOut( SEG[ hold_s[i] ] ^ 0xFF );
  Зажигает по очереди сегменты на обоих индикаторах
  ShuftOut(NUM_SEG[i]);
  digitalWrite(latch_pin, HIGH);
  PORTD |= B01000000;
*/
void ShuftOut( uint8_t value ) {
    for (uint8_t i = 0; i < 8; i++) {
     // digitalWrite(data_pin,(value & (0x80 >> i)));  //MSB
     value & (0x80 >> i) ? PORTD |= B00010000 : PORTD &= ~B00010000;
     //digitalWrite(clk_pin, HIGH);
     PORTD |= B00100000;
     //digitalWrite(clk_pin, LOW);
     PORTD &= ~B00100000;
    }
}
// End ShiftOut

void CheckEncoder() {
  EncCurrent = PIND & B00001100;
  EncMove = 0;
  if( EncCurrent != EncLast ) {
    // 8 - 12 - 4 - 0 - 8
    switch (EncCurrent) {
      case 0  : EncLast == 4 ? EncMove = 1 : EncMove = -1; break;
      case 4  : EncLast == 12 ? EncMove = 1 : EncMove = -1; break;
      case 8  : EncLast == 0 ? EncMove = 1 : EncMove = -1; break;
      case 12 : EncLast == 8 ? EncMove = 1 : EncMove = -1; break;
      default: break;
    }

    EncLast = EncCurrent;
    EncFlag = true;
  }//End Проверка состояния encoder
}//End Check Enc

void CheckBtn() {
  if (((PIND & B10000000) == 0) && (btn_flag == false)) {
    btn_flag = true;
  }

  if (((PINB & B00010000) == 0) && (btn_flag == false)) {
    hot_flag = !hot_flag;
    if (hot_flag) {
      analogWrite(FEN_FAN_PIN, 150);
      digitalWrite(hot_rele, HIGH);
      cooler_flag = true;
    }
    else {
      noTone(hot_power);
      digitalWrite(hot_rele, LOW);
      analogWrite(FEN_FAN_PIN, 255);
    }
    btn_flag = true;
  }

  if(!btn_flag) {
    return;
  }

  if ((btn_flag == true) && ((PIND & B10000000) != 0) && ((PINB & B00010000) != 0)) {
    if(pressCount < 700) {
      btn = !btn;
    }

    pressCount = 0;
    btn_flag = false;
    return;
  }

  if(pressCount < 1000) {
    pressCount++;
  }

  if(pressCount > 700){
    if(btn) {
      EEPROM.put(2, SetHotTemp);
    }
    else {
      EEPROM.put(0, SetSolderTemp);
    }

    pressCount = 0;
    btn_flag = false;
  }
}

uint8_t SetSolder() {
  // SolderTemp 0 .. 255
  // SetSolderTemp 0 .. 400
  int k = 0;
  k = (int)(SetSolderTemp - SolderTemp);

  if (k > 10) { analogWrite(solder_pwm, 255); return 0; }
  if (k > 5)  { analogWrite(solder_pwm, 220); return 0; }
  if (k > 3)  { analogWrite(solder_pwm, 200); return 0; }
  if (k > 2)  { analogWrite(solder_pwm, 150); return 0; }
  if (k > 1)  { analogWrite(solder_pwm, 120); return 0; }
  if (k >= 0) { analogWrite(solder_pwm, 100); return 0; }
  if (k < -100) { digitalWrite(solder_pwm, LOW); return 0; }
  if (k < -20) { analogWrite(solder_pwm, 10); return 0; }
  if (k < -10) { analogWrite(solder_pwm, 20); return 0; }
  if (k < -5) { analogWrite(solder_pwm, 30); return 0; }
  if (k < -2) { analogWrite(solder_pwm, 40); return 0; }
  if (k < 0) { analogWrite(solder_pwm, 80); return 0; }
  digitalWrite(solder_pwm,LOW);
  return 0;
}

uint8_t SetHot() {
  // HotTemp 0 .. 255
  // SetHotTemp 0 .. 480
  if (!hot_flag && (HotTemp < 50) && cooler_flag) {
    noTone(hot_power);
    digitalWrite(FEN_FAN_PIN, LOW);
    cooler_flag = false;
    return 0;
  }

  int k = 0;
  k = (int)(SetHotTemp - HotTemp);

  if (k > 50)   { tone(hot_power, 100, 1000); return 0; }
  if (k > 30)   { tone(hot_power, 100, 600); return 0; }
  if (k > 20)   { tone(hot_power, 100, 300); return 0; }
  if (k > 10)   { tone(hot_power, 100, 100); return 0; }
  if (k > 5)    { tone(hot_power, 100, 50); return 0; }
  if (k > 0)    { tone(hot_power, 100, 30); return 0; }
  if (k < -50)  { tone(hot_power, 100, 1); return 0; }
  if (k < -30)  { tone(hot_power, 100, 5); return 0; }
  if (k < -20)  { tone(hot_power, 100, 10); return 0; }
  if (k > -10)  { tone(hot_power, 100, 15); return 0; }
  if (k > -5)   { tone(hot_power, 100, 20); return 0; }
  if (k < 0)    { tone(hot_power, 100, 25); return 0; }
  noTone(hot_power);
  return 0;
}

uint16_t AvrValue(uint8_t pin) {
  uint16_t val[14], tmp;
  uint8_t i, j, count;

  count = (sizeof(val)/sizeof(int));

  for (i = 0; i < count; i++) {
    val[i] = analogRead(pin);
  }

  for (i = 0; i < count; i++) {
  for (j = 0; j < count - i - 1; j++) {
    if (val[j] > val[j+1]) {
      tmp = val[j];
      val[j] = val[j+1];
      val[j+1] = tmp;
    }
  }}

  return val[(int)(count/2)];
}

/*
  EEPROM
  0-1   (2 Byte) Solder T
  2-3   (2 Byte) Hot T
*/
void Read_EEPROM() {
  EEPROM.get(0, SetSolderTemp);
  if (SetSolderTemp > 1000 || SetSolderTemp < 0) {
    SetSolderTemp = 300;
    EEPROM.put(0, SetSolderTemp);
  }

  EEPROM.get(2, SetHotTemp);
  if (SetHotTemp > 1000 || SetHotTemp < 0) {
    SetHotTemp = 100;
    EEPROM.put(0, SetHotTemp);
  }
}