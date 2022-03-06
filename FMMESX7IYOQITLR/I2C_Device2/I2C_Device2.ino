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
// Compiled using Arduino 1.6.9, Tested with Arduino Uno R3 
//
// Turns Arduino into an I2C Slave device
//
// Register allocations
// --------------------
// Addr 0,  PWM 3,  Arduino Pin 3
// Addr 1,  PWM 5,  Arduino Pin 5
// Addr 2,  PWM 6,  Arduino Pin 6
// Addr 3,  PWM 9,  Arduino Pin 9
// Addr 4,  PWM 10, Arduino Pin 10
// Addr 5,  PWM 11, Arduino Pin 11
// Addr 6,  DDR,    Data Direction Register
// Addr 7,  DR,     Data Register
// Addr 8,  Analogue 0 input register lo byte, Arduino Pin A0
// Addr 9,  Analogue 0 input register hi bits, Arduino Pin A0
// Addr 10, Analogue 1 input register lo byte, Arduino Pin A1
// Addr 11, Analogue 1 input register hi bits, Arduino Pin A1
// Addr 12, Analogue 2 input register lo byte, Arduino Pin A2
// Addr 13, Analogue 2 input register hi bits, Arduino Pin A2
// Addr 14, Analogue 3 input register lo byte, Arduino Pin A3
// Addr 15, Analogue 3 input register hi bits, Arduino Pin A3
//
//
// PWM
// ---
// Pulse Width Modulation
//
// Address 0x00 ... 0x05
//
// Takes the value 0x00 to 0xFF. Default value = 0x00, PWM_OFF
//
//
// DDR
// ---
// Data direction register
//
// Address 0x06
//
// Bit            B7    B6    B5    B4    B3    B2    B1    B0
//             +-----+-----+-----+-----+-----+-----+-----+-----+
// Arduino Pin | N/A | N/A | P13 | P12 | P8  | P7  | P4  | P2  |
//             +-----+-----+-----+-----+-----+-----+-----+-----+
// Default val    X     X     1     1     1     1     1     1
//
// For output set bit value = 0, for input set bit value = 1
//
// DR
// --
// Data register
//
// Address 0x07
//
// Bit            B7    B6    B5    B4    B3    B2    B1    B0
//             +-----+-----+-----+-----+-----+-----+-----+-----+
// Arduino Pin | N/A | N/A | P13 | P12 | P8  | P7  | P4  | P2  |
//             +-----+-----+-----+-----+-----+-----+-----+-----+
// Default val    X     X     0     0     0     0     0     0
//
// When corresponding bit in DDR is set to 0, setting the matching bit in the DR will set the asociated Arduino pin to a 1
// When corresponding bit in DDR is set to 1, reading the matching bit in the DR will reflect the status of the asociated Arduino pin
//
//
// Analogue
// --------
// Analogue inputs A0 ... A3. Paired as low 8 bits abd hi 2 bits, which make up the value 0 ... 1023.
//
// Address 0x08 ... 0x0F. Paired as A0 = 0x08 - 0x09, A1 = 0x0A - 0x0B, A2 = 0x0C - 0x0D, A3 = 0x0E - 0x0F
//
// Addr 0x09                                           Addr 0x08
// +-----+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+-----+
// | N/A | N/A | N/A | N/A | N/A | N/A | B9  | B8  |   | B7  | B6  | B5  | B4  | B3  | B2  | B1  | B0  |
// +-----+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+-----+
//
// Usage
// -----
// You must first set the register pointer to the register you wish to read or write. This is accomplished with the write
//
//  ie. Writing
//  Wire.beginTransmission(SLAVE_ADDR); 
//  // Point to DDR
//  Wire.write(DDR);
//  error = Wire.endTransmission();
//
//  followed by;
//  Wire.beginTransmission(SLAVE_ADDR); 
//  // X, X, P13=O/P, P12=O/P, P8=O/P, P7=O/P, P4=O/P, P2=O/P
//  Wire.write(B00000000);  
//  error = Wire.endTransmission();
//
//  Or they can be combined;
//  // X, X, P13=O/P, P12=O/P, P8=O/P, P7=O/P, P4=O/P, P2=O/P
//  Wire.beginTransmission(SLAVE_ADDR); 
//  Wire.write(DDR);
//  Wire.write(B00000000);    
//  error = Wire.endTransmission();
//
//  ie. Reading one byte
//  Wire.beginTransmission(SLAVE_ADDR); 
//  // Point to DR
//  Wire.write(DR);
//  error = Wire.endTransmission();
//  Wire.requestFrom((int)SLAVE_ADDR, (int)1);    // request 1 byte from slave device #addr
//  value = (char) Wire.read();
//
//  ie. Reading more than one byte
//  Wire.beginTransmission(SLAVE_ADDR); 
//  // Point to ANALOGUE_IN_0_LOW
//  Wire.write(ANALOGUE_IN_0_LOW);
//  error = Wire.endTransmission();
//  Wire.requestFrom((int)SLAVE_ADDR, (int)1);    // request 1 byte from slave device #addr
//  value1 = (char) Wire.read();                  // Read from ANALOGUE_IN_0_LOW register
//  Wire.requestFrom((int)SLAVE_ADDR, (int)1);    // request next byte from slave device #addr
//  value2 = (char) Wire.read();                  // Read from ANALOGUE_IN_0_HIGH register
//
//  ie. Limitation of Wire library. It is not possible to make sucessive reads as shown below;
//  See ; http://www.gammon.com.au/forum/?id=10896
//  Wire.beginTransmission(SLAVE_ADDR); 
//  // Point to ANALOGUE_IN_0_LOW
//  Wire.write(ANALOGUE_IN_0_LOW);
//  error = Wire.endTransmission();
//  Wire.requestFrom((int)SLAVE_ADDR, (int)2);    // request 2 byte from slave device #addr
//  value1 = (char) Wire.read();                  // Read from ANALOGUE_IN_0_LOW register
//  value2 = (char) Wire.read();                  // Read from ANALOGUE_IN_0_HIGH register
//
// NOTE : Successive reads or writes auto increment the register pointer. Pointer wraps round to position 0 when incrementing of the end of the register array.
//

