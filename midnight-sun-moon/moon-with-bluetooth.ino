/**
 * Project: Multifunction Neopixel Project
 * Description: 
 * 1. White band of LEDs that chase around a circle of 2 fixed colours
 * 2. Simulates the waxing/waning of the moon
 * 3. Connect via bluetooth to change the colour of the neopixels
 * 
 * @author Kaan Seven
 */

#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif
#include "BluefruitConfig.h"

#define NEOPIXEL_PIN    3   // Digital PWM pin for neopixel strip
#define LOCATION_PIN    2   // Analogue pin for potentiometer to adjust starting positions for neopixel functions
#define INTERVAL_PIN    4   // Analogue pin for potentiometer to adjust time intervals
#define SWITCH1_PIN     5   // Digital pin to read rotary switch
#define SWITCH2_PIN     6   // Digital pin to read rotary switch
#define SWITCH3_PIN     2   // Digital pin to read rotary switch

const int tickInterval = 10; 
/*********************************************************************
 *                    NEOPIXEL VARIABLES                             *
 *********************************************************************/
const int numOfPixels = 79; // Number of neopixels
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numOfPixels, NEOPIXEL_PIN); 

/*********************************************************************
 *                    CHASER VARIABLES                               *
 *********************************************************************/
//Timing Constants
const unsigned long chaserMinIntervalTime = 200;    // Minimum time for chaser to mvoe one position (ms)
const unsigned long chaserMaxIntervalTime = 10000;  // Maximum time for chaser to move one position (ms)
const unsigned long chaserStartingTime = 10000;     // Time to select starting orientation (ms)
const long chsAddTick = chaserMaxIntervalTime * tickInterval / chaserMinIntervalTime - tickInterval; 

//RGB Values for Chaser
const float chsColour1[] = { 0, 0 , 255 }; // Blue
const float chsColour2[] = { 0, 255 , 0 }; // Green
const float chsColour3[] = { 255 , 255 , 255 }; // White

//Location & Size values
int chsStartingPixel = -1;  // Where the pixel will start from, will be altered with the use of LOCATION_PIN, don't touch
int numOfChasePixels = 3;   // Number of pixels which will rotate around the circle -use odd number-
int chaser = 55;            // Starting Pixel of the 'chaser' LEDs, make sure this value is less than the number of neopixels

/*********************************************************************
 *                    MOON           VARIABLES                       *
 *********************************************************************/
//Timing Constants
const unsigned long moonMinIntervalTime = 1000;   // Minimum time between each progression of the moon cycle (ms)
const unsigned long moonMaxIntervalTime = 10000;  // Maximum time between each progression of the moon cycle (ms)
const unsigned long moonStartingTime = 12000;     // Time to select starting position (ms)
const unsigned long moonFadeToColourTime = 5000;  // Time to fade from yellow to red, and from red to off (ms)
const unsigned long moonHoldTime = 5000;          // Time to hold colour (it holds colour before changing to bloodmoon) (ms)
const unsigned long moonOffTime = 5000;           // Time to spend completely off after fadeout from bloodmoon (ms)
const long addTick = moonMaxIntervalTime * tickInterval / moonMinIntervalTime - tickInterval; 

//RGB Values for Moon
const float moonColour1[] = { 255, 180, 0 }; // Yellow
const float moonColour2[] = { 120, 0 , 0 }; // Red (bloodmoon)
const float moonColour3[] = { 0 , 0 , 0 }; // Off

//Other 
int moonStartingPixel = 0; // Where the pixel will start from, will be altered with the use of LOCATION_PIN
const int numberOfCycles = 30; // Number of cycles

/*********************************************************************
 *                    BLUETOOTH APP VARIABLES                        *
 *********************************************************************/
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);
Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN, BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);
/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO, BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);
extern uint8_t packetbuffer[];

/*********************************************************************
 *                    Initial Setup                                  *
 *********************************************************************/
