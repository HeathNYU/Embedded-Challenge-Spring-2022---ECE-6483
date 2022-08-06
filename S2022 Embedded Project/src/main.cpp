#include <mbed.h>
#include "drivers/LCD_DISCO_F429ZI.h"

#define BACKGROUND 1 //set background
#define FOREGROUND 0 //set foreground
#define GRAPH_PADDING 5 //set padding deafault

using namespace std::chrono; 

LCD_DISCO_F429ZI lcd; //bring in lcd functions

//buffer for holding displayed text strings
char display_buf[5][60];

//sets the background layer , and rests coor to black
void setup_background_layer(){
  lcd.SelectLayer(BACKGROUND);
  lcd.Clear(LCD_COLOR_BLACK);
  lcd.SetBackColor(LCD_COLOR_BLACK);
  lcd.SetTextColor(LCD_COLOR_GREEN);
  lcd.SetLayerVisible(BACKGROUND,ENABLE);
  lcd.SetTransparency(BACKGROUND,0x7Fu);
}

//resets the foreground layer to black
void setup_foreground_layer(){
    lcd.SelectLayer(FOREGROUND);
    lcd.Clear(LCD_COLOR_BLACK);
    lcd.SetBackColor(LCD_COLOR_BLACK);
    lcd.SetTextColor(LCD_COLOR_LIGHTGREEN);
}

uint32_t byte1;
uint32_t byte2;
uint32_t byte3;
uint32_t byte4;
uint32_t byte5;
uint32_t byte6;
uint32_t byte7;
uint32_t byte8;
uint32_t byte9;
uint32_t byte10;
uint32_t byte11;
uint32_t byte12;

uint32_t pressureReading;
uint32_t MAP;
uint32_t HR;
uint32_t SYS;
uint32_t DIA;

uint32_t lastReadValue; 
uint32_t newReadValue;
uint32_t lastReadValueTime;
uint32_t newReadValueTime;

uint32_t newPulseTime;
uint32_t lastPulseTime;

uint32_t largestPulseSize;

uint32_t countsLeft;
uint64_t avgTime;

bool SYSRecorded;
bool lastPulseTimeRecorded;

SPI spi(PA_7, PA_6, PA_5); // mosi, miso, sclk
DigitalOut cs(PA_4); //ss

Timer t;

