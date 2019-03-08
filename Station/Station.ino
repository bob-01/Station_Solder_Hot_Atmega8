// #define data_pin     4  // DS     14-Pin
// #define clk_pin      5  // SH_CP  11-Pin
// #define latch_pin    6  // ST_CP  12-Pin
#define p_solder  A0
#define p_hot     A1
#define solder_pwm  9
#define hot_pwm     10

// массив для выбора октетов
const uint8_t positionSEG[] = {1, 2, 4, 8, 16, 32};

// массив для преобразования цыфр
const uint8_t digitSEG[] = {
	63,	// 0
	6,	// 1
	91,	// 2
	79,	// 3
	102,	// 4
	109,	// 5
	253,	// 6
	7,	// 7
	255,	// 8
	239,	// 9
};

unsigned long currentTime,loopTime;
uint16_t SolderTemp = 290, HotTemp = 295;
int8_t EncMove = 0;
uint8_t EncFlag, EncLast, EncCurrent;

uint8_t analog_solder = 0, p_count = 0, chg_flag = 0;
uint8_t analog_hot = 0;
uint16_t analog_tmp = 0;

boolean btn = false, btn_flag = false;

int incomingByte = 0;    // for incoming serial data

void setup() {
  // Serial.begin(9600); // инициализируем порт, скорость 9600
    
  // 4,5,6 порты для 74HC595 настраиваем на вывод 
  DDRD |= B01110000;

   // 2,3,7 на ввод
  DDRD &= ~B10001100;
  // Подтягиваем ножки к 1
  PORTD |= B10001100;

  currentTime = millis();
  loopTime = currentTime;

  pinMode(solder_pwm, OUTPUT);
  pinMode(hot_pwm, OUTPUT);
  analogWrite(solder_pwm, 127);
  analogWrite(hot_pwm, 127);
}

void loop() {
while (1) {
  /*
  // send data only when you receive data:
  if (Serial.available() > 0) {
      // read the incoming byte:
    incomingByte = Serial.read();
      // say what you got:
    Serial.print((char)incomingByte);
  }
  */
 
  currentTime = millis();
  //-------------------Проверка каждые 5 мс
  if(currentTime >= (loopTime + 5)) {
    CheckEncoder();
    CheckBtn();
    // Счетчик прошедшего времени      
    loopTime = currentTime;
    p_count++;
  }

  if (p_count > 20) {
    p_count = 0;
    
    analog_tmp = analogRead(p_solder)/4;

    if (analog_solder != analog_tmp) {
      analog_solder = analog_tmp;
      chg_flag = 1;
      analogWrite(solder_pwm, analog_solder);
    }

    analog_tmp = analogRead(p_hot)/4;

    if (analog_hot != analog_tmp) {
      analog_hot = analog_tmp;
      chg_flag = 1;
      analogWrite(hot_pwm, analog_hot);
    }

    if (chg_flag) {
      chg_flag = 0;
      Serial.print(analog_solder);
      Serial.print(" : ");
      Serial.println(analog_hot);
    }
  }

  if (EncFlag) {
    if (btn == true) HotTemp += EncMove;
    else SolderTemp  += EncMove;
    EncFlag = false;
  }

  PrintTemp( SolderTemp, HotTemp);

}}//End loop

/*
Выводит одной строкой температуру паяльника и фена
*/
void PrintTemp( uint16_t solder_temp, uint16_t hot_temp ) {
  uint8_t digitArr[] = {0,0,0,0,0,0};
  uint8_t i;

  // преобразуем числа в отдельные цифры
  // в начале шкалы темепература паяльника
  digitArr[5] = hot_temp/100;
  i = hot_temp % 100;
  digitArr[4] = i/10;
  digitArr[3] = i%10;

  digitArr[2]   = solder_temp/100;
  i = solder_temp % 100;
  digitArr[1] = i/10;
  digitArr[0] = i%10;

  for (i = 0; i < 6; i++) {
    PORTD &= ~B01000000;
    ShuftOut(~digitSEG[ digitArr[i] ]);
    ShuftOut(positionSEG[i]);
    // delay(5);
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
// End ShuftOut

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

    Serial.print(EncCurrent, DEC);
    Serial.println();

    EncLast = EncCurrent;
    EncFlag = true;
  }//End Проверка состояния encoder
}//End Check Enc

void CheckBtn() {
  if (((PIND & B10000000) == 0) && (btn_flag == false)) {
    btn = !btn;
    btn_flag = true;

    Serial.print("btn :");
    Serial.println(btn);
  }

  if ((btn_flag == true) && ((PIND & B10000000) != 0)) btn_flag = false;
}