void setup() {
  Serial.begin(9600);
  while (! Serial);
  pinMode(NEOPIXEL_PIN, OUTPUT);
  pinMode(SWITCH1_PIN, INPUT_PULLUP);  
  pinMode(SWITCH2_PIN, INPUT_PULLUP);
  pinMode(SWITCH3_PIN, INPUT_PULLUP);
  strip.begin();
  for(int i = 0; i < numOfPixels; i++) {
    strip.setPixelColor(i, 0, 0, 0); // Sets all pixels to 'off'
  }
  strip.show();
}

/*********************************************************************
 *                            LOOP                                   *
 *          Runs after setup or if switch changes position           *
 *********************************************************************/
void loop() {
  if (digitalRead(SWITCH1_PIN) == LOW)
  {
    chsStartingPixel = -1;
    chaseStartingFunction();
    //beginTheChase1(); //anticlockwise
    beginTheChase2(); //clockwise
  }
  else if (digitalRead(SWITCH2_PIN) == LOW)
  {
    moonStartingPixel = moonStartingFunction();
    while (digitalRead(SWITCH2_PIN) == LOW) {
      moonCycles(moonStartingPixel);
      fadeToColour(moonColour1, moonColour2);
      moonOffHoldFunction(moonHoldTime);
      fadeToColour(moonColour2, moonColour3);
      moonOffHoldFunction(moonOffTime);
    }
  }
  else if (digitalRead(SWITCH3_PIN) == LOW)
  {
    if ( !ble.begin(VERBOSE_MODE) )
    {
      //error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
    }
    ble.echo(false);
    while (!ble.isConnected()) {
      if (digitalRead(SWITCH3_PIN) != LOW) {
        break;
      }
      delay(500);
    }
    ble.verbose(false);
    if (digitalRead(SWITCH3_PIN) == LOW) {
      ble.setMode(BLUEFRUIT_MODE_DATA);
      bluetoothLoop();
    }
    ble.end();
  }
  for (int i = 0; i < numOfPixels; i++) {
    strip.setPixelColor(i, 0, 0, 0); 
  }
  strip.show();
  delay(100);
}

/**
 * Allows the user to select the orientation of the green/blue LEDs
 */
void chaseStartingFunction() {
  unsigned long delayCounter = 0;
  while (delayCounter < chaserStartingTime) {
    if (digitalRead(SWITCH1_PIN) != LOW) break; 
    int newchsStartingPixel = (numOfPixels - 1) * ((float)analogRead(LOCATION_PIN)/1023);
    if (newchsStartingPixel != chsStartingPixel) {
        for (int i = 0; i < numOfPixels; i++) {
          if (newchsStartingPixel > (numOfPixels/2)) {
            if (i >= newchsStartingPixel || i < newchsStartingPixel+(numOfPixels/2)-numOfPixels) {
              strip.setPixelColor(i, chsColour1[0], chsColour1[1], chsColour1[2]); 
            }
            else {
              strip.setPixelColor(i, chsColour2[0], chsColour2[1], chsColour2[2]); 
            }
          }
          else {
            if (i >= newchsStartingPixel && i < newchsStartingPixel+(numOfPixels/2)) {
              strip.setPixelColor(i, chsColour1[0], chsColour1[1], chsColour1[2]); 
            }
            else {
              strip.setPixelColor(i, chsColour2[0], chsColour2[1], chsColour2[2]); 
            }
          }
        }
      chsStartingPixel = newchsStartingPixel;
      strip.show();
    }
    delayCounter = delayCounter + tickInterval;
    delay(tickInterval);
  }
}

/**
 * Anti-Clockwise rotation of a set number of white pixels
 */
