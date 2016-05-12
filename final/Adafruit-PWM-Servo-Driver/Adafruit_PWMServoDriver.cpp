/*************************************************** 
  This is a library for our Adafruit 16-channel PWM & Servo driver

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/815

  These displays use I2C to communicate, 2 pins are required to  
  interface. For Arduino UNOs, thats SCL -> Analog 5, SDA -> Analog 4

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
 
 /*****************************
  This program was ported from https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library.
  I also added some functions.
  Shundo Kishi
 *****************************/
 
#include "Adafruit_PWMServoDriver.h"

Adafruit_PWMServoDriver::Adafruit_PWMServoDriver(PinName sda, PinName scl, int addr) : i2c(sda, scl), _i2caddr(addr) {}

void Adafruit_PWMServoDriver::i2c_probe(void) 
{
    printf("Searching for I2C devices...\n");
    
    int count = 0;
    for (int address=4; address<256; address+=2) {
        if (!i2c.write(address, NULL, 0)) { // 0 returned is ok
            printf(" - I2C device found at address 0x%02X\r\n", address);
            count++;
        }
    }
    printf("%d devices found\r\n", count);
}

void Adafruit_PWMServoDriver::begin(void) {
    reset();
}

void Adafruit_PWMServoDriver::setI2Cfreq(int freq) {
    i2c.frequency(freq);
}

void Adafruit_PWMServoDriver::reset(void) {
    write8(PCA9685_MODE1, 0x0);
}

void Adafruit_PWMServoDriver::setPrescale(uint8_t prescale) {
    uint8_t oldmode = read8(PCA9685_MODE1);
    uint8_t newmode = (oldmode&0x7F) | 0x10; // sleep
    write8(PCA9685_MODE1, newmode); // go to sleep
    wait_ms(5);
    write8(PCA9685_PRESCALE, prescale); // set the prescaler
    write8(PCA9685_MODE1, oldmode);
    wait_ms(5);
    write8(PCA9685_MODE1, oldmode | 0xa1);
}

void Adafruit_PWMServoDriver::setPWMFreq(float freq) {
    //Serial.print("Attempting to set freq ");
    //Serial.println(freq);
    float prescaleval = 25000000;
    prescaleval /= 4096;
    prescaleval /= freq;
    //Serial.print("Estimated pre-scale: "); Serial.println(prescaleval);
    uint8_t prescale = floor(prescaleval  + 0.5) - 1;
    //Serial.print("Final pre-scale: "); Serial.println(prescale);  
    setPrescale(prescale);
}

void Adafruit_PWMServoDriver::setPWM(uint8_t num, uint16_t on, uint16_t off) {
    // hmm doesnt work, whyso?  (  Not in AI mode.  See line 54 above.  ( Works now!!  :D  )
  
    char cmd[5];
    cmd[0] = LED0_ON_L + 4 * num;
    cmd[1] = on;
    cmd[2] = on >> 8;
    cmd[3] = off;
    cmd[4] = off >> 8;
    i2c.write(_i2caddr, cmd, 5);
    
    /*write8(LED0_ON_L+4*num, on);
    write8(LED0_ON_H+4*num, on >> 8);
    write8(LED0_OFF_L+4*num, off);
    write8(LED0_OFF_H+4*num, off >> 8);*/
}

// Set pwm duty in us order
void Adafruit_PWMServoDriver::setDuty(uint8_t num, uint16_t duty) {
    float pulselength = 10000;   // 10,000 us per second
    duty = 4094 * duty / pulselength;
    setPWM(num, 0, duty);
}

uint8_t Adafruit_PWMServoDriver::read8(char addr) {
    i2c.write(_i2caddr, &addr, 1);
    char rtn;
    i2c.read(_i2caddr, &rtn, 1);
    return rtn;
}

void Adafruit_PWMServoDriver::write8(char addr, char d) {
    char cmd[2];
    cmd[0] = addr;
    cmd[1] = d;
    i2c.write(_i2caddr, cmd, 2);
}