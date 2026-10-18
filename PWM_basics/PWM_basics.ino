/*
USING THE LCD 

VOLTMETER setup
	SCHEMATIC
 	* LCD RS pin to digital pin 12
 	* LCD Enable pin to digital pin 11
 	* LCD D4 pin to digital pin 5
 	* LCD D5 pin to digital pin 4
 	* LCD D6 pin to digital pin 3
 	* LCD D7 pin to digital pin 2
 	* LCD R/W pin to ground
 	* LCD VSS pin to ground
 	* LCD VCC pin to 5V
 	* 10K resistor:
 	* ends to +5V and ground at A and K respectively
 	* wiper to LCD VO pin (pin 3)

PWM setup
	* Output pin is Pin 9

*/

// need the library for LCD monitor
#include <LiquidCrystal.h>

// initialize the library: associate LCD interface with ARDUINO pin number
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); 
// Manual LCD crystal update
const unsigned long lcd_period = 250 * 1e3; // us
unsigned long last_lcd_update = 0;

// Analog input measuring the voltage
const int DVM_DC = A5, DVM_full = A4;
float voltage_dc, voltage;

// PWM setup : signal generator
const int pwm_pin = 9;
const unsigned long period = 3000; //us
const unsigned long serial_period =  1200; // us
unsigned long last_serial_time = 0;
const int PWM = 255;
unsigned long t1, t0 = 0;
int val = 0;


void setup() {
  // set up the LCD's number of columns and rows
  lcd.begin(16,2);
  // print an initial message
  lcd.print("Ciao mbare!");

  // set Analog Pin 5 and 4 as input
  pinMode(DVM_DC, INPUT);
  pinMode(DVM_full, INPUT);
  delay(1000);
  lcd.clear();
  lcd.print("Voltmeter (DC) ");

  //Start the serial communication
  Serial.begin(115200);
  // additional delay ensuring Python's connection
  delay (1000);
  // Share the serial communication frequency through
  // the serial port
  Serial.print("CONNECTION: ");
  Serial.println(serial_period);
  // PWM setup
  pinMode(pwm_pin, OUTPUT);
  analogWrite(pwm_pin, val);

  
}

void loop() {
  voltage_dc = analogRead(DVM_DC) ;
  voltage = analogRead(DVM_full) ;
  
  // LCD screen update is periodic for performance
  if (micros() - last_lcd_update >= lcd_period) {
    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    // print the number of seconds since reset:
    lcd.setCursor(0, 1);
    lcd.print(voltage_dc * 5/1023, 4);
    lcd.print(" V");
    last_lcd_update = micros();
    }

  PWM_gen();

if (micros() - last_serial_time >= serial_period) {
    last_serial_time = micros();
    Serial.print(voltage_dc);
    Serial.print(' ');
    Serial.println(voltage);
  }

}

void PWM_gen(){
	  t1 = micros();
  if ((t1 - t0) >= period) {
    t0 = micros();
    if (val == 0)
      val = PWM;
    else
      val = 0;
    analogWrite(pwm_pin, val);
  }
}