#include <Wire.h>


//#define DEBUG_GENERAL      // Define this to get debug information out of the serial port
#define SLAVE_ADDR         8

#define PWM_OFF            0
#define PWM_3              3
#define PWM_5              5
#define PWM_6              6
#define PWM_9              9
#define PWM_10             10
#define PWM_11             11

#define ALL_IN             B00111111
#define DIGITAL_IO_2       2
#define DIGITAL_IO_4       4
#define DIGITAL_IO_7       7
#define DIGITAL_IO_8       8
#define DIGITAL_IO_12      12
#define DIGITAL_IO_13      13

#define ANALOGUE_IN_0      0
#define ANALOGUE_IN_1      1
#define ANALOGUE_IN_2      2
#define ANALOGUE_IN_3      3

#define PWM_3_REG          0
#define PWM_5_REG          1
#define PWM_6_REG          2
#define PWM_9_REG          3
#define PWM_10_REG         4
#define PWM_11_REG         5
#define DDR                6
#define DR                 7
#define ANALOGUE_IN_0_LOW  8
#define ANALOGUE_IN_0_HIGH 9
#define ANALOGUE_IN_1_LOW  10
#define ANALOGUE_IN_1_HIGH 11
#define ANALOGUE_IN_2_LOW  12
#define ANALOGUE_IN_2_HIGH 13
#define ANALOGUE_IN_3_LOW  14
#define ANALOGUE_IN_3_HIGH 15
#define MAX_REGISTERS      16

volatile byte bRegisterPointer = 0;
volatile byte bRegisterArray[MAX_REGISTERS];

#ifdef DEBUG_GENERAL
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif


void setup() {
  #ifdef DEBUG_GENERAL
  Serial.begin(115200);           // start serial for output
  Serial.println(__FILENAME__);
  Serial.println("Serial port Active");
  #endif
  Wire.begin(SLAVE_ADDR);         // join i2c bus with address SLAVE_ADDR
  Wire.onReceive(receiveEvent);   // register event
  Wire.onRequest(requestEvent);   // register event

  pinMode(PWM_3, OUTPUT);
  analogWrite(PWM_3, PWM_OFF);  
  pinMode(PWM_5, OUTPUT);
  analogWrite(PWM_5, PWM_OFF);  
  pinMode(PWM_6, OUTPUT);
  analogWrite(PWM_6, PWM_OFF);  
  pinMode(PWM_9, OUTPUT);
  analogWrite(PWM_9, PWM_OFF);  
  pinMode(PWM_10, OUTPUT);
  analogWrite(PWM_10, PWM_OFF);  
  pinMode(PWM_11, OUTPUT);
  analogWrite(PWM_11, PWM_OFF);  
  
  pinMode(DIGITAL_IO_2,  INPUT_PULLUP);
  pinMode(DIGITAL_IO_4,  INPUT_PULLUP);
  pinMode(DIGITAL_IO_7,  INPUT_PULLUP);
  pinMode(DIGITAL_IO_8,  INPUT_PULLUP);
  pinMode(DIGITAL_IO_12, INPUT_PULLUP);
  pinMode(DIGITAL_IO_13, INPUT_PULLUP);
  
  bRegisterArray[PWM_3_REG]          = PWM_OFF;
  bRegisterArray[PWM_5_REG]          = PWM_OFF;
  bRegisterArray[PWM_6_REG]          = PWM_OFF;
  bRegisterArray[PWM_9_REG]          = PWM_OFF;
  bRegisterArray[PWM_10_REG]         = PWM_OFF;
  bRegisterArray[PWM_11_REG]         = PWM_OFF;
  bRegisterArray[DDR]                = ALL_IN;  
  bRegisterArray[DR]                 = 0;
  bRegisterArray[ANALOGUE_IN_0_LOW]  = 0;
  bRegisterArray[ANALOGUE_IN_0_HIGH] = 0;
  bRegisterArray[ANALOGUE_IN_1_LOW]  = 0;
  bRegisterArray[ANALOGUE_IN_1_HIGH] = 0;
  bRegisterArray[ANALOGUE_IN_2_LOW]  = 0;
  bRegisterArray[ANALOGUE_IN_2_HIGH] = 0;
  bRegisterArray[ANALOGUE_IN_3_LOW]  = 0; 
  bRegisterArray[ANALOGUE_IN_3_HIGH] = 0;
}


void loop() {
  int iTmpAnalogueRead = 0;
  // Do PWM
  analogWrite(PWM_3,  bRegisterArray[PWM_3_REG]);
  analogWrite(PWM_5,  bRegisterArray[PWM_5_REG]);
  analogWrite(PWM_6,  bRegisterArray[PWM_6_REG]);
  analogWrite(PWM_9,  bRegisterArray[PWM_9_REG]);
  analogWrite(PWM_10, bRegisterArray[PWM_10_REG]);
  analogWrite(PWM_11, bRegisterArray[PWM_11_REG]);
  
  // Do digital
  if (bRegisterArray[DDR] & B00000001)
    if (digitalRead(DIGITAL_IO_2) == HIGH)
      bitSet(bRegisterArray[DR],0);
    else  
      bitClear(bRegisterArray[DR],0);
  else 
    if (bRegisterArray[DR] & B00000001)
      digitalWrite(DIGITAL_IO_2,HIGH);
    else  
      digitalWrite(DIGITAL_IO_2,LOW);

  if (bRegisterArray[DDR] & B00000010)
    if (digitalRead(DIGITAL_IO_4) == HIGH)
      bitSet(bRegisterArray[DR],1);
    else  
      bitClear(bRegisterArray[DR],1);
  else 
    if (bRegisterArray[DR] & B00000010)
      digitalWrite(DIGITAL_IO_4,HIGH);
    else  
      digitalWrite(DIGITAL_IO_4,LOW);

  if (bRegisterArray[DDR] & B00000100)
    if (digitalRead(DIGITAL_IO_7) == HIGH)
      bitSet(bRegisterArray[DR],2);
    else  
      bitClear(bRegisterArray[DR],2);
  else 
    if (bRegisterArray[DR] & B00000100)
      digitalWrite(DIGITAL_IO_7,HIGH);
    else  
      digitalWrite(DIGITAL_IO_7,LOW);
  
  if (bRegisterArray[DDR] & B00001000)
    if (digitalRead(DIGITAL_IO_8) == HIGH)
      bitSet(bRegisterArray[DR],3);
    else  
      bitClear(bRegisterArray[DR],3);
  else 
    if (bRegisterArray[DR] & B00001000)
      digitalWrite(DIGITAL_IO_8,HIGH);
    else  
      digitalWrite(DIGITAL_IO_8,LOW);
  
  if (bRegisterArray[DDR] & B00010000)
    if (digitalRead(DIGITAL_IO_12) == HIGH)
      bitSet(bRegisterArray[DR],4);
    else  
      bitClear(bRegisterArray[DR],4);
  else 
    if (bRegisterArray[DR] & B00010000)
      digitalWrite(DIGITAL_IO_12,HIGH);
    else  
      digitalWrite(DIGITAL_IO_12,LOW);

  if (bRegisterArray[DDR] & B00100000)
    if (digitalRead(DIGITAL_IO_13) == HIGH)
      bitSet(bRegisterArray[DR],5);
    else  
      bitClear(bRegisterArray[DR],5);
  else 
    if (bRegisterArray[DR] & B00100000)
      digitalWrite(DIGITAL_IO_13,HIGH);
    else  
      digitalWrite(DIGITAL_IO_13,LOW);
  
  // Do Analogue I/P
  iTmpAnalogueRead = analogRead(ANALOGUE_IN_0);
  bRegisterArray[ANALOGUE_IN_0_LOW]  = ((byte)(iTmpAnalogueRead & B11111111));
  bRegisterArray[ANALOGUE_IN_0_HIGH] = ((byte)((iTmpAnalogueRead >> 8) & B00000011));
  iTmpAnalogueRead = analogRead(ANALOGUE_IN_1);
  bRegisterArray[ANALOGUE_IN_1_LOW]  = ((byte)(iTmpAnalogueRead & B11111111));
  bRegisterArray[ANALOGUE_IN_1_HIGH] = ((byte)((iTmpAnalogueRead >> 8) & B00000011));
  iTmpAnalogueRead = analogRead(ANALOGUE_IN_2);
  bRegisterArray[ANALOGUE_IN_2_LOW]  = ((byte)(iTmpAnalogueRead & B11111111));
  bRegisterArray[ANALOGUE_IN_2_HIGH] = ((byte)((iTmpAnalogueRead >> 8) & B00000011));
  iTmpAnalogueRead = analogRead(ANALOGUE_IN_3);
  bRegisterArray[ANALOGUE_IN_3_LOW]  = ((byte)(iTmpAnalogueRead & B11111111));
  bRegisterArray[ANALOGUE_IN_3_HIGH] = ((byte)((iTmpAnalogueRead >> 8) & B00000011));
}