void beginTheChase1() {
  int threshold = numOfPixels/2;
  while(true) {
    if (digitalRead(SWITCH1_PIN) != LOW) break; 
    if ((chaser-1) >= chsStartingPixel && (chaser-1) <= chsStartingPixel+threshold && chsStartingPixel <= threshold) {
      strip.setPixelColor(chaser-1, chsColour1[0], chsColour1[1], chsColour1[2]);
    }
    else if (((chaser-1) >= chsStartingPixel || (chaser-1) <= chsStartingPixel-threshold) && chsStartingPixel > threshold) {
      strip.setPixelColor(chaser-1, chsColour1[0], chsColour1[1], chsColour1[2]);
    }
    else {
      strip.setPixelColor(chaser-1, chsColour2[0], chsColour2[1], chsColour2[2]);
    }
    if (chaser >= numOfPixels) {
      chaser = 0;
    }
    for (int i = 0; i < numOfChasePixels; i++) {
      if ((chaser+i) >= numOfPixels) {
        strip.setPixelColor(chaser+i-numOfPixels, chsColour3[0], chsColour3[1], chsColour3[2]);
      }
      else {
        strip.setPixelColor(chaser+i, chsColour3[0], chsColour3[1], chsColour3[2]);
      }
    }
    strip.show();
    chaser++;
    unsigned long delayCounter = 0;
    while (delayCounter < chaserMaxIntervalTime) {
      long delayTick = tickInterval + chsAddTick - chsAddTick * analogRead(INTERVAL_PIN)/1023;
      delayCounter = delayCounter + delayTick;
      delay(tickInterval);
    }
  }
}

/**
 * Clockwise rotation of a set number of white pixels
 */
void beginTheChase2() {
  int threshold = numOfPixels/2;
  while(true) {
    if (digitalRead(SWITCH1_PIN) != LOW) break; 
    if ((chaser+1) >= chsStartingPixel && (chaser+1) <= chsStartingPixel+threshold && chsStartingPixel <= threshold) {
      strip.setPixelColor(chaser+1, chsColour1[0], chsColour1[1], chsColour1[2]);
    }
    else if (((chaser+1) >= chsStartingPixel || (chaser+1) <= chsStartingPixel-threshold) && chsStartingPixel > threshold) {
      strip.setPixelColor(chaser+1, chsColour1[0], chsColour1[1], chsColour1[2]);
    }
    else {
      strip.setPixelColor(chaser+1, chsColour2[0], chsColour2[1], chsColour2[2]);
    }
    if (chaser < 0) {
      chaser = numOfPixels-1;
    }
    for (int i = 0; i < numOfChasePixels; i++) {
      if ((chaser-i) < 0) {
        strip.setPixelColor(chaser-i+numOfPixels, chsColour3[0], chsColour3[1], chsColour3[2]);
      }
      else {
        strip.setPixelColor(chaser-i, chsColour3[0], chsColour3[1], chsColour3[2]);
      }
    }
    strip.show();
    chaser--;
    unsigned long delayCounter = 0;
    while (delayCounter < chaserMaxIntervalTime) {
      long delayTick = tickInterval + chsAddTick - chsAddTick * analogRead(INTERVAL_PIN)/1023;
      delayCounter = delayCounter + delayTick;
      delay(tickInterval);
    }
  }
}

/**
 * Cycles through a number of intervals (specified in the constants) to simulate the moon cycles
 * 
 * @param moonStartingPixel the number of the pixel where the lighting will 'grow' from
 */
void moonCycles(int moonStartingPixel) {
  int n = 1;
  while (n <= numberOfCycles) {
    if (digitalRead(SWITCH2_PIN) != LOW) break; 
    unsigned long delayTime = moonMinIntervalTime + moonMaxIntervalTime * analogRead(INTERVAL_PIN)/1023;   
    int pixelLevel = (n*numOfPixels)/(numberOfCycles*2);
    for (int i = 0; i < numOfPixels; i++) {
      if (i <= pixelLevel) {
        int adjustedPixelPos = moonStartingPixel+i;
        int adjustedPixelNeg = moonStartingPixel-i;
        if (adjustedPixelPos > numOfPixels-1) {
          adjustedPixelPos = adjustedPixelPos-numOfPixels;
        }
        if (adjustedPixelNeg < 0) {
          adjustedPixelNeg = numOfPixels + adjustedPixelNeg;
        }
        strip.setPixelColor(adjustedPixelPos, moonColour1[0], moonColour1[1], moonColour1[2]); 
        strip.setPixelColor(adjustedPixelNeg, moonColour1[0], moonColour1[1], moonColour1[2]);     
      }
    }
    strip.show();
    unsigned long delayCounter = 0;
    while (delayCounter < moonMaxIntervalTime) {
      long delayTick = tickInterval + addTick - addTick * analogRead(INTERVAL_PIN)/1023;
      delayCounter = delayCounter + delayTick;
      delay(tickInterval);
    }
    n++;
  } 
}

