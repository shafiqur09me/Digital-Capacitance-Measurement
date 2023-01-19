//////////////////////////////////////////////////////////////////////////
///AD7147 Driver by  Jacob Girgis

#include <Arduino.h>			// Use the arduino functions (digitalWrite/Read, analogRead, tone, Serial, .etc)
#include <Wire.h>					// used for I2C communication
#include "avr/pgmspace.h" // allows for better memory allocation
#include <stdlib.h>				// standard library
#include <inttypes.h> 		// recognize uint8_t, uint16_t and other types

//REGISTERS and ADDRESSES
#define AD7147_ADDR 0x2C  
#define PWR_CONTROL 0x000
#define STAGE_CAL_EN 0x001
#define AMB_COMP_CTRL0 0x002
#define AMB_COMP_CTRL1 0x003
#define AMB_COMP_CTRL2 0x004
#define STAGE_LOW_INT_ENABLE 0x005
#define STAGE_HIGH_INT_ENABLE 0x006
#define STAGE_COMPLETE_INT_ENABLE 0x007
#define STAGE_LOW_INT_STATUS 0x008
#define STAGE_HIGH_INT_STATUS 0x009
#define STAGE_COMPLETE_INT_STATUS 0x00A
#define STAGE0_CONNECTION60 0x080
#define STAGE0_CONNECTION127 0x081
#define STAGE1_CONNECTION60 0x088
#define STAGE1_CONNECTION127 0x089
#define STAGE2_CONNECTION60 0x090
#define STAGE2_CONNECTION127 0x091
#define CDC_RESULT_S0 0x00B
#define CDC_RESULT_S1 0x00C
#define CDC_RESULT_S2 0x00D
#define CDC_RESULT_S6 0x011





/*
The AD7147 operates in a way that has many registers. Each register has a 16 bit address
Each register contains a 16 bit number. This will be referred to as the register's value
Each bit in the register's value represents a setting.
For example PWR_CONTROL Register could be this:
15-------------0
1010101010101111
in page 43 of the data sheet it states that the first two bits (0,1)
determine the operating mode.
so:
XXXXXXXXXXXXXX00 = full power mode
XXXXXXXXXXXXXX01 = full shutdown mode         
XXXXXXXXXXXXXX10 = low power mode
XXXXXXXXXXXXXX11 = full shutdown mode

The AD7147 registers need to be changed per the settings the user wants to use.
So to write to the register we can only block write because we cannot change each individual bit.
we can only change the whole 16 bit number						
however we can do &ing and |ing (bitmasking) to change individual bits and keep others the same.
so you can acually write to individual bits by doing that.

On page 43 it shows the default value of the PWE_control register to be 0b0000000000000000
We would need to make a couple of functions to set it. 
Additionally there are many registers that handle calibration, ambient compensation control, and other things.
This is why it is critical to read the datasheet on what each one specifically does.


Process to write to register of AD7147
1. Write the 7 bit i2c address to the device followed by a read or write bit.
	->Wire library takes care of this for us
2. Write the 16 bit Register address to the device.
	-> therefore we write bits 15 through 8 first then write bits 7 through 0
3. Write the 16 bit data value to the register. This is the settings
	-> therefore we write bits 15 through 8 first then write bits 7 through 0
4. done.

Process to read a register from AD7147
1. Write the 7 bit i2c address to the device followed by a read or write bit.
	->Wire library takes care of this for us
2. Write the 16 bit Register address to the device.
	-> therefore we write bits 15 through 8 first then write bits 7 through 0
3. Send a Repeated start condition and send the i2c address again
	-> Wire.endTransmision(false) will send the repeated start for us after the write
4. Read from the 16 bit address, so we read in 8 bits (1 byte at a time) so we need to read twice (8+8=16)
	-> we read bits 15 through 8 first and then read bits 7 through 0
5. done.

*/

