int ledPin = 9;    // Светодиод подключен к выходы 9
//int analogPin = 3; // потенциометр подключен к выходу 3
//int val = 0;       // переменная для хранения значения
 
void setup()
{
    pinMode(ledPin, OUTPUT);      // установка порта на выход
    pinMode(13, OUTPUT);
}
 
void loop(){

    tone(9, 100);


  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second
}

