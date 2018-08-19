// #define data_pin     2  // DS     14-Pin
// #define clk_pin      3  // SH_CP  11-Pin
// #define latch_pin    4  // ST_CP  12-Pin

// Ноги для энкодера
// #define pin_A          5
// #define pin_B          6
// #define EncoderButton  7

const uint8_t NUM_SEG[] = {
  1,
  2,
  4
};

const uint8_t SEG[] = {
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

int8_t EncMove = 0;

uint8_t EncFlag, EncLast, EncCurrent;

uint16_t SolderTemp = 325, HotTemp = 320;

void setup(){

  //2,3,4 порты настраиваем на вывод
  DDRD |= B00011100;

  //0,1,2,3 порты настраиваем на вывод
  DDRB |= B00001111;
  // Подтягиваем ножки к 1
  PORTB |= B00000011;

  // 5,6 на ввод
  DDRD &= ~B11100000;
  // Подтягиваем ножки к 1
  PORTD |= B11100000;

  currentTime = millis();
  loopTime = currentTime; 
}

void loop(){
while (1) {
  currentTime = millis();
    
  //-------------------Проверка каждые 3 мс
  if(currentTime >= (loopTime + 3)) {
    CheckEncoder();
    
    // Счетчик прошедшего времени      
    loopTime = currentTime;
  }
  //-------------------Конец проверка каждые 3 мс

  if (EncFlag) {
    SolderTemp  += EncMove;
    HotTemp     += EncMove;
    EncFlag = false;
  }

  PrintTemp( SolderTemp, HotTemp);
}
}
//End loop

void PrintTemp( uint16_t solder_temp, uint16_t hot_temp ) {
  
  uint16_t solder_s[] = {0,0,0}, hold_s[] = {0,0,0};
  uint16_t y = 0, i = 0;

  //преобразуем числа в отдельные цифры
    solder_s[2] = solder_temp/100;
    i = solder_temp % 100;
    solder_s[1] = i/10;
    solder_s[0] = i%10;

    hold_s[2]   = hot_temp/100;
    i = hot_temp % 100;
    hold_s[1] = i/10;
    hold_s[0] = i%10;
  
  for (i = 0; i < 3; i++) {
    //digitalWrite(latch_pin, LOW); 4 port
    //Но если надо выставить 0 так, чтобы не прибить остальные биты в 0,
    // нужен оператор "И", обозначающийся &. Так же к нему понадобится оператор "НЕ" - обозначается ~.
    PORTD &= ~B00010000;
    
    // Отправляем байт на последнею микросхему в цепочке
    // инвертирование битов исключающим или ^
    ShiftOut( SEG[ solder_s[i] ] ^ 0xFF );
    // Отправляем байт на вторую микросхему в цепочке
    ShiftOut( SEG[ hold_s[i] ] ^ 0xFF );
    // Зажигает по очереди сегменты на обоих индикаторах
    ShiftOut(NUM_SEG[i]);

    //digitalWrite(latch_pin, HIGH);
    PORTD |= B00010000;
 }
}

void ShiftOut( uint8_t value ) {
    for (uint8_t i = 0; i < 8; i++) {
      //digitalWrite(data_pin,(value & (0x80 >> i)));  //MSB
      !!(value & (0x80 >> i)) == HIGH ? PORTD |= B00000100 : PORTD &= ~B00000100;
      //digitalWrite(clk_pin, HIGH);
      PORTD |= B00001000;
      //digitalWrite(clk_pin, LOW);
      PORTD &= ~B00001000;
    }
}
// End ShiftOut

void CheckEncoder() {

  EncCurrent = PIND & B01100000;
  EncMove = 0;
  if( EncCurrent != EncLast ){
    if(EncLast == 96){
      if(EncCurrent == 32) EncMove = -1;
      if(EncCurrent == 64) EncMove = 1;
    }
    EncLast = EncCurrent;
    EncFlag = true;
  }//End Проверка состояния encoder
}
// End Check Enc

