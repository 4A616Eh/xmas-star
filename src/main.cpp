#if 0
#include <Arduino.h>

#define LED 8

void setup() {
    Serial.begin(115200);
    Serial.println("Hello World");
    pinMode(LED, OUTPUT);
}

void loop() {
    
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
}

#endif

/// @file    DemoReel100.ino
/// @brief   FastLED "100 lines of code" demo reel, showing off some effects
/// @example DemoReel100.ino

#include <FastLED.h>

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014


#define DATA_PIN    10
//#define CLK_PIN   4
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    5
CRGB leds[NUM_LEDS];

enum LedStateEnum {
  ST_INIT,
  ST_IDLE,
  ST_FADEIN,
  ST_HOLD,
  ST_FADEOUT
};

struct LedState {
  LedStateEnum state;
  uint16_t waitCounter;      // Wartezeit bis Start
  uint8_t  fadeCounter;      // 0–255 Fadewert
  uint8_t  fadeMin;
  uint8_t  fadeMax;
  CRGB     targetColor;
  uint32_t holdMillis;
  bool debugPrinted;         // verhindert Spam im Serial
};

LedState ledState[NUM_LEDS];

#define BRIGHTNESS          20
#define FRAMES_PER_SECOND  120

void initLedStates() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ledState[i].state        = ST_INIT;
    ledState[i].debugPrinted = false;
  }
}

void setup() {
  delay(3000); // 3 second delay for recovery
  
  Serial.begin(115200);
  Serial.println("ESP32C3 setup...");

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  // initialize LED states
  initLedStates();

  Serial.println("setup done.");
}

// Forward declarations for pattern functions
void xmasRedGreen();
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();
void nextPattern();
void addGlitter(fract8 chanceOfGlitter);

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

SimplePatternList gPatterns = { xmasRedGreen };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void xmasRedGreen() 
{
  for(int pos = 0; pos < NUM_LEDS; pos++)
  {
    LedState &ls = ledState[pos];
    switch (ls.state)
    {
    // ---------------------------------------------------------
    case ST_INIT:
      ls.waitCounter = random8(10, 50) * FRAMES_PER_SECOND/2;
      ls.state = ST_IDLE;
      ls.debugPrinted = false;
      Serial.printf("LED %d -> ST_IDLE (wait=%d)\n", pos, ls.waitCounter);
      break;
    // ---------------------------------------------------------
    case ST_IDLE:
      if (ls.waitCounter > 0) {
        ls.waitCounter--;
        break;
      }
      // Zeit zum starten: Farbe wählen
      if(random8() & 1) {
        ls.targetColor = CRGB::Red;
        ls.fadeMax = 255;
      } else {
        ls.targetColor = CRGB::Green;
        ls.fadeMax = 185; // Grünton etwas dunkler, damit er etwa gleich hell wie rot wirkt
      }
      ls.fadeMin     = 0;
      ls.fadeCounter = ls.fadeMin;
      ls.state = ST_FADEIN;
      ls.debugPrinted = false;
      Serial.printf("LED %d -> ST_FADEIN (max=%d)\n", pos, ls.fadeMax);
      break;
    // ---------------------------------------------------------
    case ST_FADEIN:
      if (!ls.debugPrinted) {
        Serial.printf("LED %d start FADEIN\n", pos);
        ls.debugPrinted = true;
      }
      leds[pos] = ls.targetColor;
      leds[pos].nscale8(ls.fadeCounter);
      if (++ls.fadeCounter >= ls.fadeMax) {
        ls.state = ST_HOLD;
        ls.holdMillis = millis();
        ls.debugPrinted = false;
        Serial.printf("LED %d -> ST_HOLD\n", pos);
      }
      break;
    // ---------------------------------------------------------
    case ST_HOLD:
      if (!ls.debugPrinted) {
        Serial.printf("LED %d holding\n", pos);
        ls.debugPrinted = true;
      }
      // halte 200–600 ms
      if (millis() - ls.holdMillis > random16(200, 600)) {
        ls.fadeCounter = ls.fadeMax;
        ls.state = ST_FADEOUT;
        ls.debugPrinted = false;
        Serial.printf("LED %d -> ST_FADEOUT\n", pos);
      }
      break;
    // ---------------------------------------------------------
    case ST_FADEOUT:
      if (!ls.debugPrinted) {
        Serial.printf("LED %d start FADEOUT\n", pos);
        ls.debugPrinted = true;
      }
      leds[pos] = ls.targetColor;
      leds[pos].nscale8(ls.fadeCounter);
      if (ls.fadeCounter > 0)
        ls.fadeCounter--;
      else {
        ls.state = ST_INIT;  // neu starten
        ls.debugPrinted = false;
        Serial.printf("LED %d -> ST_INIT\n", pos);
      }
      break;
    }
  }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
