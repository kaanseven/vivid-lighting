#include "FastLED.h"
#include <Wire.h>
//#include <Adafruit_RGBLCDShield.h>
//#include <utility/Adafruit_MCP23017.h>
#include <SharpIR.h>

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define NUM_STRIPS 5
#define NUM_LEDS 405 //NUM_STRIPS * NUM_LEDS_PER_STRIP
#define OFF 0x0
#define ON 0x1
#define RANGE_SENSOR_PIN    0
#define RANGE_SENSOR_MODEL  1

//Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
SharpIR sharp(RANGE_SENSOR_MODEL, RANGE_SENSOR_PIN );
CRGB leds[NUM_LEDS];
int xArray[NUM_LEDS]; 
int yArray[NUM_LEDS]; 
int xAxisMax = 90;
int yAxisMax = 90;

extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;
uint8_t gCurrentPaletteNumber = 0;
CRGBPalette16 gCurrentPalette( gGradientPalettes[2] );
CRGBPalette16 gTargetPalette( gGradientPalettes[5] );
int gradientPixelShifter = 1; // increase to shift the gradient quicker (1 way of increasing speed)
int check = 0;

const int numReadings = 5;
double average = 80;

void setup() {
  delay(3000); // power-up safety delay
  //lcd.begin(16, 2);
  //lcd.setBacklight(ON);
  //lcd.print("Program: Music Notes");
  FastLED.addLeds<NEOPIXEL, 8>(leds, 0, 160); //array, starting index, length of neopixels
  FastLED.addLeds<NEOPIXEL, 9>(leds, 160, 114);
  FastLED.addLeds<NEOPIXEL, 10>(leds, 271, 131);
  for (int counter = 0; counter < 150; counter++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::White; // Can be any colour
      leds[i].maximizeBrightness(counter);  // 'FastLED_fade_counter' How high we want to fade up to 255 = maximum.
    }
    FastLED.show(); 
    delay(20);
  }
}

void loop()
{
  EVERY_N_SECONDS( 2 ) {
    if (check == 0) {
      gTargetPalette = gGradientPalettes[2];  
      check = 1;
    }
    else {
      gTargetPalette = gGradientPalettes[5];
      check = 0;        
    }
  }
  EVERY_N_MILLISECONDS(75){
    average = average + (sharp.getDistance() - average);
    if (average  < 9.3) {
      for (int senseIndex = 0; senseIndex > -510; senseIndex--) {
        generatePaletteTRAV(leds, NUM_LEDS, gGradientPalettes[16], senseIndex);
        FastLED.show();
      }
      average = 80;
    }
  }
  
  //unsigned long StartTime = micros();
  static uint8_t sy = 0;
  //sy = sy - gradientPixelShifter; /* motion speed "-" goes clockwise, "+" goes anticlockwise */
  //generatePaletteFILL(leds, NUM_LEDS, gCurrentPalette);
  generatePaletteTRAV(leds, NUM_LEDS, gCurrentPalette, sy);
  //unsigned long CurrentTime = micros();
  //unsigned long ElapsedTime = CurrentTime - StartTime;
  //Serial.println(ElapsedTime);
  nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 48);
  //generatePalletPULSE(leds, NUM_LEDS, gCurrentPalette, sy);
  FastLED.show();
  //delay(100);
}


//inaccurate, fast, will cause banding
//approx 4070microseconds for each iteration (1000 LEDS)
void generatePaletteFILL(CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette)
{
  static uint8_t startindex = 0;
  uint8_t inc = 5;
  fill_palette(ledarray, numleds, startindex, inc, gCurrentPalette, 180, LINEARBLEND);
}

//maps gradient correctly, a bit slower..
//approx 6720microseconds for each iteration (1000 LEDS)  
//65% slower, not a huge amount, most of the speed delay is actually from pushing the data to the leds
void generatePaletteTRAV(CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette, uint8_t colourIndex)
{
  for ( int i = 0; i < NUM_LEDS; i++) {
    uint32_t inc = colourIndex + (i * 256) / NUM_LEDS;
    leds[i] = ColorFromPalette( gCurrentPalette, inc, 255, LINEARBLEND);
  }
}

void generatePalletTRAVxAxis(CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette, uint8_t colourIndex) {
  for ( int i = 0; i < NUM_LEDS; i++) {
    uint32_t inc = colourIndex + (xArray[i] * 256) / xAxisMax;
    leds[i] = ColorFromPalette( gCurrentPalette, inc, 255, LINEARBLEND);
  }
}

void generatePalletTRAVyAxis(CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette, uint8_t colourIndex) {
  for ( int i = 0; i < NUM_LEDS; i++) {
    uint32_t inc = colourIndex + (yArray[i] * 256) / yAxisMax;
    leds[i] = ColorFromPalette( gCurrentPalette, inc, 255, LINEARBLEND);
  }

}

void generatePalletPULSE(CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette, uint8_t colourIndex) {
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( gCurrentPalette, colourIndex, 255, LINEARBLEND);
  }
}


double approxRollingAverage(double avg, double new_sample) {
  avg -= avg / 3;
  avg += new_sample / 3;
  return avg;
}

DEFINE_GRADIENT_PALETTE( oneWaveOPO ) {
  0,   255,  50,  0,
  127, 180,  0,  70,
  255, 255,  50,  0
};

