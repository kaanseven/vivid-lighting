/**
 * Project: Sunrise 
 * 
 * @author Kaan Seven
 */

#include <math.h>

#define NUMINTERVALS      6   // Number of intervals (used in array value so don't change unless array is changed too)
#define BRIGHTNESS_PIN    2   // Analogue pin on board linked to potentiometer which adjusts brightness
#define INTERVAL_PIN      4   // Analogue pin on board linked to potentiometer which time intervals
#define DEBUG_PIN         6   // Analogue pin for debug mode -- not used
#define RED1_PIN          3   // Pin 3 for PWM control, change if using different pin
#define GREEN1_PIN        9   // Pin 9 for PWM control, change if using different pin
#define BLUE1_PIN         11  // Pin 11 for PWM control, change if using different pin

//Used for multicolour display - ignore these pins
#define RED2_PIN          15   // Pin 3 for PWM control, change if using different pin
#define GREEN2_PIN        16   // Pin 9 for PWM control, change if using different pin
#define BLUE2_PIN         17  // Pin 11 for PWM control, change if using different pin
#define RED3_PIN          18   // Pin 3 for PWM control, change if using different pin
#define GREEN3_PIN        19   // Pin 9 for PWM control, change if using different pin
#define BLUE3_PIN         20  // Pin 11 for PWM control, change if using different pin

/*********************************************************************
 *                    TIMING CONSTANTS                               *
 *********************************************************************/
const unsigned long minIntervalDuration = 1000;   // Minimum duration for interval (ms)
const unsigned long maxIntervalDuration = 20000;  // Max duration for interval (ms)
const float holdDuration = 0.5; // Change value to reflect the fraction of maxIntervalDuration
const float fadeDuration = 0.5; // Change value to reflect the fraction of maxIntervalDuration
const unsigned long offDuration = 10000; // Duration for off interval (ms)
const int tickInterval = 100; 
const long addTick = maxIntervalDuration * tickInterval / minIntervalDuration - tickInterval; 

/*********************************************************************
 *                    COLOUR ARRAYS                                  *
 *********************************************************************/

// Arrays that hold the current rgb values of the LEDs
float amberColour[] = {0, 0, 0}; 
float redColour[] = {0, 0, 0}; 
float whiteColour[] = {0, 0, 0};

// Alter this array for 1 colour setup
float amberInterval[NUMINTERVALS][3] = {
  {200, 0, 0},
  {255,80, 0},
  {255,160, 0},
  {255,255, 255},
  {255,90, 255},
  {255, 50, 0}
};

// IGNORE ARRAYS BELOW THIS LINE
// Alter red and white interval arrays for multicolour setup
float redInterval[NUMINTERVALS][3] = { 
  {100,0,0},
  {200,0,0},
  {180,0,0},
  {150,0,0},
  {140,0,0},
  {100,0,0}
};
float whiteInterval[NUMINTERVALS][3] = {
  {10,10,10},
  {40,40,40},
  {150,150,150},
  {200,200,200},
  {175,175,175},
  {100,100,100}
};

void setup() {
  Serial.begin(9600);
  while (! Serial);
  pinMode(RED1_PIN, OUTPUT);
  pinMode(GREEN1_PIN, OUTPUT);
  pinMode(BLUE1_PIN, OUTPUT);
}

void loop() {
  for (int i = 0; i < NUMINTERVALS; i++) {
    sunriseFunction(i);
  }
  holdFunction();
  fadeColours(); 
  offFunction();
}

/**
 * Turns all LEDs off for a certain period of time
 */
void offFunction() {
  memset(amberColour,0,sizeof(amberColour)); // guarantees all values in array will be set to 0
  memset(redColour,0,sizeof(redColour));
  memset(whiteColour,0,sizeof(whiteColour));
  changeColour(); 
  delay(offDuration);
}