// this function reads from an register and returns the 16 bit register value
int16_t readByte(uint16_t address) {  
  //Variables for the halves of the 16 bit var and one for the combined
  uint16_t rx1Byte, rx2Byte, rxFinal16bitbyte;
  // Begin Transmission to the 7 bit i2c address
  Wire.beginTransmission(AD7147_ADDR);							
  //We write the upper bits of the 16 bit register value
  //We need to shift the bits two the right 8 places to make the upper bits conform to an 8 bit number
  //for example:
  //0b1111000011110000 >> 8 = 0bXXXXXXXX11110000 where X is an unknown bit
  //if we & the 0bXXXXXXXX11110000 with 0xFF(11111111) that cuts off the X's
  //so we get 0b11110000
  //readByte(0x000) = 0b1111000011110000
  Wire.write((address >> 8) & 0xFF); // writes the upper byte		
  // for example:
  //0b1111000011110000 & with 0xFF(11111111) = 0b11110000 
  Wire.write((address) & 0xFF);			
	// the way Wire library works is that it queues up the commands and endTransmission actually sends it
  // from the procedure it says we need to send a repeated start to read now.
  // endTransmission() will just kill the communication after sending it.
  // To send a repeated start condition we need endTransmission(false)
  int i2cStatus = Wire.endTransmission(false); // i2cStatus will return 1 if there is an error. 
  _delay_ms(1); // delay for 1 ms just in case it takes too long.
  // this is equivalent of writing an address like we did above and saying we want to read a 16 bit number (2 bytes)
  // the reason this is different is because Wire places the results in a buffer for us so we just have to call Wire.read()
  Wire.requestFrom(AD7147_ADDR, 2); 
  //Wire.availible will see how much data is in the buffer, only go into the loop if there is non zero amount of data
  if (Wire.available()>=1) { 
    rx1Byte = Wire.read();	 // get the upper byte (8 bits)
    rx2Byte = Wire.read();	//get the lower byte (8 bits)
    // this shifts the first byte over to the left 8 places
    // for example: if we expect 0b1111000011110000
    //read the upper byte = 0b11110000
    // 0b11110000 << 8 = 0b1111000000000000  this is now 16 bit number
    // 0b1111000000000000 | 0b11110000 
    // | (bitwise OR) its result is a 1 if one of the either bits is 1 and zero only when both bits are 0. 
    // so therefore the extra 0's become a 1 if the lower byte has a 1 in it
    // 0b1111000000000000 | 
    //         0b11110000 
    //=0b1111000011110000 = what we expected 0b1111000011110000
    // so using the | operator will copy the 1's over and the 0's over because when there's a 1, the 1 gets copied over.
    // when there is a 0 it only gets copied over when both bits are 0's
    rxFinal16bitbyte = (rx1Byte << 8) | rx2Byte; //combined of the two 8 bits to a 16 bit
    return rxFinal16bitbyte; // return final number 
  }
  else {
    // return an error code
    return -1;
  }
}

// this function will block write to a specific register
bool writeByte(uint16_t address, uint16_t data16bit) { // register address and data value to be sent as an argument
  Wire.beginTransmission(AD7147_ADDR); // Begin Transmission to the 7 bit i2c address						// so this is auomatically writing the 7 bit address?
  Wire.write((address >> 8) & 0xFF);	 // writes the upper byte		
  Wire.write((address) & 0xFF);				 // writes the lower byte		
  Wire.write((data16bit >> 8) & 0xFF);	// write the data value upper byte
  Wire.write((data16bit) & 0xFF);				// write the data value lower byte
  int i2cStatus = Wire.endTransmission();	// write all the commands in the queue and close the connection
  _delay_ms(1);	// let it do its thing, in case it takes a long time
  if (i2cStatus)		// return false if i2cStatus is 1, indicating an error
    return false;
  else
    return true;
}


void writePwr_Control(){
  	  writeByte(PWR_CONTROL,0b0000001011000000);		//write lower pow, 400ms CONV delay, decimation of 64ms

}

