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
unsigned long lcd_period = 250 * 1e3; // us
unsigned long last_lcd_update = 0;

// Analog input measuring the voltage
const int DVM_DC = A5, DVM_full = A4, PM = A3;
int voltage_dc, voltage;

// PWM setup : signal generator
const int pwm_pin = 9;
unsigned long half_period = 20000; //us
const int PWM = 255;
unsigned long t1, t0 = 0;
int val = 0;

// serial commnication
const unsigned long serial_period =  1200; // us
unsigned long last_serial_time = 0;

// sine wave LUT setup: total wave steps
const int lut_size = 32;
// Store only the first 8 amplitude offsets from the center (0 to 127)
const byte quarter_amp[8] = {0, 25, 49, 71, 90, 106, 117, 125};
int lut_index = 0;
unsigned long step_period = 40000; // Microseconds per LUT step
int t3, t2 = 0;

void setup() {
  // set up the LCD's number of columns and rows
  lcd.begin(16,2);
  // print an initial message
  lcd.print("Ciao mbare!");

  // set Analog Pin 5, 4, 3 as input
  pinMode(DVM_DC, INPUT);
  pinMode(DVM_full, INPUT);
  pinMode(PM, INPUT);
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
  voltage_dc = analogRead(DVM_DC);
  voltage = analogRead(DVM_full);
  
  // check potentiometer for signal's frequency
  PWM_freq();
  // generate ARDUINO output (DIGITAL PIN 9)
  Sine_PWM();
  // send ARDUINO input signals (ANALOG PIN A5 and A4) 
  serial_communication(voltage_dc, voltage);
  // show the DC voltage through the LCD screen
  show_LCD_Voltage(voltage_dc);

}

void PWM_freq(){
  half_period = analogRead(PM)* 1e6/1023 + 5000;
}
void PWM_gen(){
	  t1 = micros();
  if ((t1 - t0) >= half_period) {
    t0 = micros();
    if (val == 0)
      val = PWM;
    else
      val = 0;
    analogWrite(pwm_pin, val);
  }
}

void show_LCD_Voltage(int v){
	// LCD screen update is periodic for performance
  if (micros() - last_lcd_update >= lcd_period) {
    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    // print the number of seconds since reset:
    lcd.setCursor(0, 1);
    lcd.print(float(v) * 5/1023, 4);
    lcd.print(" V");
    last_lcd_update = micros();
    }
}

void serial_communication(int v_dc, int v){
  if (micros() - last_serial_time >= serial_period) {
    last_serial_time = micros();
    Serial.print(v_dc);
    Serial.print(' ');
    Serial.println(v);
  }
}

void Sine_PWM() {
  t3 = micros();
  if ((t3 - t2) >= half_period) {
    t2 = micros();
    
    int quadrant = lut_index / 8;  // Determines phase: 0, 1, 2, or 3
    int step = lut_index % 8;      // Position within the quadrant: 0 to 7
    int pwm_val = 128;             // Center voltage baseline (~2.5V)
    
    // Reconstruct the full wave based on the current quadrant
    if (quadrant == 0) {
      pwm_val += quarter_amp[step];           // 0° to 90°: Rising positive
    } else if (quadrant == 1) {
      pwm_val += quarter_amp[7 - step];       // 90° to 180°: Falling positive
    } else if (quadrant == 2) {
      pwm_val -= quarter_amp[step];           // 180° to 270°: Falling negative
    } else if (quadrant == 3) {
      pwm_val -= quarter_amp[7 - step];       // 270° to 360°: Rising negative
    }
    
    analogWrite(pwm_pin, pwm_val);
    lut_index = (lut_index + 1) % lut_size;
  }
}