/**
 * Holds all LEDs at a certain colour/brightness for a period of time
 * The brightness of the LEDs and the duration of this hold function 
 * are dependent on two potentiometer inputs
 */
void holdFunction() {
  unsigned long delayCount = 0;
  //loop to change brightness and change interval duration during hold
  while (delayCount < holdDuration * maxIntervalDuration) {
    long delayTick = tickInterval + addTick - addTick * (1023 - analogRead(INTERVAL_PIN))/1023;
    changeColour();
    delayCount = delayCount + delayTick;
    delay(tickInterval);
  }
}

/**
 * Update colour of LEDs.
 * Checks brightness level based on potentiometer reading and sets pwm value for specific output pins.
 */
void changeColour() {
  int i=analogRead(BRIGHTNESS_PIN);
  if (i > 1000) {
    i = 1023;
  }
  else {
    i = (i+50)/100;
    i = 100*i;
  }
  int potValue = 1023 - i;
  analogWrite(RED1_PIN, amberColour[0] * long(potValue) / 1023); // use 5v pin so potValue = max of 1023
  analogWrite(GREEN1_PIN, amberColour[1] * long(potValue) / 1023);
  analogWrite(BLUE1_PIN, amberColour[2] * long(potValue) / 1023);
  //analogWrite(RED2_PIN, redColour[0] * long(potValue) / 1023);
  //analogWrite(GREEN2_PIN, redColour[1] * long(potValue) / 1023);
  //analogWrite(BLUE2_PIN, redColour[2] * long(potValue) / 1023);
  //analogWrite(RED3_PIN, whiteColour[0] * long(potValue) / 1023);
  //analogWrite(GREEN3_PIN, whiteColour[1] * long(potValue) / 1023);
  //analogWrite(RED3_PIN, whiteColour[2] * long(potValue) / 1023);
}

/**
 * Fades all the LEDs to 'off' over a period of time
 */
void fadeColours() {
  unsigned long delayCount = 0;
  float amberRatios[] = { 0 , 0 , 0 };
  float redRatios[] = { 0 , 0 , 0 };
  float whiteRatios[] = { 0 , 0 , 0 };
  colourRatios(amberColour, amberRatios);
  colourRatios(redColour, redRatios);
  colourRatios(whiteColour, whiteRatios);
  while (delayCount < fadeDuration * maxIntervalDuration) {
    unsigned long delayTick = tickInterval + addTick - addTick * (1023 - analogRead(INTERVAL_PIN))/1023;
    fadeFunction(amberColour, amberRatios, delayTick, delayCount);
    fadeFunction(redColour, redRatios, delayTick, delayCount);
    fadeFunction(whiteColour, whiteRatios, delayTick, delayCount);
    changeColour();
    delayCount = delayCount + delayTick;
    debugFunct(delayCount);
    delay(tickInterval);
  }
}

/**
 * Finds the ratios of all the colour values (pwm values) based on the largest value
 * passed to the function via colour[]
 * 
 * @param colour[] array of values which reflect rgb values
 * @param ratios[] array of ratios of the colours
 */
void colourRatios(float colour[], float ratios[]) {
  int largestNumber = 0;
  for (int i = 0; i < 3; i++) {
    if (colour[i] > largestNumber && colour[i] > 0) {
      largestNumber = colour[i];
    }
  }
  if (largestNumber == 0) {
    return;
  }
  ratios[0] = (float)colour[0]/largestNumber;
  ratios[1] = (float)colour[1]/largestNumber;
  ratios[2] = (float)colour[2]/largestNumber;
}

/**
 * Reduces values of colour[] array to 0 over a period of time
 * 
 * @param colour[] the array which will be adjusted
 * @param ratioArray[] an array which has ratios of each of the values based on the largest value
 * @param delayTick a tickcounter which is calculated based on min and max interval durations
 * @param delayCount how much time has already elasped
 */