DEFINE_GRADIENT_PALETTE( twoWaveOPO ) {
  0,   255,  50,  0,
  64,  180,  0,  70,
  127, 255,  50,  0,
  191, 180,  0,  70,
  255, 255,  50,  0
};

DEFINE_GRADIENT_PALETTE( threeWaveOPO ) {
  0,   0,  0,  255,
  42,  180,  0,  70,
  85,  0,  0,  255,
  127, 180,  0,  70,
  170, 0,  0,  255,
  212, 180,  0,  70,
  255, 0,  0,  255,
};


DEFINE_GRADIENT_PALETTE( oneWavePOP ) {
  0,   180,  0,  70,
  127, 255,  50,  0,
  255, 180,  0,  70
};

DEFINE_GRADIENT_PALETTE( twoWavePOP ) {
  0,   180,  0,  70,
  64,  255,  50,  0,
  127, 180,  0,  70,
  191, 255,  50,  0,
  255, 180,  0,  70
};

DEFINE_GRADIENT_PALETTE( threeWavePOP ) {
  0,   180,  0,  70,
  42,   0,  0,  255,
  85,  180,  0,  70,
  127,  0,  0,  255,
  170, 180,  0,  70,
  212,  0,  0,  255,
  255, 180,  0,  70
};

DEFINE_GRADIENT_PALETTE( es_pinksplash_08_gp ) {
  0, 126, 11, 255,
  127, 197,  1, 22,
  175, 210, 157, 172,
  221, 157,  3, 112,
  255, 157,  3, 112
};

DEFINE_GRADIENT_PALETTE( rainbowsherbet_gp ) {
  0, 255, 33,  4,
  43, 255, 68, 25,
  86, 255,  7, 25,
  127, 255, 82, 103,
  170, 255, 255, 242,
  209,  42, 255, 22,
  255,  87, 255, 65
};

DEFINE_GRADIENT_PALETTE( gr65_hult_gp ) {
  0, 247, 176, 247,
  48, 255, 136, 255,
  89, 220, 29, 226,
  160,   7, 82, 178,
  216,   1, 124, 109,
  255,   1, 124, 109
};

DEFINE_GRADIENT_PALETTE( es_emerald_dragon_08_gp ) {
  0,  97, 255,  1,
  101,  47, 133,  1,
  178,  13, 43,  1,
  255,   2, 10,  1
};

DEFINE_GRADIENT_PALETTE( lava_gp ) {
  0,   0,  0,  0,
  46,  18,  0,  0,
  96, 113,  0,  0,
  108, 142,  3,  1,
  119, 175, 17,  1,
  146, 213, 44,  2,
  174, 255, 82,  4,
  188, 255, 115,  4,
  202, 255, 156,  4,
  218, 255, 203,  4,
  234, 255, 255,  4,
  244, 255, 255, 71,
  255, 255, 255, 255
};

DEFINE_GRADIENT_PALETTE( fire_gp ) {
  0,   1,  1,  0,
  76,  32,  5,  0,
  146, 192, 24,  0,
  197, 220, 105,  5,
  240, 252, 255, 31,
  250, 252, 255, 111,
  255, 255, 255, 255
};

DEFINE_GRADIENT_PALETTE( Magenta_Evening_gp ) {
  0,  71, 27, 39,
  31, 130, 11, 51,
  63, 213,  2, 64,
  70, 232,  1, 66,
  76, 252,  1, 69,
  108, 123,  2, 51,
  255,  46,  9, 35
};

DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
  0, 120,  0,  0,
  22, 179, 22,  0,
  51, 255, 104,  0,
  85, 167, 22, 18,
  135, 100,  0, 103,
  198,  16,  0, 130,
  255,   0,  0, 160
};

DEFINE_GRADIENT_PALETTE( BlacK_Magenta_Red_gp ) {
  0,   0,  0,  0,
  63,  42,  0, 45,
  127, 255,  0, 255,
  191, 255,  0, 45,
  255, 255,  0,  0
};


DEFINE_GRADIENT_PALETTE( BlacK_Red_Magenta_Yellow_gp ) {
  0,   0,  0,  0,
  42,  42,  0,  0,
  84, 255,  0,  0,
  127, 255,  0, 45,
  170, 255,  0, 255,
  212, 255, 55, 45,
  255, 255, 255,  0
};

DEFINE_GRADIENT_PALETTE( GREENBLUE ) {
  0,   0,  255,  0,
  120, 0, 255, 0,
  127, 0,  0,  255,
  135, 0, 255, 0,
  255, 0,  255,  0
};

const TProgmemRGBGradientPalettePtr gGradientPalettes[] = {
  oneWavePOP,                       //0
  twoWavePOP,                       //1
  threeWavePOP,                     //2
  oneWaveOPO,                       //3
  twoWaveOPO,                       //4
  threeWaveOPO,                     //5
  es_pinksplash_08_gp,              //6
  rainbowsherbet_gp,                //7
  gr65_hult_gp,                     //8
  es_emerald_dragon_08_gp,          //9
  lava_gp,                          //10
  fire_gp,                          //11
  Magenta_Evening_gp,               //12
  Sunset_Real_gp,                   //13
  BlacK_Magenta_Red_gp,             //14
  BlacK_Red_Magenta_Yellow_gp,       //15
  GREENBLUE                         //16
};    

const uint8_t gGradientPaletteCount =
  sizeof( gGradientPalettes) / sizeof( TProgmemRGBGradientPalettePtr );
