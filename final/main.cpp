#include "mbed.h"
#include "PololuLedStrip.h"
#include "Adafruit_PWMServoDriver.h"
//#include "NeoStrip.h"

#define DEFAULT_PULSE 990
#define MIN_PULSE 990
#define MAX_PULSE 2358

#define NUM_UNITS 32
 
Serial pc(USBTX, USBRX);
AnalogIn bass(p18);
AnalogIn mids(p19);
AnalogIn treble(p20);
Adafruit_PWMServoDriver pwm1(p9, p10);
Adafruit_PWMServoDriver pwm2(p9, p10, 0x82);
//NeoStrip myleds(p5, NUM_UNITS);
PololuLedStrip ledStrip(p5);

InterruptIn b0(p21);
InterruptIn b1(p22);
InterruptIn b2(p23);
InterruptIn b3(p24);
InterruptIn b4(p25);
InterruptIn b5(p26);

rgb_color colors[NUM_UNITS];

int snake_order[12] = {0,1,2,3,7,11,10,9,8,4,5,6};
//int snake_order[60] = {0,1,2,3,7,11,15,14,13,12,8,4,5,6,10,9,5,6,10,14,13,12,8,4,0,1,2,3,7,11,15,14,13,12,8,4,0,1,2,3,7,11,10,9,5,6,10,9,5,1,2,3,7,11,15,14,13,12,8,4};
int snake_index = 0;

float servoPositions[NUM_UNITS];

int inner_ring[4] = {5,6,9,10};
int outer_ring[8] = {0,1,2,3,4,7,8,11};

int row1[4] = {0,1,2,3};
int row2[4] = {4,5,6,7};
int row3[4] = {8,9,10,11};
//int row4[4] = {3,7,11,15};

int col1[3] = {row1[0],row2[0],row3[0]};
int col2[3] = {row1[1],row2[1],row3[1]};
int col3[3] = {row1[2],row2[2],row3[2]};
int col4[3] = {row1[3],row2[3],row3[3]};

//int colors[4];
Timer timer;

void setServoPulse(uint8_t n, float pulse);

//void resetDisplay() {
//    for (int i = 0; i < NUM_UNITS; i++) {
//        setServoPulse(i, 0.0);
//    }
//    myleds.clear();
//}

void resetDisplay() {
    for (int i = 0; i < NUM_UNITS; i++) {
        colors[i] = (rgb_color) {0,0,0};
        setServoPulse(i, 0.0);
    }
    ledStrip.write(colors, NUM_UNITS);
}