void requestEvent(void)
{
  #ifdef DEBUG_GENERAL
  {
    byte value = 0;  
    char tmpBuf[50+1];  // Used in callback function, temp char array
    Serial.println();
    Serial.println("requestEvent");
    Serial.print("bRegisterPointer : ");
    Serial.println(bRegisterPointer);
    value = bRegisterArray[bRegisterPointer];
    sprintf(tmpBuf,"%c%c%c%c%c%c%c%c",(value&0x80?'1':'0'),(value&0x40?'1':'0'),(value&0x20?'1':'0'),(value&0x10?'1':'0'),(value&0x08?'1':'0'),(value&0x04?'1':'0'),(value&0x02?'1':'0'),(value&0x01?'1':'0'));  
    Serial.println(tmpBuf);
  }
  #endif
  Wire.write (bRegisterArray[bRegisterPointer++]);
  if (bRegisterPointer> (MAX_REGISTERS-1))
    bRegisterPointer = 0;
  #ifdef DEBUG_GENERAL
  {
    Serial.print("bRegisterPointer : ");
    Serial.println(bRegisterPointer);
  }
  #endif
}



// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  #ifdef DEBUG_GENERAL
  {
    Serial.println();
    Serial.println("receiveEvent");
    Serial.print("bRegisterPointer : ");
    Serial.println(bRegisterPointer);
    Serial.print("howMany : ");
    Serial.println(howMany);
  }
  #endif

  if (howMany > 0) {
    bRegisterPointer = Wire.read();
    #ifdef DEBUG_GENERAL
    {
      Serial.print("bRegisterPointer : ");
      Serial.println(bRegisterPointer);
    }
    #endif
    
    if (bRegisterPointer> (MAX_REGISTERS-1))
      bRegisterPointer = MAX_REGISTERS-1;
      
    for (int x = 0; x < howMany-1; x++)
    {
      //bRegisterArray[bRegisterPointer++] = Wire.read();
      bRegisterArray[bRegisterPointer] = Wire.read();
      #ifdef DEBUG_GENERAL
      {
        byte value = 0;  
        char tmpBuf[50+1];  // Used in callback function, temp char array
        value = bRegisterArray[bRegisterPointer];
        sprintf(tmpBuf,"%c%c%c%c%c%c%c%c",(value&0x80?'1':'0'),(value&0x40?'1':'0'),(value&0x20?'1':'0'),(value&0x10?'1':'0'),(value&0x08?'1':'0'),(value&0x04?'1':'0'),(value&0x02?'1':'0'),(value&0x01?'1':'0'));  
        Serial.print(value);
        Serial.print(", ");
        Serial.println(tmpBuf);
      }
      #endif
      
      bRegisterPointer++;
      if (bRegisterPointer> (MAX_REGISTERS-1))
        bRegisterPointer = 0;
    }
  }
  #ifdef DEBUG_GENERAL
  {
    Serial.print("bRegisterPointer : ");
    Serial.println(bRegisterPointer);
  }
  #endif
}