void fadeFunction(float colour[], float ratioArray[], long delayTick, long delayCount) { 
  int maxIndex = 0;
  if (colour[0] > 0 || colour[1] > 0 || colour[2] > 0) {
    for (int i = 0; i < 3; i++) {
      if (colour[i] > colour[maxIndex] && colour[i] > 0) {
        maxIndex = i;
      }
    }
    float deductionAmount = colour[maxIndex] * delayTick / (long(fadeDuration * maxIntervalDuration)-delayCount);
    colour[maxIndex] = colour[maxIndex] - deductionAmount;
    for (int i = 0; i < 3; i++) {
      if (colour[i] > 0) {
        int newCol = colour[maxIndex] * ratioArray[i];
        if (newCol < 1) {
          colour[i] = 0;
        }
        else {
          colour[i] = newCol;
        }
        
        if (colour[i] > 255) {
          colour[i] = 255;
        }
        if (colour[i] < 0) {
          colour[i] = 0;
        }
      }
    }
    if (colour[0] + colour[1] + colour[2] < 3) {
      colour[0] = 0;
      colour[1] = 0;
      colour[2] = 0;
    }
  }
}

/**
 * Runs a function to adjust colour levels over a set period of time.
 * The amount of time it runs for is variable based on a potentiometer reading.
 * The values in amberInterval, redInterval, and whiteInterval arrays are the targeted
 * values which amberColour, redColour, and whiteColour arrays will hold at completetion
 * of the function.
 * 
 * @param i used to select which targeted values should be used
 */
void sunriseFunction(int i) {
  unsigned long delayCount = 0;
  while (delayCount < maxIntervalDuration) {
    long delayTick = tickInterval + addTick - addTick * (1023 - analogRead(INTERVAL_PIN))/1023;
    linearFunction(amberColour, amberInterval[i], delayTick, delayCount);
    //linearFunction(redColour, redInterval[i], delayTick, delayCount);
    //linearFunction(whiteColour, whiteInterval[i], delayTick, delayCount);
    changeColour();
    delayCount = delayCount + delayTick;
    debugFunct(delayCount);
    delay(tickInterval);
  }
}

/**
 * Increments the values in colour linearly to reach a value in the targetColour array based
 * upon the amount of time which is calculated using delayTick, delayCount and maxIntervalDuration
 * 
 * @param colour[] the current RGB values of an LED
 * @param targetColour[] the targeted RGB of an LED
 * @param delayTick a tickcounter which is calculated based on min and max interval durations
 * @param delayCount how much time has already elasped
 */
void linearFunction(float colour[], float targetColour[], long delayTick, long delayCount) {
  for (int i = 0; i < 3; i++) {
      colour[i] = colour[i] + (targetColour[i]-colour[i]) * delayTick / (long(maxIntervalDuration)-delayCount);
      if (colour[i] > 255) {
        colour[i] = 255;
      }
      else if (colour[i] < 0) {
        colour[i] = 0;
      }
  }
}

/**
 * Used for debugging.
 * Prints out values of an array which is supposed to represent the RGB values
 * 
 * @param info unsigned long value which would be generated within a function
 */
void debugFunct(unsigned long info) {
    Serial.print(amberColour[0]);
    Serial.print(" , ");
    Serial.print(amberColour[1]);
    Serial.print(" , ");
    Serial.print(amberColour[2]);
    Serial.print(" , ");
    Serial.println(info);
}

/**
 * Debugging function to test values of yellow
 * Not used
 */
void debugColourMode() {
  while(true) {
    int potValue = analogRead(DEBUG_PIN);
    analogWrite(RED1_PIN, 255 * long(potValue) / 1023);
    analogWrite(GREEN1_PIN, 255 * long(potValue) / 1023);
    analogWrite(BLUE1_PIN, 0);
    Serial.print(255 * long(potValue) / 1023);
    Serial.print(",");
    Serial.print(255 * long(potValue) / 1023);
    Serial.println(",0");
    delay(100);
  }
}

