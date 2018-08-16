#define data_pin     2  // DS     14-Pin
#define clk_pin      3  // SH_CP  11-Pin
#define latch_pin   4  // ST_CP  12-Pin

const uint8_t NUM_SEG[] = {
  1,
  2,
  4
};

uint8_t SEG[] = {
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

void setup(){
  pinMode(data_pin,   OUTPUT);
  pinMode(clk_pin,    OUTPUT);
  pinMode(latch_pin, OUTPUT);    
}

void loop(){

  uint16_t SolderTemp = 325, HotTemp = 420;
  PrintTemp( SolderTemp, HotTemp);

}//End loop

void PrintTemp( uint16_t solder_temp, uint16_t hot_temp ) {
  
  uint16_t solder_s[] = {0,0,0}, hold_s[] = {0,2,4};
  uint16_t y = 0, i = 0;
  
  for (i = 0; i < 3; i++) {
    digitalWrite(latch_pin, LOW);
    
    // Отправляем байт на последнею микросхему в цепочке
    // инвертирование битов исключающим или ^
    ShuftOut( SEG[ solder_s[i] ] ^ 0xFF );
    // Отправляем байт на вторую микросхему в цепочке
    ShuftOut( SEG[ hold_s[i] ] ^ 0xFF );
    // Зажигает по очереди сегменты на обоих индикаторах
    ShuftOut(NUM_SEG[i]);
    digitalWrite(latch_pin, HIGH);
 }
}

void ShuftOut( uint8_t value ) {
    for (uint8_t i = 0; i < 8; i++) {
     digitalWrite(data_pin,(value & (0x80 >> i)));  //MSB
     digitalWrite(clk_pin, HIGH);
     digitalWrite(clk_pin, LOW);
    }
}
// End ShuftOut