void writeStage_Cal_En(){
	
	writeByte(STAGE_CAL_EN,0b1110000000000000);
}

void writeStage0_Connection60(){
	
	writeByte(STAGE0_CONNECTION60,0b1111111111000010);
}

void writeStage0_Connection127(){
	
	writeByte(STAGE0_CONNECTION127,0b0001111111111111);
}

void writeStage1_Connection60(){
	
	writeByte(STAGE1_CONNECTION60,0b1111111111001000);
}

void writeStage1_Connection127(){
	
	writeByte(STAGE1_CONNECTION127,0b0001111111111111);
}

void writeStage2_Connection60(){
	
	writeByte(STAGE2_CONNECTION60,0b1111111111100000);
}

void writeStage2_Connection127(){
	
	writeByte(STAGE2_CONNECTION127,0b0010111111111111);
}



void writeStage_Low_Int_Enable(){
	
	writeByte(STAGE_LOW_INT_ENABLE,0b0000000000000000);
}

void writeStage_Hight_Int_Enable(){
	
	writeByte(STAGE_HIGH_INT_ENABLE,0b0000000000000000);
	
}

void writeStage_Complete_Int_Enable(){
	
	writeByte(STAGE_COMPLETE_INT_ENABLE,0b0000000000000100);
}


int main(){ // this function runs immeadiately upon upload
  //run once
  init(); // calls some arduino intializing to allow the arduino library to be used
  Serial.begin(9600);	// Start USART0 (TX0 and RX0)
  Wire.begin();				// Start the Wire library
  writePwr_Control();
  writeStage_Cal_En();
  writeStage0_Connection60();
  writeStage0_Connection127();
  writeStage1_Connection60();
  writeStage1_Connection127();
  writeStage2_Connection60();
  writeStage2_Connection127();
  writeStage_Low_Int_Enable();
  writeStage_Hight_Int_Enable();
  writeStage_Complete_Int_Enable();
  
  // print what is in the power control register this will be in decimal form, so convert it later
  Serial.print("PWR_CONTROL""\t");
  Serial.println(readByte(PWR_CONTROL)); 
  
  
  
  Serial.print("STAGE_CAL_EN""\t");
  Serial.println(readByte(STAGE_CAL_EN));
  
  
  
  Serial.print("Stage0_Connection[6:0]""\t");
  Serial.println(readByte(STAGE0_CONNECTION60));
  
  Serial.print("Stage0_Connection[12:7]""\t");
  Serial.println(readByte(STAGE0_CONNECTION127));
  
  Serial.print("Stage1_Connection[6:0]""\t");
  Serial.println(readByte(STAGE1_CONNECTION60));
  
  Serial.print("Stage1_Connection[12:7]""\t");
  Serial.println(readByte(STAGE1_CONNECTION127));
  
  Serial.print("Stage2_Connection[6:0]""\t");
  Serial.println(readByte(STAGE2_CONNECTION60));
  
  Serial.print("Stage2_Connection[12:7]""\t");
  Serial.println(readByte(STAGE2_CONNECTION127));
  
  Serial.print("STAGE_LOW_INT_ENABLE \t");
  Serial.println(readByte(STAGE_LOW_INT_ENABLE));
  
  Serial.print("STAGE_HIGH_INT_ENABLE \t");
  Serial.println(readByte(STAGE_HIGH_INT_ENABLE));
  
  Serial.print("STAGE_COMPLETE_INT_ENABLE \t");
  Serial.println(readByte(STAGE_COMPLETE_INT_ENABLE));
  
  
  
  
  while(1){

  Serial.print(readByte(CDC_RESULT_S0));
  Serial.print("\t");
  Serial.print(readByte(CDC_RESULT_S1));
  Serial.print("\t");
  Serial.print(readByte(CDC_RESULT_S2));
  Serial.print("\n");
  _delay_ms(1000);//loop forever

  }
  return(0);
}