int main() {

      // Chip deselected
      cs = 1;

      t.start();
      
      // Setup the spi for 8 bit data, spi3,
      // second edge capture, with a 1MHz clock rate
      spi.format(8,1);
      spi.frequency(1000000);

      setup_background_layer();
      setup_foreground_layer();

      while (1) {

        byte1 = 0;
        byte2 = 0;
        byte3 = 0;
        byte4 = 0;
        byte5 = 0;
        byte6 = 0;
        byte7 = 0;
        byte8 = 0;
        byte9 = 0;
        byte10 = 0;
        byte11 = 0;
        byte12 = 0;

        pressureReading = 0;
        MAP = 0;
        HR = 0;
        SYS = 0;
        DIA = 0;

        lastReadValue = 0;
        newReadValue = 0;
        lastReadValueTime = 0;
        newReadValueTime = 0;

        newPulseTime = 0;
        lastPulseTime = 0;

        largestPulseSize = 0;

        countsLeft = 5;
        avgTime = 0;

        SYSRecorded = false;
        lastPulseTimeRecorded = false;

        printf("System Time: %llu\n", duration_cast<milliseconds>(t.elapsed_time()).count());
        
        cs = 0; //Select chip

        spi.write(0xAA); //clear miso
        spi.write(0x00); //clear second segment
        spi.write(0x00); //clear third segment

        //thread_sleep_for(5); //delay write

        spi.write(0xF0); //initialize write for incoming pressure readings
        
        byte1 = spi.write(0x00); //first segment of the pressure output
        byte2 = spi.write(0x00); //second segment of the pressure output
        byte3 = spi.write(0x00); //third segment of the pressure output
        byte4 = spi.write(0x00);
        byte5 = spi.write(0x00);
        byte6 = spi.write(0x00);
        byte7 = spi.write(0x00);
        byte8 = spi.write(0x00);
        byte9 = spi.write(0x00);
        byte10 = spi.write(0x00);
        byte11 = spi.write(0x00);
        byte12 = spi.write(0x00);
        
        cs = 1;

        pressureReading = (((byte6 << 16) | (byte7 << 8) | byte8) - 400000) / 10000; //add values to register

        printf("Pressure Read: %lu\n", pressureReading);
        printf("Byte 1: %lu\n", byte1);
        printf("Byte 2: %lu\n", byte2);
        printf("Byte 3: %lu\n", byte3);
        printf("Byte 4: %lu\n", byte4);
        printf("Byte 5: %lu\n", byte5);
        printf("Byte 6: %lu\n", byte6);
        printf("Byte 7: %lu\n", byte7);
        printf("Byte 8: %lu\n", byte8);
        printf("Byte 9: %lu\n", byte9);
        printf("Byte 10: %lu\n", byte10);
        printf("Byte 11: %lu\n", byte11);
        printf("Byte 12: %lu\n\n", byte12);

        snprintf(display_buf[0],60,"Standing By");
        lcd.SelectLayer(FOREGROUND);
        //display the buffered string on the screen
        lcd.DisplayStringAt(8, 0, (uint8_t *)display_buf[0], LEFT_MODE);

        snprintf(display_buf[1],60,"Pressure Read:       "); //erasing previous line
        lcd.DisplayStringAt(8, LINE(2), (uint8_t *)display_buf[1], LEFT_MODE);

        snprintf(display_buf[1],60,"Pressure Read: %lu", pressureReading);
        lcd.DisplayStringAt(8, LINE(2), (uint8_t *)display_buf[1], LEFT_MODE);

        snprintf(display_buf[2],60,"Heart Rate: --     ");
        lcd.DisplayStringAt(8, LINE(6), (uint8_t *)display_buf[2], LEFT_MODE);

        snprintf(display_buf[3],60,"Systolic Value: --     ");
        lcd.DisplayStringAt(8, LINE(7), (uint8_t *)display_buf[3], LEFT_MODE);

        snprintf(display_buf[4],60,"Diastolic Value: --     ");
        lcd.DisplayStringAt(8, LINE(8), (uint8_t *)display_buf[4], LEFT_MODE);

        //Reads in the data given several conditions
        if(pressureReading >= 150 && pressureReading <= 350) {//verifies that the blood pressure is in the correct range

              thread_sleep_for(2000);
        
              while(pressureReading > 30) {
              //starts adding data when blood flow restarts

              snprintf(display_buf[0],60,"                                       "); //erasing previous line
              lcd.DisplayStringAt(8, LINE(0), (uint8_t *)display_buf[0], LEFT_MODE);

              snprintf(display_buf[0],60,"Test in Progress");
              lcd.DisplayStringAt(8, LINE(0), (uint8_t *)display_buf[0], LEFT_MODE);
              
              cs = 0;

              spi.write(0xAA); //clear miso
              spi.write(0x00); //clear second segment
              spi.write(0x00); //clear third segment

              //thread_sleep_for(5);
        
              spi.write(0xF0); //initialize write for incoming pressure readings
        
              byte1 = spi.write(0x00); //first segment of the pressure output
              byte2 = spi.write(0x00); //second segment of the pressure output
              byte3 = spi.write(0x00); //third segment of the pressure output
              byte4 = spi.write(0x00);
              byte5 = spi.write(0x00);
              byte6 = spi.write(0x00);
              byte7 = spi.write(0x00);
              byte8 = spi.write(0x00);
              byte9 = spi.write(0x00);
              byte10 = spi.write(0x00);
              byte11 = spi.write(0x00);
              byte12 = spi.write(0x00);

              cs = 1;

              pressureReading = (((byte6 << 16) | (byte7 << 8) | byte8) - 400000) / 10000; //add values to register

              printf("Pressure Read: %lu\n", pressureReading);
              printf("Byte 1: %lu\n", byte1);
              printf("Byte 2: %lu\n", byte2);
              printf("Byte 3: %lu\n", byte3);
              printf("Byte 4: %lu\n", byte4);
              printf("Byte 5: %lu\n", byte5);
              printf("Byte 6: %lu\n", byte6);
              printf("Byte 7: %lu\n", byte7);
              printf("Byte 8: %lu\n", byte8);
              printf("Byte 9: %lu\n", byte9);
              printf("Byte 10: %lu\n", byte10);
              printf("Byte 11: %lu\n", byte11);
              printf("Byte 12: %lu\n\n", byte12);

              snprintf(display_buf[1],60,"Pressure Read:       "); //erasing previous line
              lcd.DisplayStringAt(8, LINE(2), (uint8_t *)display_buf[1], LEFT_MODE);
              
              snprintf(display_buf[1],60,"Pressure Read: %lu", pressureReading);
              //display the buffered string on the screen
              lcd.DisplayStringAt(8, LINE(2), (uint8_t *)display_buf[1], LEFT_MODE);

              if(pressureReading <= 150) {

                newReadValue = (((byte6 << 16) | (byte7 << 8) | byte8) - 400000) / 100; //A more precise version of pressureReading

                //detect peaks
                if(lastReadValue != 0 && newReadValue > lastReadValue + 85) {
                  
                  //If the first peak is recorded - read the same as the systolic value
                  if(!SYSRecorded) {
                    
                    SYS = newReadValue / 100;
                    SYSRecorded = true;

                  }
                  else if (newReadValue - lastReadValue > largestPulseSize){
                    
                    largestPulseSize = newReadValue - lastReadValue;
                    MAP = newReadValue / 100;

                  }
                  
                  if(!lastPulseTimeRecorded) {
                    
                    newPulseTime = duration_cast<milliseconds>(t.elapsed_time()).count();
                    lastPulseTimeRecorded = true;
                    
                  }
                  else if(countsLeft != 0){

                    lastPulseTime = newPulseTime;
                    newPulseTime = duration_cast<milliseconds>(t.elapsed_time()).count();

                    avgTime = (avgTime * (5 - countsLeft) + (newPulseTime - lastPulseTime)) / (6 - countsLeft);

                    countsLeft--;

                  }
                  else {

                    //Calculate heart rate
                    HR = 60 * 1000 / avgTime;
                    
                  }

                lastReadValue = newReadValue;

                }
                else {

                  lastReadValue = newReadValue;

                }

                thread_sleep_for(50);

                }
              
              }

              DIA = (3 * MAP - SYS) / 2;
              

              snprintf(display_buf[0],60,"                                       "); //erasing previous line
              lcd.DisplayStringAt(8, LINE(0), (uint8_t *)display_buf[0], LEFT_MODE);

              snprintf(display_buf[0],60,"Test Complete");
              lcd.DisplayStringAt(8, LINE(0), (uint8_t *)display_buf[0], LEFT_MODE);

              snprintf(display_buf[2],60,"Heart Rate:       "); //erasing previous line
              lcd.DisplayStringAt(8, LINE(6), (uint8_t *)display_buf[2], LEFT_MODE);

              snprintf(display_buf[2],60,"Heart Rate: %lu", HR);
              lcd.DisplayStringAt(8, LINE(6), (uint8_t *)display_buf[2], LEFT_MODE);

              snprintf(display_buf[3],60,"Systolic Value:       "); //erasing previous line
              lcd.DisplayStringAt(8, LINE(7), (uint8_t *)display_buf[3], LEFT_MODE);

              snprintf(display_buf[3],60,"Systolic Value: %lu", SYS);
              lcd.DisplayStringAt(8, LINE(7), (uint8_t *)display_buf[3], LEFT_MODE);
              
              snprintf(display_buf[4],60,"Diastolic Value:       "); //erasing previous line
              lcd.DisplayStringAt(8, LINE(8), (uint8_t *)display_buf[4], LEFT_MODE);

              snprintf(display_buf[4],60,"Diastolic Value: %lu", DIA);
              lcd.DisplayStringAt(8, LINE(8), (uint8_t *)display_buf[4], LEFT_MODE);
              
              thread_sleep_for(15000);

              snprintf(display_buf[0],60,"                                       "); //erasing previous line
              lcd.DisplayStringAt(8, LINE(0), (uint8_t *)display_buf[0], LEFT_MODE);
            }

            thread_sleep_for(250);

      }

}