// Steve Quinn 31/12/16
//
// Copyright 2016 Steve Quinn
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//
//
// Compiled using Arduino 1.6.9 
//
// Used to test IC2_Device2.ino code on an Arduino Uno R3.
// As 10K Pot is varied the intensity of the Led 1 will vary.
// Once a mid scale is reached on the Pot Led 2 will illuminate.
// State of button is also read back and displayed
//
// Parts list
// ----------
// 2 off Leds
// 2 off 470R resistors
// 1 off 10K  resistor
// 1 off SPST button
// 1 off 10K pot
// 2 off Arduino Uno R3
//
// Arduino interconnections
// ------------------------
// M     S
// Gnd - Gnd
// A4  - A4
// A5  - A5
//
// Slave Requires
// --------------
// 10K Pot common pin connected to A0
// Led with 470R resistor connected to Digital I/O 3 (Led 1) and ground
// Led with 470R resistor connected to Digital I/O 2 (Led 2) and ground
// Button with 10K pull up to VCC connected to Digital I/O 8
//
//

#include <Wire.h>

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define PWM_3_REG         0
#define PWM_5_REG         1
#define PWM_6_REG         2
#define PWM_9_REG         3
#define PWM_10_REG        4
#define PWM_11_REG        5
#define PWM_OFF           0
#define DDR               6
#define DR                7
#define ALL_IN            B00111111
#define ANALOGUE_IN_0_LOW 8
#define ANALOGUE_PORT     0  // Change this to read from Analogue ports 0...3
#define SLAVE_ADDR        8  // Change this to the Addr of your device

unsigned int iPWMValue_new = 0;
unsigned int iPWMValue_old = 1;

void setup()
{
  byte error;
  Wire.begin();
  Wire.beginTransmission(SLAVE_ADDR); // Configure Digital Ports 4, 7, 8, 12 and 13 as inputs, Port 2 as an output.
  Wire.write(DDR);
  Wire.write(B00111110);    // X, X, P13=I/P, P12=I/P, P8=I/P, P7=I/P, P4=I/P, P2=O/P
  error = Wire.endTransmission();
  
  Wire.beginTransmission(SLAVE_ADDR); // Set Port 2 output low.
  Wire.write(DR);
  Wire.write(B00111110);    // X, X, X, X, X, X, X, P2=O/P Low
  error = Wire.endTransmission();
  
  Serial.begin(115200);
  Serial.println(__FILENAME__);
  Serial.println("Serial Port Ready");
}


void loop()
{
  byte error;
  byte value = 0;  
  byte value1 = 0;  
  unsigned int value2 = 0;
  char tmpBuf[1000];  // Temp char array
  byte bBaseAddress = PWM_3_REG;


  // Read P13=I/P, P12=I/P, P8=I/P, P7=I/P, P4=I/P and display value
  Wire.beginTransmission(SLAVE_ADDR); // Point to Digital Data Register
  Wire.write(DR);
  error = Wire.endTransmission();
  Wire.requestFrom((int)SLAVE_ADDR, (int)1);    // request 1 byte from slave device #addr
  value = (char) Wire.read();
  sprintf(tmpBuf,"Binary : %c%c%c%c%c%c%c%c",(value&0x80?'1':'0'),(value&0x40?'1':'0'),(value&0x20?'1':'0'),(value&0x10?'1':'0'),(value&0x08?'1':'0'),(value&0x04?'1':'0'),(value&0x02?'1':'0'),(value&0x01?'1':'0'));  
  Serial.println(tmpBuf);


  // Read Analogue Port 
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(ANALOGUE_IN_0_LOW + (2 * ANALOGUE_PORT));  // Point to the analogue port of interest
  error = Wire.endTransmission();
  // Note. Wire.requestFrom((int)SLAVE_ADDR, (int)2); will not work due to the way the Wire Library is written
  // See http://www.gammon.com.au/forum/?id=10896. This means the requestEvent handler can only (successfully) do a single send.
  Wire.requestFrom((int)SLAVE_ADDR, (int)1);    // request 1 byte from slave device #addr
  value = (char) Wire.read();  // Read low byte
  Wire.requestFrom((int)SLAVE_ADDR, (int)1);    // request 1 byte from slave device #addr. Buffer pointer will auto increment
  value1 = (char) Wire.read(); // Read upper 2 bits
  value2 = ((unsigned int)((value1 << 8)& 0xff00)) | (value);
  sprintf(tmpBuf,"Port : %d, Decimal : %d, Binary : %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",ANALOGUE_PORT,value2,(value2&0x8000?'1':'0'),(value2&0x4000?'1':'0'),(value2&0x2000?'1':'0'),(value2&0x1000?'1':'0'),(value2&0x0800?'1':'0'),(value2&0x0400?'1':'0'),(value2&0x0200?'1':'0'),(value2&0x0100?'1':'0'),(value2&0x0080?'1':'0'),(value2&0x0040?'1':'0'),(value2&0x0020?'1':'0'),(value2&0x0010?'1':'0'),(value2&0x0008?'1':'0'),(value2&0x0004?'1':'0'),(value2&0x0002?'1':'0'),(value2&0x0001?'1':'0'));  
  //Serial.println(value2);
  Serial.println(tmpBuf);

  // Scale analogue value and write to PWM port
  iPWMValue_new = map(value2, 0, 1023, 0, 255);
  if (iPWMValue_old != iPWMValue_new) {
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(bBaseAddress);  // Point to PWM Register
    Serial.print("New PWM Value : ");
    Serial.println(iPWMValue_new);
    Wire.write((byte)iPWMValue_new);
    error = Wire.endTransmission();
    iPWMValue_old != iPWMValue_new;
  }

  // If PWM value > than midscale turn on Output Pin2
  Wire.beginTransmission(SLAVE_ADDR); // Point to Digital Data Register
  Wire.write(DR);
  if (iPWMValue_new > 128)
    Wire.write(B00000001);
  else
    Wire.write(B00000000);
  error = Wire.endTransmission();

  delay(1000);           // wait 1 second

}