// Converts a color from the HSV representation to RGB.
rgb_color hsvToRgb(float h, float s, float v)
{
    int i = floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    float r = 0, g = 0, b = 0;
    switch(i % 6){
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return (rgb_color){r * 255, g * 255, b * 255};
}

void breathing(int w, int h, int speed) {
    int in, out;
    for (in = 0; in < 628; in += 1) {
        for (int i = 0; i < NUM_UNITS; i++){
            colors[i] = (rgb_color) {(0.5 * cos(in / 100.0 - 1.571 * (i % w)) + 0.5) * 255, 0, 0};
        }
        if ((in % 10) == 0) {
            for (int i = 0; i < NUM_UNITS; i++){
                setServoPulse(i, 0.5 * cos(in / 100.0 - 1.571 * (i % w)) + 0.5);
            }
        }
        ledStrip.write(colors, NUM_UNITS);
        wait_us(1000);
    }   
}

void heartbeat() {
        uint32_t time = timer.read_ms();
        
        for (int i = 0; i < 4; i++) {
            uint8_t phase = (time >> 4) - (0 << 2);
            colors[inner_ring[i]] = hsvToRgb(phase / 256.0, 1.0, 1.0);
        }
        for (int i = 0; i < 8; i++) {
            colors[outer_ring[i]] = (rgb_color) {0,0,0};
        }
        ledStrip.write(colors, NUM_UNITS);
        wait_ms(500);
        for (int i = 0; i < 4; i++) {
            colors[inner_ring[i]] = (rgb_color) {0,0,0};
        }
        for (int i = 0; i < 8; i++) {
            uint8_t phase = (time >> 4) - (0 << 2);
            colors[outer_ring[i]] = hsvToRgb(phase / 256.0, 1.0, 1.0);
        }
        ledStrip.write(colors, NUM_UNITS);
        wait_ms(1000);
}

void snake(int w, int h) {
    
    for (int j = 0; j < h; j++) {
        if (j % 2 == 0){
            
            for (int i = w * j; i < w * (j+1); i++) {
                resetDisplay();
                colors[i] = (rgb_color) {255, 255, 255};
                ledStrip.write(colors, NUM_UNITS);
                setServoPulse(i, 0.5);
                wait_ms(500);
            }
        } else {
            for (int i = w*(j+1) - 1; i >= w * j; i--) {
                resetDisplay();
                colors[i] = (rgb_color) {255, 255, 255};
                ledStrip.write(colors, NUM_UNITS);
                setServoPulse(i, 0.5);
                wait_ms(500);
            }
        }
    }   
}    

void spiral() {
        for (int i = 0; i < NUM_UNITS; i++) {
            setServoPulse(i, 0.0);
        }
        resetDisplay();
        
        uint32_t time = timer.read_ms();       
        //for(int i = 0; i < 2; i++)
//        {
//            uint8_t phase = (time >> 4) - (i << 2);
//            int led = (snake_index + i) % 60;
//            myleds.setPixel(snake_order[led], hsvToRgb(phase / 256.0, 1.0, 1.0));
//        }
        colors[snake_order[snake_index]] = (rgb_color) {255, 255, 255};
        setServoPulse(snake_order[snake_index], 0.5);
        snake_index++;
        if (snake_index == 12) snake_index = 0;
        ledStrip.write(colors, NUM_UNITS);
        wait_ms(500);
}

int onOff (int i, float lvl) {
        if (( 240 - 60 * ((i % 4) + 1)) < (lvl * 255)) return 1;
        return 0; 
}

void audioResponse() {

        float basslvl = bass.read();
        float midslvl = mids.read();
        float treblelvl = treble.read();
        
        for (int i = 0; i < NUM_UNITS; i++) {
            if ((i % 8) > 5) {
                //colors[i] = (rgb_color) {0, 0, onOff(i / 8, treblelvl) * 255};
//                setServoPulse(i, onOff(i / 8, treblelvl));
                colors[i] = (rgb_color) {0, 0, treblelvl * 255};
                setServoPulse(i, treblelvl);
//                myleds.setPixel(i, 0, basslvl * 255, 0);
            } else if ((i % 8) < 2) {
                //colors[i] = (rgb_color) {onOff(i / 8, midslvl)*255, 0, 0};
//                setServoPulse(i, onOff(i / 8, midslvl));
                colors[i] = (rgb_color) {midslvl * 255, 0, 0};
                setServoPulse(i, midslvl);
            } else {
               // colors[i] = (rgb_color) {0, onOff(i / 8, basslvl)*255, 0};
//                setServoPulse(i, onOff(i / 8, basslvl));
                colors[i] = (rgb_color) {0, basslvl * 255, 0};
                setServoPulse(i, basslvl);
            }
        }
        ledStrip.write(colors, NUM_UNITS);
}

void rainbow() {
   // Update the colors array.
        uint32_t time = timer.read_ms();       
        for(int i = 0; i < NUM_UNITS; i++)
        {
            uint8_t phase = (time >> 4) - (i << 2);
            colors[i] = hsvToRgb(phase / 256.0, 1.0, 1.0);
        }
        ledStrip.write(colors, NUM_UNITS);
}

void setServoPulse(uint8_t n, float pulse) {
    pulse = pulse > 1.0 ? 1.0 : pulse;
    pulse = pulse < 0.0 ? 0.0 : pulse;
    float pulselength = 20000;   // 10,000 us per second
    pulse = MIN_PULSE + (MAX_PULSE - MIN_PULSE) * pulse;
    pulse = 4096 * (pulse) / pulselength;
    
    if (n <= 15) pwm1.setPWM(15 - n, 0, pulse);
    else pwm2.setPWM(15 - (n - 16), 0, pulse);
}

void initServoDriver() {
    pwm1.begin();
//    pwm.setPWMFreq(45);  // This dosen't work well because of uncertain clock speed. Use setPrescale().
    pwm1.setPrescale(121);    // This value is decided for 20ms interval.
    pwm1.setI2Cfreq(400000); // 400kHz
    
    pwm2.begin();
//    pwm.setPWMFreq(45);  // This dosen't work well because of uncertain clock speed. Use setPrescale().
    pwm2.setPrescale(121);    // This value is decided for 20ms interval.
    pwm2.setI2Cfreq(400000); // 400kHz
}

void adjustServo(int i) {
    bool fin = false;
    while(!fin){
        char c = pc.getc();
//        printf("%c\r\n", c);
        if (c == '5') {
            servoPositions[i] -= 0.05;
            if (servoPositions[i] < 0) servoPositions[i] = 0.0;
            setServoPulse(i, servoPositions[i]);
        } else if (c == '8') {
            servoPositions[i] += 0.05;
            if (servoPositions[i] > 1.0) servoPositions[i] = 1.0;
            setServoPulse(i, servoPositions[i]);
        } else fin = !fin;
    }
}

 
//void locationResponse() {
//    
//    for (int i = 0; i < 16; i++) {
//           myleds.setPixel(i, (uint8_t) 100,(uint8_t) 0,(uint8_t) 0);
//           
//    }
////    myleds.write();
//    char c = pc.getc();
////    printf("%c\r\n", c);
////    printf("separate\r\n");
//    switch (c) {
//        case '1':
//        myleds.setPixel(row1[0], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(0);
//        break;
//        case '2':
//        myleds.setPixel(row1[1], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(1);
//        break;
//        case '3':
//        myleds.setPixel(row1[2], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(2);
//        break;
//        case '4':
//        myleds.setPixel(row1[3], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(3);
//        break;
//        case 'q':
//        myleds.setPixel(row2[0], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(4);
//        break;
//        case 'w':
//        myleds.setPixel(row2[1], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(5);
//        break;
//        case 'e':
//        myleds.setPixel(row2[2], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(6);
//        break;
//        case 'r':
//        myleds.setPixel(row2[3], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(7);
//        break;
//        case 'a':
//        myleds.setPixel(row3[0], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(8);
//        break;
//        case 's':
//        myleds.setPixel(row3[1], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(9);
//        break;
//        case 'd':
//        myleds.setPixel(row3[2], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(10);
//        break;
//        case 'f':
//        myleds.setPixel(row3[3], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
//        myleds.write();
//        adjustServo(11);
//        break;
////        case 'z':
////        myleds.setPixel(row4[0], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
////        break;
////        case 'x':
////        myleds.setPixel(row4[1], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
////        break;
////        case 'c':
////        myleds.setPixel(row4[2], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
////        break;
////        case 'v':
////        myleds.setPixel(row4[3], (uint8_t) 0,(uint8_t) 255,(uint8_t) 0);
////        break;
//    }
////    printf("%c\r\n", c);
//    myleds.write();
//}

int volatile mode = 0;
void mode0() { mode = 0; pc.putc('0'); }
void mode1() { mode = 1; pc.putc('1'); }
void mode2() { mode = 2; pc.putc('2'); }
void mode3() { mode = 3; pc.putc('3'); }
void mode4() { mode = 4; pc.putc('4'); }
void mode5() { mode = 4; pc.putc('5'); }

int main() {
    pc.baud(115200);
    
    initServoDriver();
    timer.start();
    
    int n = 16;
    float pulse = 0.5;
    float max_pulse = 1;
    float min_pulse = 0;
    int speed = 100;
    
    b0.mode(PullNone);
    b1.mode(PullNone);
    b2.mode(PullNone);
    b3.mode(PullNone);
    b4.mode(PullNone);
    b5.mode(PullNone);
    
    wait(.01);
    
    b0.fall(&mode0);
    b1.fall(&mode1);
    b2.fall(&mode2);
    b3.fall(&mode3);
    b4.fall(&mode4);
    b5.fall(&mode5);
    
    // Start sampling pb input using interrupts
    //b0.setSampleFrequency();
//    b1.setSampleFrequency();
//    b2.setSampleFrequency();
//    b3.setSampleFrequency();
    
//    myleds.setBrightness(1.0);
//    resetDisplay();
    
    //while (1) {
////        snake(8,4);
////        breathing(4,3, speed);
//        audioResponse();
////        rainbow();
//     //   for (int i = 0; i < NUM_UNITS; i++) {
////            myleds.setPixel(i, 0xFFFFFF);
////        }
////        myleds.write();
////        wait_ms(10);
////        locationResponse();
//    }
    
//    setServoPulse(2, 0.5);
    
    while(1) {
//        setServoPulse(0, bass.read());
//        setServoPulse(1, mids.read());
//        setServoPulse(2, treble.read());
       // printf("%d\n", mode);
        if (mode == 2) {
            rainbow();
            continue;    
        } else if (mode == 3) {
            snake(8,4);
            continue;
        } else if (mode == 4) {
            audioResponse();
            continue;
        } else if (mode == 5) {
            breathing(8,4, speed);
            wait_ms(100);
            continue;
        }
        
        if (!pc.readable()) continue;
        char c = pc.getc();
        if (c != 'a' - 2) continue;
        //while (c != 'a' - 2) {
//            c = pc.getc();
//        }
        for (int i = 0; i < NUM_UNITS; i++) {
            float val = (float)(pc.getc() - 'a') / (float)(100.0);
            val = val*val*val*val*val;
//            printf("i: %d c: %c, val: %.1f\r\n", i, c, val);
            setServoPulse(i, val);
            
            int s = pc.getc() - 'a';
            float r = s == 0 ? val : 0.0;
            float g = s == 1 ? val : 0.0;
            float b = s == 2 ? val : 0.0;
            //c = pc.getc();
//            float g = (float)(c - 'a') / (float)(100.0);
//            c = pc.getc();
//            float b = (float)(c - 'a') / (float)(100.0);
            colors[i] = (rgb_color) {g * 255, r * 255, b * 255};
//            myleds.setPixel(i, (uint8_t) (g*255), (uint8_t)(r*255), (uint8_t)(b*255));
        }
        ledStrip.write(colors, NUM_UNITS);
    }
    
    //while(1) {
//        setServoPulse(n, pulse);
//        setServoPulse(n+1, pulse);
//        setServoPulse(n+2, pulse);
//        printf("min: %.4f, max: %.4f, pulse: %.4f\r\n", min_pulse, max_pulse, pulse);
//        
//        switch(pc.getc()) {
//            case '1': pulse -= 0.05; break;
//            case '2': pulse = 0.5; break;
//            case '3': pulse += 0.05; break;
//            
//            case '7': min_pulse = pulse; break;
//            case '8': max_pulse = pulse; break;
//            case '9': pulse = min_pulse; break;
//            case '0': pulse = max_pulse; break;
//        }
//    }
}