/**
 * Allows the user to select the starting position for the moon function and returns the integer value of the
 * selected neopixel
 * 
 * @return the number of the starting pixel.
 */
int moonStartingFunction() {
  unsigned long delayCounter = 0;
  int startingPosition = moonStartingPixel;
  strip.setPixelColor(startingPosition, moonColour1[0], moonColour1[1], moonColour1[2]);
  strip.show();
  while (delayCounter < moonStartingTime) {
    if (digitalRead(SWITCH2_PIN) != LOW) break; 
    int newStartingPosition = (numOfPixels - 1) * ((float)analogRead(LOCATION_PIN)/1023);
    if (newStartingPosition != startingPosition) {
      strip.setPixelColor(startingPosition, 0, 0, 0);
      startingPosition = newStartingPosition;
      strip.setPixelColor(startingPosition, moonColour1[0], moonColour1[1], moonColour1[2]);
      strip.show();
    }
    delayCounter++;
    delay(1);
  }
  return startingPosition;
}

/**
 * Changes neopixels from one array of rgb values to another array of rgb values over a period of time
 * Colour change is a smooth transition
 * 
 * @param inputColour[] RGB values of what the pixels will start at.
 * @param targetColour[] RGB values of what the pixels will finish at.
 */
void fadeToColour(float inputColour[], float targetColour[]) {
  unsigned long delayCounter = 0;
  float currColour[3] = { inputColour[0], inputColour[1], inputColour[2] };
  while (delayCounter < moonFadeToColourTime) {
    if (digitalRead(SWITCH2_PIN) != LOW) break; 
    for (int i = 0; i < 3; i++) {
      currColour[i] = currColour[i] + (targetColour[i]-currColour[i]) * tickInterval / (long(moonFadeToColourTime)-delayCounter);
      if (currColour[i] < 0) {
        currColour[i] = 0;
      }
    }
    for (int i = 0; i < numOfPixels; i++) {
      strip.setPixelColor(i, currColour[0], currColour[1], currColour[2]);
    }
    strip.show();
    delayCounter = delayCounter + tickInterval;
    delay(tickInterval);
  }
}

/**
 * Holds a specific colour (or no colour) for a period of time
 * While waiting, the loop will check if user has changed positions for a switch (i.e. selecting another function)
 * 
 * @param inputTime amount of time to wait
 */
void moonOffHoldFunction(unsigned long inputTime) {
  unsigned long delayCounter = 0;
  while (delayCounter < inputTime) {
    if (digitalRead(SWITCH2_PIN) != LOW) break; 
    delayCounter = delayCounter + tickInterval;
    delay(tickInterval);
  }
}

/**
 * Reads bluetooth packets and the neopixel strip to reflect what the user has selected 
 * via the app
 */
void bluetoothLoop() {
  while(true) {
    if (digitalRead(SWITCH3_PIN) != LOW) break; 
    uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
    if (len == 0) continue;
    if (packetbuffer[1] == 'C') {
      uint8_t red = packetbuffer[2];
      uint8_t green = packetbuffer[3];
      uint8_t blue = packetbuffer[4];
      for(uint8_t i=0; i < numOfPixels; i++) {
        strip.setPixelColor(i, strip.Color(red,green,blue));
      }
      strip.show(); // This sends the updated pixel color to the hardware.
    }
  }
}

/**
 * A small helper function for the bluetooth app
 */
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
