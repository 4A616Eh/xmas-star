#define DEBUG_ENABLE 0

#if DEBUG_ENABLE
#define DEBUGF(...)  Serial.printf(__VA_ARGS__)
#define DEBUGLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUGF(...)
#define DEBUGLN(...)
#endif

#include <Arduino.h>
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
#define LED_TYPE    WS2812B
#define COLOR_ORDER RGB
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
  uint8_t fadeCounter;       // 0–255 Fadewert
  uint8_t fadeMin;
  uint8_t fadeMax;
  CRGB targetColor;
  uint32_t holdMillis;       // Startzeit des Hold-States
  uint32_t holdDuration;     // Dauer des Halts (ms)
  bool debugPrinted;         // Debugausgabe pro State
};

LedState ledState[NUM_LEDS];

struct PaletteEntry {
  CRGB color;
  uint8_t maxBrightness;   // 0–255
};

const PaletteEntry christmasPalette[] = {
  { CRGB::Red, 255 }, { CRGB::Green, 120 },
  { CRGB(255, 215, 0), 200 },  // Gold
  { CRGB(255, 147, 41), 180 }, // Warm white
  { CRGB(180, 200, 255), 160 } // Cool white
};

const uint8_t christmasPaletteSize =
  sizeof(christmasPalette) / sizeof(christmasPalette[0]);

const PaletteEntry redGreenPalette[] = {
  { CRGB::Red, 240 }, { CRGB::Green, 220 }
};

const uint8_t redGreenPaletteSize =
  sizeof(redGreenPalette) / sizeof(redGreenPalette[0]);

#define BRIGHTNESS          20
#define FRAMES_PER_SECOND  120

void initLedStates() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ledState[i].state        = ST_INIT;
    ledState[i].debugPrinted = false;
  }
}

PaletteEntry getRandomPaletteEntry(const PaletteEntry* palette, uint8_t paletteSize) {
  return palette[random8(paletteSize)];
}

void setup() {
  //pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, HIGH);
  delay(3000); // 3 second delay for recovery
  //digitalWrite(LED_BUILTIN, LOW);

#if DEBUG_ENABLE
  Serial.begin(115200);
#endif

  DEBUGLN("ESP32C3 setup...");

  // Hardware-RNG
  uint16_t seed = (uint16_t)esp_random();
  random16_set_seed(seed);
  DEBUGF("Random Seed: %lu\n", seed);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  // initialize LED states
  initLedStates();

  DEBUGLN("setup done.");
}

// Forward declarations for pattern functions
void stupidTest();
void xmasAnimationPalette(const PaletteEntry* palette, uint8_t paletteSize);
void xmasRedGreen();
void xmasChristmasColors();
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
  //stupidTest();
  //return

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

void stupidTest() {
  // just a test pattern
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
  FastLED.show();
  delay(500);
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Green;
  }
  FastLED.show();
  delay(500);
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;
  }
  FastLED.show();
  delay(500);
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  delay(1500);
}

void xmasAnimationPalette(const PaletteEntry* palette, uint8_t paletteSize)
{
  for(int pos = 0; pos < NUM_LEDS; pos++)
  {
    LedState &ls = ledState[pos];
    switch (ls.state)
    {
    // ---------------------------------------------------------
    case ST_INIT:
      ls.waitCounter = random8(10, 40) * FRAMES_PER_SECOND/2;
      ls.state = ST_IDLE;
      ls.debugPrinted = false;
      break;
    // ---------------------------------------------------------
    case ST_IDLE:
      if (ls.waitCounter > 0) {
        ls.waitCounter--;
        break;
      }
      // einmalig Farbe aus Palette wählen
      {
        PaletteEntry entry = getRandomPaletteEntry(palette, paletteSize);
        ls.targetColor = entry.color;
        ls.fadeMax = entry.maxBrightness;
      }
      ls.fadeMin = 0;
      ls.fadeCounter = ls.fadeMin;
      ls.state = ST_FADEIN;
      ls.debugPrinted = false;
      break;
    // ---------------------------------------------------------
    case ST_FADEIN:
      if (!ls.debugPrinted) {
        DEBUGF("LED %d -> ST_FADEIN\n", pos);
        ls.debugPrinted = true;
      }
      leds[pos] = ls.targetColor;
      leds[pos].nscale8(ls.fadeCounter);
      if (++ls.fadeCounter >= ls.fadeMax) {
        ls.state = ST_HOLD;
        ls.holdMillis = millis();
        ls.debugPrinted = false;
      }
      break;
    // ---------------------------------------------------------
    case ST_HOLD:
      if (!ls.debugPrinted) {
        // einmalig Hold-Dauer zufällig festlegen
        ls.holdDuration = random16(200, 600);
        DEBUGF("LED %d -> ST_HOLD (%d ms)\n", pos, ls.holdDuration);
        ls.debugPrinted = true;
      }
      if (millis() - ls.holdMillis >= ls.holdDuration) {
        ls.fadeCounter = ls.fadeMax;
        ls.state = ST_FADEOUT;
        ls.debugPrinted = false;
      }
      break;
    // ---------------------------------------------------------
    case ST_FADEOUT:
      if (!ls.debugPrinted) {
        DEBUGF("LED %d -> ST_FADEOUT\n", pos);
        ls.debugPrinted = true;
      }
      leds[pos] = ls.targetColor;
      leds[pos].nscale8(ls.fadeCounter);
      if (ls.fadeCounter > 0) {
        ls.fadeCounter--;
      } else {
        ls.state = ST_INIT;
        ls.debugPrinted = false;
      }
      break;
    }
  }
}

void xmasRedGreen() 
{
  xmasAnimationPalette(redGreenPalette, redGreenPaletteSize);
}

void xmasChristmasColors()
{
  xmasAnimationPalette(christmasPalette, christmasPaletteSize);
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
