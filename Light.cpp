/*
 * Light.cpp
 *
 * author: 		Sebastien CAPOU (neskweek@gmail.com) and Andras Kun (kun.andras@yahoo.de)
 * Source :  https://github.com/Protonerd/FX-SaberOS
 */
#include "Light.h"
#include "ConfigMenu.h"
#include "Soundfont.h"

// global Saber state and Sub State variables
extern SaberStateEnum SaberState;
extern SaberStateEnum PrevSaberState;
extern ActionModeSubStatesEnum ActionModeSubStates;
extern ConfigModeSubStatesEnum ConfigModeSubStates;
extern ActionModeSubStatesEnum PrevActionModeSubStates;
extern ConfigModeSubStatesEnum PrevConfigModeSubStates;
//extern SubStateEnum SubState;

extern bool lockuponclash;
extern bool tipmeltonclash;
extern long tipmeltStart;
extern int8_t modification;

extern struct StoreStruct {
  // This is for mere detection if they are our settings
  char version[5];
  // The settings
  uint8_t volume;// 0 to 31
  uint8_t soundFont;// as many as Sound font you have defined in Soundfont.h Max:253
  struct Profile {
    uint32_t mainColor;
    uint32_t clashColor;
    uint32_t blasterboltColor;
    uint16_t swingSensitivity;
    uint8_t flickerType;
    uint8_t poweronoffType;
  }sndProfile[SOUNDFONT_QUANTITY];
}storage;

extern SoundFont soundFont;
extern uint8_t ledPins[];
#if defined ACCENT_LED or defined PIXEL_ACCENT
  unsigned long lastAccent = millis();
#endif

#ifdef JUKEBOX
#define SAMPLESIZEAVERAGE 30
#endif

const uint8_t TIP_PIXELS=25; // the number of pixels to animate in tipmelt mode
bool fireblade=false;

#ifdef PIXELBLADE 
  Adafruit_NeoPixel pixels(NUMPIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);
  // FIREBLADE
  #ifdef ANIBLADE
    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 50, suggested range 20-100 
    static uint8_t Fire_Cooling = 100;//50;
    
    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    static uint8_t Fire_Sparking = 50;//100;
    //#ifdef CROSSGUARDSABER
    //static byte heat[MN_STRIPE];  
    //static byte heat_cg[CG_STRIPE];
    //#else
    //#endif
    #define PIXELSTEP 2 // how many pixel to treat as a group to save on processing capability
    static byte heat[NUMPIXELS/PIXELSTEP];
  #endif // ANIBLADE
#endif  // PIXELBLADE


/* ***************** UNIFIED LIGHT FUNCTIONS *********************/

#define I_BEGINNEXTSEGMENT 50
#define R_BEGINNEXTSEGMENT 100
#define PULSEFLICKERDEPTH 100
#define PULSEDURATION 500


uint8_t pulseflicker_pwm=0;
bool pulsedir=true;
static uint8_t flickerPos = 0;
static long lastFlicker = millis();

#ifdef PIXEL_ACCENT
  Adafruit_NeoPixel accentPixels(NUMPIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);
#endif

#if defined STAR_LED or defined PIXELBLADE or defined ADF_PIXIE_BLADE
extern uint32_t currentColor;
#endif

#ifdef ADF_PIXIE_BLADE
  SoftwareSerial pixieSerial(-1, PIXIEPIN);
  Adafruit_Pixie strip = Adafruit_Pixie(NUMPIXELS, &pixieSerial);
#endif

#ifdef COLOR_PROFILE
// define an array for the 15 color profiles
uint32_t colorProfiles[15];
//colorProfiles[0].r=255;
#endif
// ====================================================================================
// ===              	    			LED FUNCTIONS		                		===
// ====================================================================================

/* ***************** UNIFIED LIGHT FUNCTIONS *********************/
void lightOn(uint8_t ledPins[], int8_t segment, uint32_t color, int8_t StartPixel, int8_t StopPixel) {
  // Light On
  #if defined LEDSTRINGS
    if (segment == -1) {
      for (uint8_t i = 0; i < 6; i++) {
        digitalWrite(ledPins[i], HIGH);
      }
    } else {
      digitalWrite(ledPins[segment], HIGH);
    }
  #endif
  #if defined STAR_LED
  // Light On
      analogWrite(ledPins[0], color.r); // RED
      analogWrite(ledPins[1], color.g); // GREEN
      analogWrite(ledPins[2], color.b); // BLUE
  #endif

  #ifdef ADF_PIXIE_BLADE
    for(uint8_t i=0; i< NUMPIXELS; i++) {
      strip.setPixelColor(i, color.r, color.g, color.b);
    }
    strip.show();
  #endif
  
  #ifdef PIXELBLADE
    // Light On
    if (StartPixel == -1 or StopPixel==-1 or StopPixel<StartPixel or StartPixel>NUMPIXELS or StopPixel>NUMPIXELS) {  // if neither start nor stop is defined or invalid range, go through the whole stripe
      for (uint8_t i = 0; i <= NUMPIXELS; i++) {
        pixels.setPixelColor(i, color);
      }
    } else {
      for (uint8_t i = StartPixel; i <= StopPixel; i++) {
        pixels.setPixelColor(i, color);
      }
    }
    pixels.show();
  #endif
} //lightOn

void lightOff(uint8_t ledPins[], int8_t segment, int8_t StartPixel, int8_t StopPixel) {
  #if defined LEDSTRINGS
    // shut Off
    //Shut down PWM
    TCCR0A &= ~((1 << COM0A1) | (1 << COM0B1));
    TCCR1A &= ~((1 << COM1A1) | (1 << COM1B1));
    TCCR2A &= ~((1 << COM2A1) | (1 << COM2B1));
    //Shut down everything at once
    PORTB &= B11010001;
    PORTD &= B10010111;
  #endif
  
  #if defined STAR_LED
    // shut Off
    digitalWrite(LED_RED, LOW); // RED
    digitalWrite(LED_GREEN, LOW); // GREEN
    digitalWrite(LED_BLUE, LOW); // BLUE
  #endif

  #ifdef ADF_PIXIE_BLADE
    for(uint8_t i=0; i< NUMPIXELS; i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();
  #endif
  
  #ifdef PIXELBLADE
    // shut Off
    uint32_t value = pixels.Color(0, 0, 0);
    if (StartPixel == -1 or StopPixel==-1 or StopPixel<StartPixel or StartPixel>NUMPIXELS or StopPixel>NUMPIXELS) {  // if neither start nor stop is defined or invalid range, go through the whole stripe
      for (uint16_t i = 0; i <= NUMPIXELS; i++) {
        pixels.setPixelColor(i, value);
      }
    }
    else {
      for (uint8_t i = StartPixel; i <= StopPixel; i++) {
        pixels.setPixelColor(i, value);
      }
    }
    pixels.show();
  #endif

} //lightOff

void getColor(uint32_t color) {
  #if defined LEDSTRINGS
  
  #endif
  
  #if defined STAR_LED
    currentColor.r = color.r;
    currentColor.g = color.g;
    currentColor.b = color.b;
  #endif
  
  #ifdef ADF_PIXIE_BLADE
    currentColor.r = color.r;
    currentColor.g = color.g;
    currentColor.b = color.b;
  #endif
    
  #ifdef PIXELBLADE
    currentColor = color;
  #endif  
} // getColor

void RampBlade(uint16_t RampDuration, bool DirectionUpDown, int8_t StartPixel, int8_t StopPixel) {
  #if defined LEDSTRINGS
    // Nothing
  #endif
  #if defined STAR_LED
    // Nothing
  #endif
  #ifdef ADF_PIXIE_BLADE
    // Nothing
  #endif
  
  #ifdef PIXELBLADE
    // The start of the ramp function
    unsigned long ignitionStart = millis();
    // Create a new variable value to store the color we're choosing (starts as black)
    uint32_t value = pixels.Color(0,0,0);


    // If either start nor stop is undefined or invalid range, go through the whole stripe    
    if (StartPixel == -1 or StopPixel==-1 or StopPixel<StartPixel or StartPixel>NUMPIXELS or StopPixel>NUMPIXELS) {  // neopixel ramp code from jbkuma
      StartPixel=0;
      StopPixel= NUMPIXELS; 
    }

    if (fireblade) { // #ifdef FIREBLADE
      #ifdef ANIBLADE
        for (unsigned int i=StartPixel; i<StopPixel; (i=i+5)) { // turn on/off one LED at a time
          FireBlade(storage.sndProfile[storage.soundFont].flickerType-2);
          for(unsigned int j=0; j<StopPixel; j++ ) { // fill up string with data
            if ((DirectionUpDown and j<=i) or (!DirectionUpDown and j<=StopPixel-1-i)){
              }
              else if ((DirectionUpDown and j>i) or (!DirectionUpDown and j>StopPixel-1-i)){
                //heat[j]=0;
                pixels.setPixelColor(j, value); // Set value at LED found at index j
              }      
            }
            pixels.show(); // Sends the data to the LEDs
          }    
        #endif // ANIBLADE
    } // fireblade
    else {
      // Loop from startpixel to stoppixel, turning on/off the number of LEDs that match ramp timing
      for (unsigned int i = StartPixel; i < StopPixel; i = StopPixel*(millis()-ignitionStart)/RampDuration) {
        // Generate a flicker effect between 65% and 115% of MAX_BRIGHTNESS, with a 1 in 115 chance of flicking to 0
        int flickFactor = random(0,115);
        if (flickFactor < 65 && flickFactor > 0) { flickFactor = 100; } 

        // Fill up string with correct data
        for(uint8_t  j=StartPixel; j<=StopPixel; j++ ) {
          value = pixels.Color(0,0,0);

          if ((DirectionUpDown and j<=i)){
            uint8_t r, g, b;
            UnpackColor(currentColor, r, g, b);
            value = pixels.Color(
              MAX_BRIGHTNESS * i / NUMPIXELS * r / rgbFactor * flickFactor / 100,
              MAX_BRIGHTNESS * i / NUMPIXELS * g / rgbFactor * flickFactor / 100,
              MAX_BRIGHTNESS * i / NUMPIXELS * b / rgbFactor * flickFactor / 100
            );
          }
          else if (!DirectionUpDown and j<=NUMPIXELS-1-i){
            uint8_t r, g, b;
            UnpackColor(currentColor, r, g, b);
            value = pixels.Color(
              MAX_BRIGHTNESS * (NUMPIXELS - i) / NUMPIXELS * r / rgbFactor * flickFactor / 100,
              MAX_BRIGHTNESS * (NUMPIXELS - i) / NUMPIXELS * g / rgbFactor * flickFactor / 100,
              MAX_BRIGHTNESS * (NUMPIXELS - i) / NUMPIXELS * b / rgbFactor * flickFactor / 100
            );
          }
          else if ((DirectionUpDown and j>i) or (!DirectionUpDown and j>NUMPIXELS-1-i)){
            // Do nothing, leave black
          }
          pixels.setPixelColor(j, value);
        }
        // Send the data to the LEDs
        pixels.show();
        // Match the ramp duration to the number of pixels in the string
        delay(RampDuration/(StopPixel-StartPixel));
      }
    } // #endif
  #endif  
} // RampBlade

void lightIgnition(uint8_t ledPins[], uint16_t time, uint8_t type, uint32_t color, int8_t StartPixel, int8_t StopPixel) {
  #if defined LEDSTRINGS
  
    uint8_t LS_Status[6];
    bool ongoing=true;

    switch (type) {
      default:
      case 0:
      for (uint8_t i=0; i<6; i++) {
        LS_Status[i]=0;
      }
      while (ongoing) {  // do the loops as long the variable is set to false, when the last segment finsihed the ramp
        for (uint8_t i = 0; i < 6; i++) {
          analogWrite(ledPins[i], LS_Status[i]);
          if (i==0 and LS_Status[i]<255) {
            LS_Status[i]++;
          }
          else if (i>0 and LS_Status[i-1]>=I_BEGINNEXTSEGMENT and LS_Status[i]<255) {
            LS_Status[i]++;
          }
          if (LS_Status[5]==255) {
            ongoing=false;
          }
        }
        delayMicroseconds(time * (1000/(5*I_BEGINNEXTSEGMENT+255)));
      }
      // ramp down to MAX_BRIGHTNESS
      for (uint8_t j = 255; j >= MAX_BRIGHTNESS; j--) {
        for (uint8_t i = 0; i < 6; i++) {
          analogWrite(ledPins[i], j);
        }
        delay(3);
      }
      /*
      // Light up the ledstrings Movie-like
      for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j=0; j<=MAX_BRIGHTNESS;j+=10) {
          analogWrite(ledPins[i], j);
          delay(time / (6*25));
        }
        //delay(time / (5*10));
      }
      */
      break;
      case 1:
      for (int8_t i = 5; i >= 0; i--) {
        for (uint8_t j = 0; j <= i; j++) {
          if (j > 0) {
            digitalWrite(ledPins[j - 1], LOW);
          }
          digitalWrite(ledPins[j], HIGH);
          delay(time / 20);
        }
      }
      break;
    }
  #endif // LEDSTRINGS
  
  #if defined STAR_LED
    // Fade in to Maximum brightness
     for (uint8_t fadeIn = 0; fadeIn < 255; fadeIn++) {
      analogWrite(LED_RED, color.r * fadeIn / 255); // RED
      analogWrite(LED_GREEN, color.g * fadeIn / 255); // GREEN
      analogWrite(LED_BLUE, color.b * fadeIn / 255); // BLUE
      delay(time / 255);
    }
  #endif // STAR

  #ifdef ADF_PIXIE_BLADE
    // Fade in to Maximum brightness
    for (uint8_t fadeIn = 0; fadeIn < 255; fadeIn++) {
      for(uint8_t i=0; i< NUMPIXELS; i++) {        
        strip.setPixelColor(i, color.r * fadeIn / 255, color.g * fadeIn / 255, color.b * fadeIn / 255);
      }
    strip.show();
    delay(time / 255);
    }
  #endif // ADF_PIXIE
  
  #ifdef PIXELBLADE
    // Declaring a new variable to hold a color
    uint32_t value = 0;
    // If the start or stop pixels are invalid, go through the whole strip
    if (StartPixel == -1 or StopPixel == -1 or StopPixel < StartPixel or StartPixel > NUMPIXELS or StopPixel > NUMPIXELS) { // neopixel ramp code from jbkuma
      StartPixel=0;
      StopPixel= NUMPIXELS; 
    }

    // Unpack the currentColor of the saber and modify it based on the MAX_BRIGHTNESS and rgbFactor
    uint8_t r, g, b;
    UnpackColor(currentColor, r, g, b);
    value = pixels.Color(
      MAX_BRIGHTNESS * r / rgbFactor,
      MAX_BRIGHTNESS * g / rgbFactor,
      MAX_BRIGHTNESS * b / rgbFactor
    );
    // After this, value is now the current color modified by MAX_BRIGHTNESS and rgbFactor

    // Now go and light up the LEDs 1x1, movie-like
    RampBlade(time, true, StartPixel, StopPixel);
  #endif
} // lightIgnition

void lightRetract(uint8_t ledPins[], uint16_t time, uint8_t type,uint32_t color, int8_t StartPixel, int8_t StopPixel) {
  #if defined LEDSTRINGS

    uint8_t LS_Status[6];
    bool ongoing=true;

    switch (type) {
      default:
      case 0:
      // Light off the ledstrings Movie Like
       for (uint8_t i=0; i<6; i++) {
        LS_Status[i]=MAX_BRIGHTNESS;
       }
       while (ongoing) {  // do the loops as long the variable is set to false, when the last segment finsihed the ramp
        for (uint8_t i = 0; i < 6; i++) {
          if (i==5 and LS_Status[i]>0) {
            LS_Status[i]--;
          }
          else if (i<5 and LS_Status[i+1]<=R_BEGINNEXTSEGMENT and LS_Status[i]>0) {
            LS_Status[i]--;
          }
          if (LS_Status[0]==0) {
            ongoing=false;
          }
          analogWrite(ledPins[i], LS_Status[i]);
        }
        delayMicroseconds(time * (1000/(5*(MAX_BRIGHTNESS-R_BEGINNEXTSEGMENT)+MAX_BRIGHTNESS)));
       }
      break;
      case 1:
    // Light off the ledstrings invert
        for (int8_t i = 5; i >= 0; i--) {
          for (uint8_t j = 0; j <= i; j++) {
            if (j > 0) {
              digitalWrite(ledPins[j - 1], HIGH);
            }
            digitalWrite(ledPins[j], LOW);
            delay(time / 20);
          }
        }
        break;
      }
  #endif
  
  #if defined STAR_LED
    // Fade in to Maximum brightness
     for (uint8_t fadeIn = 255; fadeIn > 0; fadeIn--) {
      analogWrite(LED_RED, color.r * fadeIn / 255); // RED
      analogWrite(LED_GREEN, color.g * fadeIn / 255); // GREEN
      analogWrite(LED_BLUE, color.b * fadeIn / 255); // BLUE
      delay(time / 255);
    }
  #endif

  #ifdef ADF_PIXIE_BLADE
    // Fade in to Maximum brightness
    for (uint8_t fadeIn = 255; fadeIn > 0; fadeIn--) {
      for(uint8_t i=0; i< NUMPIXELS; i++) {        
        strip.setPixelColor(i, color.r * fadeIn / 255, color.g * fadeIn / 255, color.b * fadeIn / 255);
      }
    strip.show();
    delay(time / 255);
    }
  #endif
  
  #ifdef PIXELBLADE
    // Light off the ledstrings Movie Like
    uint32_t value = 0;
    if (StartPixel == -1 or StopPixel==-1 or StopPixel<StartPixel or StartPixel>NUMPIXELS or StopPixel>NUMPIXELS) {  // if neither start nor stop is defined or invalid range, go through the whole stripe    // neopixel ramp code from jbkuma
      StartPixel=0;
      StopPixel= NUMPIXELS; 
    }
    RampBlade(time, false, StartPixel, StopPixel);
    if (fireblade) { // #ifdef FIREBLADE
      #ifdef ANIBLADE
        for(unsigned int j=0; j<sizeof(heat); j++ ) { // clear the heat static variables
          heat[j]=0;
        }  
      #endif // ANIBLADE
    }
  #endif  
} // lightRetract

void lightFlicker(uint8_t ledPins[], uint8_t type, uint8_t value, uint32_t maincolor, uint32_t clashcolor,uint8_t AState, int8_t StartPixel, int8_t StopPixel) {
    uint8_t brightness;

  #if defined LEDSTRINGS

  
    switch (type) {
      default:
      case 0:
      // // AudioTracker Flickering
     brightness = constrain(MAX_BRIGHTNESS
      - (abs(analogRead(SPK1) - analogRead(SPK2)))*31/storage.volume,0,255);
      for (uint8_t i = 0; i <= 5; i++) {
        analogWrite(ledPins[i], brightness);
      }
      break;
      case 1:
      // anarchic Flickering
      brightness = constrain(MAX_BRIGHTNESS
      - random(FLICKERDEPTH),0,255);
      for (uint8_t i = 0; i <= 5; i++) {
        if (i != flickerPos)
        analogWrite(ledPins[i], brightness);
        else
        analogWrite(ledPins[i], MAX_BRIGHTNESS);
      }
      if ((flickerPos != 0
          and millis() - lastFlicker > (120 - (100 - 15 * flickerPos)))
          or (flickerPos == 0 and millis() - lastFlicker > 300)) {
        flickerPos++;
        lastFlicker = millis();
        if (flickerPos == 6) {
          flickerPos = 0;
        }
      }
      break;
      case 2:
      // pulse Flickering
      if (((millis()-lastFlicker>=PULSEDURATION/PULSEFLICKERDEPTH) and AState != AS_BLADELOCKUP) or ((millis()-lastFlicker>=2) and AState == AS_BLADELOCKUP)) {
        lastFlicker=millis();
        for (uint8_t i = 0; i <= 5; i++) {
          analogWrite(ledPins[i],MAX_BRIGHTNESS - pulseflicker_pwm);
        }
        if (pulsedir) {
          pulseflicker_pwm++;
        }
        else {
          pulseflicker_pwm--;
        }
        if (pulseflicker_pwm == PULSEFLICKERDEPTH) { 
          pulsedir=false;
        }
        else if (pulseflicker_pwm == 0) {
          pulsedir=true;
        }
      }
      break;
    }
  #endif
  
  #if defined STAR_LED

  switch (type) {
    default:
    case 0: // AudioTracker Flickering
      brightness = constrain(MAX_BRIGHTNESS
      - (abs(analogRead(SPK1) - analogRead(SPK2)))*31/storage.volume,0,255);        
      if (AState==AS_BLADELOCKUP) { //animate blade in lockup mode
          // gives 25% chance to flick larger range for better randomization
          int lockupFlick = random(0,39); 
          if (lockupFlick < 25) {
            analogWrite(LED_RED, (brightness * maincolor.r / rgbFactor)); // RED
            analogWrite(LED_GREEN, (brightness * maincolor.g / rgbFactor)); // GREEN
            analogWrite(LED_BLUE, (brightness * maincolor.b / rgbFactor)); // BLUE  
          } else if (lockupFlick < 35) {
            analogWrite(LED_RED, (brightness * clashcolor.r / rgbFactor)); // RED
            analogWrite(LED_GREEN, (brightness * clashcolor.g / rgbFactor)); // GREEN
            analogWrite(LED_BLUE, (brightness * clashcolor.b / rgbFactor)); // BLUE  
          }
          else  { // simple white
            analogWrite(LED_RED, MAX_BRIGHTNESS); // RED
            analogWrite(LED_GREEN, MAX_BRIGHTNESS); // GREEN
            analogWrite(LED_BLUE, MAX_BRIGHTNESS); // BLUE  
          }
        }
        else if (AState==AS_CLASH) {
          analogWrite(LED_RED, (brightness * clashcolor.r / rgbFactor)); // RED
          analogWrite(LED_GREEN, (brightness * clashcolor.g / rgbFactor)); // GREEN
          analogWrite(LED_BLUE, (brightness * clashcolor.b / rgbFactor)); // BLUE            
        }
        else {
          analogWrite(LED_RED, (brightness * maincolor.r / rgbFactor)); // RED
          analogWrite(LED_GREEN, (brightness * maincolor.g / rgbFactor)); // GREEN
          analogWrite(LED_BLUE, (brightness * maincolor.b / rgbFactor)); // BLUE  
        }
      break;
    case 1: // pulse flickering
      if (((millis()-lastFlicker>=PULSEDURATION/PULSEFLICKERDEPTH) and AState != AS_BLADELOCKUP) or ((millis()-lastFlicker>=2) and AState == AS_BLADELOCKUP)) {
        lastFlicker=millis();
        if (AState==AS_CLASH) {
          analogWrite(LED_RED, ((MAX_BRIGHTNESS - pulseflicker_pwm) * clashcolor.r / rgbFactor)); // RED
          analogWrite(LED_GREEN, ((MAX_BRIGHTNESS - pulseflicker_pwm) * clashcolor.g / rgbFactor)); // GREEN
          analogWrite(LED_BLUE, ((MAX_BRIGHTNESS - pulseflicker_pwm) * clashcolor.b / rgbFactor)); // BLUE  
        }
        else {
          analogWrite(LED_RED, ((MAX_BRIGHTNESS - pulseflicker_pwm) * maincolor.r / rgbFactor)); // RED
          analogWrite(LED_GREEN, ((MAX_BRIGHTNESS - pulseflicker_pwm) * maincolor.g / rgbFactor)); // GREEN
          analogWrite(LED_BLUE, ((MAX_BRIGHTNESS - pulseflicker_pwm) * maincolor.b / rgbFactor)); // BLUE         
        }
        if (pulsedir) {
          pulseflicker_pwm++;
        }
        else {
          pulseflicker_pwm--;
        }
        if (pulseflicker_pwm == PULSEFLICKERDEPTH) { 
          pulsedir=false;
        }
        else if (pulseflicker_pwm == 0) {
          pulsedir=true;
        }
      }
      break;
    case 2: // static blade
    if (AState==AS_CLASH) {
      analogWrite(LED_RED, clashcolor.r ); // RED
      analogWrite(LED_GREEN, clashcolor.g); // GREEN
      analogWrite(LED_BLUE, clashcolor.b); // BLUE 
    }
    else {
      analogWrite(LED_RED, maincolor.r ); // RED
      analogWrite(LED_GREEN, maincolor.g); // GREEN
      analogWrite(LED_BLUE, maincolor.b); // BLUE        
    }
      break;      
  }
  #endif

  #ifdef ADF_PIXIE_BLADE
      for(uint8_t i=0; i< NUMPIXELS; i++) {
        Serial.print("\t");Serial.print(brightness);Serial.print("\t");Serial.print(maincolor.g);Serial.print("\t");Serial.println((brightness * maincolor.r) / rgbFactor);
        //strip.setPixelColor(i, ((brightness * maincolor.r) / rgbFactor), ((brightness * maincolor.r) / rgbFactor), ((brightness * maincolor.r) / rgbFactor));//maincolor.r, maincolor.g, maincolor.b);
        strip.setPixelColor(i, maincolor.r,maincolor.g, maincolor.b);//maincolor.r, maincolor.g, maincolor.b);
      }
      strip.show();
  #endif
  
  #ifdef PIXELBLADE
      if (StartPixel == -1 or StopPixel==-1 or StopPixel<StartPixel or StartPixel>NUMPIXELS or StopPixel>NUMPIXELS) {  // if neither start nor stop is defined or invalid range, go through the whole stripe    // neopixel ramp code from jbkuma
        StartPixel=0;
        #ifdef TIP_MELT        
        if (AState==AS_TIPMELT)
          StopPixel=NUMPIXELS-TIP_PIXELS;
        else
        #endif
          StopPixel=NUMPIXELS; 
      }
      int flickFactor = random(0,255);
      if (flickFactor > 3 && flickFactor < 170) { flickFactor = 255; }
      //brightness = 255 * flickFactor / 100;
      brightness = flickFactor;
      uint32_t color;

      switch (type) {
        default:
        case 0:
        // use random generated values instead of AudioTracker values for neopixel to reduce loop time i.e. improve motion sensitivity and reaction
        brightness = constrain(MAX_BRIGHTNESS
        - random(FLICKERDEPTH),0,255);        // AudioTracker Flickering
        if (AState==AS_BLADELOCKUP) { //animate blade in lockup mode
          // gives 25% chance to flick larger range for better randomization
          int lockupFlick = random(0,39); 
          if (lockupFlick < 20) {
            uint8_t r,g,b;
            UnpackColor(maincolor, r, g, b);
            color = pixels.Color(
              brightness * r / rgbFactor,
              brightness * g / rgbFactor,
              brightness * b / rgbFactor
            );
          } else if (lockupFlick < 30) {
            uint8_t r,g,b;
            UnpackColor(clashcolor, r, g, b);
            color = pixels.Color(
              brightness * r / rgbFactor,
              brightness * g / rgbFactor,
              brightness * b / rgbFactor
            );
          }
          else  { // simple white
            color = pixels.Color(255,255,255);
          }
        }
        else if (AState==AS_CLASH) {
          uint8_t r,g,b;
          UnpackColor(clashcolor, r, g, b);
          color = pixels.Color(
            brightness * r / rgbFactor,
            brightness * g / rgbFactor,
            brightness * b / rgbFactor
          );       
        }
        else {  //normal operation
            uint8_t r,g,b;
            UnpackColor(maincolor, r, g, b);
            color = pixels.Color(
              brightness * r / rgbFactor,
              brightness * g / rgbFactor,
              brightness * b / rgbFactor
            );
        }
      
        for (uint16_t i = StartPixel; i <= StopPixel; i++) {
            pixels.setPixelColor(i, color); 
        }
        pixels.show();
        break;
      case 1:
        // Pulse flickering
      // pulse Flickering
        if (((millis()-lastFlicker>=PULSEDURATION/PULSEFLICKERDEPTH) and AState != AS_BLADELOCKUP) or (AState == AS_BLADELOCKUP)) {
          lastFlicker=millis();
          if (AState==AS_CLASH) {
            uint8_t r,g,b;
            UnpackColor(clashcolor, r, g, b);
            color = pixels.Color(
              (MAX_BRIGHTNESS - pulseflicker_pwm) * r / rgbFactor,
              (MAX_BRIGHTNESS - pulseflicker_pwm) * g / rgbFactor,
              (MAX_BRIGHTNESS - pulseflicker_pwm) * b / rgbFactor
            );        
          }
          else {
            uint8_t r,g,b;
            UnpackColor(maincolor, r, g, b);
            color = pixels.Color(
              (MAX_BRIGHTNESS - pulseflicker_pwm) * r / rgbFactor,
              (MAX_BRIGHTNESS - pulseflicker_pwm) * g / rgbFactor,
              (MAX_BRIGHTNESS - pulseflicker_pwm) * b / rgbFactor
            );   
          }
          if (pulsedir) {
            pulseflicker_pwm++;
          }
          else {
            pulseflicker_pwm--;
          }
          if (pulseflicker_pwm == PULSEFLICKERDEPTH) { 
            pulsedir=false;
          }
          else if (pulseflicker_pwm == 0) {
            pulsedir=true;
          }
           for (uint16_t i = StartPixel; i <= StopPixel; i++) {
              pixels.setPixelColor(i, color); 
          }
  
          pixels.show();
        }

          break;
      case 2: // fire blade red
        if (fireblade) { // #ifdef FIREBLADE
          #ifdef ANIBLADE
          if (AState==AS_BLADELOCKUP or AState==AS_TIPMELT) {
            Fire_Cooling=150;
            Fire_Sparking=50;
          }
          else {
            Fire_Cooling=50;
            Fire_Sparking=100;  
          }
            FireBlade(0);
            pixels.show(); // Sends the data to the LEDs
            #endif // ANIBLADE
        }
        break;
      case 3: // fire blade green
        if (fireblade) { // #ifdef FIREBLADE
          #ifdef ANIBLADE
          if (AState==AS_BLADELOCKUP or AState==AS_TIPMELT) {
            Fire_Cooling=200;
            Fire_Sparking=70;
          }
          else {
            Fire_Cooling=100;
            Fire_Sparking=150;  
          }
            FireBlade(1);
            pixels.show(); // Sends the data to the LEDs
            #endif // ANIBLADE
        }
        break;
      case 4: // fire blade blue
        if (fireblade) { // #ifdef FIREBLADE
          #ifdef ANIBLADE
        
          if (AState==AS_BLADELOCKUP or AState==AS_TIPMELT) {
            Fire_Cooling=100;
            Fire_Sparking=20;
          }
          else {
            Fire_Cooling=50;
            Fire_Sparking=100;  
          }
            FireBlade(2);
            pixels.show(); // Sends the data to the LEDs
            #endif // ANIBLADE

        }
        break;
      case 5: // spark blade
        // use random generated values instead of AudioTracker values for neopixel to reduce loop time i.e. improve motion sensitivity and reaction
        //brightness = constrain(MAX_BRIGHTNESS
        //- (abs(analogRead(SPK1) - analogRead(SPK2)))*31/storage.volume/8,0,255); 
        brightness = constrain(MAX_BRIGHTNESS
        - random(FLICKERDEPTH),0,255);        // AudioTracker Flickering
        uint8_t r,g,b;
        UnpackColor(maincolor, r, g, b);
        color = pixels.Color(
          brightness * r / rgbFactor,
          brightness * g / rgbFactor,
          brightness * b / rgbFactor
        );   
        for (uint16_t i = StartPixel; i <= StopPixel; i++) {
            pixels.setPixelColor(i, color); 
        }
        pixels.show();
        lightBlasterEffect(ledPins, random(5, NUMPIXELS - 3), map(NUMPIXELS, 10, NUMPIXELS-10, 1, 2), 0, storage.sndProfile[storage.soundFont].blasterboltColor);
        break;

    }
  #ifdef TIP_MELT
    if (AState==AS_TIPMELT) { //animate blade tip in tipmelt mode
      Fire_Cooling = 55;
      Fire_Sparking = 120;
      static byte heat[TIP_PIXELS];
      int cooldown;    
      // Cool down every cell a little
      for( int i = 0; i < TIP_PIXELS; i++) {
        cooldown = random(0, ((Fire_Cooling * 10) / TIP_PIXELS) + 2);
      
        if(cooldown>heat[i]) {
          heat[i]=0;
        } else {
          heat[i]=heat[i]-cooldown;
        }
      }
      // Heat from each cell drifts 'up' and diffuses a little
      for(int k= TIP_PIXELS - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
      }
      // Randomly ignite new 'sparks' near the bottom
      if(random(255) < Fire_Sparking) {
        int y = random(7);
        heat[y] = heat[y] + random(160,255);
      }
  
      int flickRange = 3 + constrain((millis()-tipmeltStart)/1000,0,100); // gradually increase fire effect over time
      int flick = random(0,flickRange); 
      for (uint16_t i = 0; i <= TIP_PIXELS; i++) { // flash clash color           
        uint32_t tipcolor;
        if (flick < 2) { // clash color
          color.r = brightness * clashcolor.r / rgbFactor;
          color.g = brightness * clashcolor.g / rgbFactor;
          color.b = brightness * clashcolor.b / rgbFactor;
          if (i>=5)
            tipcolor = CombineColors(maincolor,color,i*100/TIP_PIXELS); // gradient into main color
          else
            tipcolor = color;
        } else if (flick == 2) { // flash white
          color.r = MAX_BRIGHTNESS;
          color.g = MAX_BRIGHTNESS;
          color.b = MAX_BRIGHTNESS;
          if (i>=5)
            tipcolor = CombineColors(maincolor,color,i*100/TIP_PIXELS); // gradient into main color
          else
            tipcolor = color;
        } else  { // gradually heaten up the tip and show flame effect
            tipcolor = HeatColor(heat[i], 0);    
        }
        pixels.setPixelColor(NUMPIXELS-i, tipcolor); 
      }
      pixels.show();
    }    
  #endif // TIP_MELT

  #endif
} // lightFlicker

uint32_t CombineColors(uint32_t color1, uint32_t color2, int8_t percentage1) {
  uint32_t color;

  uint8_t r1, g1, b1;
  UnpackColor(color1, r1, g1, b1);
  uint8_t r2, g2, b2;
  UnpackColor(color2, r2, g2, b2);

  color = pixels.Color(
    (percentage1*r1 + (100-percentage1)*r2)/100,
    (percentage1*g1 + (100-percentage1)*g2)/100,
    (percentage1*b1 + (100-percentage1)*b2)/100
  );
  return color;
}

void ColorMixing(uint32_t colorID, int8_t mod, uint8_t maxBrightness, bool Saturate) {
  #if defined LEDSTRINGS
  
  #endif
  

  #if defined PIXELBLADE or defined STAR_LED or defined ADF_PIXIE_BLADE
    uint32_t mixedColor = colorID;
    uint8_t r, g, b;
    UnpackColor(mixedColor, r, g, b);

      switch(mod) {
        default:
        case(0):
          if (Saturate) {
            mixedColor = pixels.Color(maxBrightness, g, b);
          }
          else {
            mixedColor = pixels.Color(constrain(r+1,0,255), g, b);
          }
          break;
        case(1):
          if (Saturate) {
            mixedColor = pixels.Color(0, g, b);
          }
          else {
            mixedColor = pixels.Color(constrain(r-1,0,255), g, b);
          }
          break;
        case(2):
          if (Saturate) {
            mixedColor = pixels.Color(r, maxBrightness, b);
          }
          else {
            mixedColor = pixels.Color(r, constrain(g+1,0,255), b);
          }
          break;
        case(3):
          if (Saturate) {
            mixedColor = pixels.Color(r, 0, b);
          }
          else {
            mixedColor = pixels.Color(r, constrain(g-1,0,255), b);
          }
          break;
        case(4):
          if (Saturate) {
            mixedColor = pixels.Color(r, g, maxBrightness);
          }
          else {
            mixedColor = pixels.Color(r, g, constrain(b+1,0,255));
          }
          break;
        case(5):
          if (Saturate) {
            mixedColor = pixels.Color(r, g, 0);
          }
          else {
            mixedColor = pixels.Color(r, g, constrain(b-1,0,255));
          }
          break; 
      }
        getColor(mixedColor);
        //lightOn(mixedColor, 0, NUMPIXELS-6);
        #if defined LS_DEBUG
          //Serial.print(storage.sndProfile[storage.soundFont].mainColor);
          Serial.print("\tR:");
          Serial.print(currentColor.r);
          Serial.print("\tG:");
          Serial.print(currentColor.g);
          Serial.print(" \tB:");
          Serial.println(currentColor.b);
        #endif
          #if defined STAR_LED
            // LED_RED, LED_GREEN, LED_BLUE
            analogWrite(LED_RED,currentColor.r); // RED
            analogWrite(LED_GREEN, currentColor.g); // GREEN
            analogWrite(LED_BLUE, currentColor.b); // BLUE  
          #endif
          #ifdef ADF_PIXIE_BLADE
            for(uint8_t i=0; i< NUMPIXELS; i++) {
              strip.setPixelColor(i, currentColor.r, currentColor.g, currentColor.b);
            }
            strip.show();
          #endif
  #endif
} // ColorMixing

void lightBlasterEffect(uint8_t ledPins[], uint8_t pixel, uint8_t range, uint16_t B_time, uint32_t SndFnt_MainColor) {
  #if defined LEDSTRINGS
    analogWrite(ledPins[random(1,5)], LOW); 
    delay(BLASTER_FX_DURATION); 
  #endif
  
  #if defined STAR_LED
    lightOn(ledPins, -1, currentColor); 
    delay(BLASTER_FX_DURATION);  
  #endif
  
  #ifdef ADF_PIXIE_BLADE
    for(uint8_t i=0; i< NUMPIXELS; i++) {
      strip.setPixelColor(i, currentColor.r, currentColor.g, currentColor.b);
    }
    strip.show();
    delay(BLASTER_FX_DURATION); 
  #endif
            
  #ifdef PIXELBLADE
    uint32_t blastcolor;
    uint32_t fadecolor;
    blastcolor = currentColor;
    getColor(SndFnt_MainColor);  // get the main blade color for the fading effect
    for (uint8_t i = 0; i<=2*range-1;i++) {
      if (fireblade) {
        #ifdef ANIBLADE
        // fully cool down (switch off LED) of a small segment of the blade, which will go up afterwards
        heat[(pixel/PIXELSTEP)-i] = 0; // white hot fire burst along the whole blade
        #endif // ANIBLADE
      }
      else {
        uint8_t j=i+pixel;
        if (j==pixel or j==pixel+2*range) { // 2 pixels at the edges shall be shut down
          fadecolor = 0;
          pixels.setPixelColor(j, fadecolor);
        }
        else if (j==pixel+range+1) { // middle pixel full white
          fadecolor = pixels.Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
          pixels.setPixelColor(j, fadecolor);
        }
        else { // rest of the pixels between middle and edge
          pixels.setPixelColor(j, blastcolor);
        }
        /* 
        for (uint8_t j = 0; j <=range; j++) {
          if (j==i) {
            pixels.setPixelColor(pixel-j, blastcolor);
            pixels.setPixelColor(pixel+j, blastcolor);
          }
          else {
            pixels.setPixelColor(pixel-j, currentColor);
            pixels.setPixelColor(pixel+j, currentColor);
          }
        }*/
        pixels.show();
        if (not fireblade) {
          delay(B_time/(2*range));  // blast deflect should last for ~500ms
        }
      }
    }
  #endif
} // lightBlasterEffect

void pixelblade_KillKey_Enable() {
  // Messing with these pins screws up the SD card
  #if defined USE_RAW_SPEAKER
    return;
  #endif

  #if defined PIXELBLADE or defined ADF_PIXIE_BLADE
    #ifdef PIXELBLADE
      digitalWrite(DATA_PIN,HIGH); // in order not to back-connect GND over the Data pin to the stripes when the Low-Sides disconnect it
    #elif defined ADF_PIXIE_BLADE
      digitalWrite(PIXIEPIN,HIGH); // in order not to back-connect GND over the Data pin to the stripes when the Low-Sides disconnect it
    #endif
    // cut power to the neopixels stripes by disconnecting their GND signal using the LS pins
    #if defined DIYINO_STARDUST_V2 or defined DIYINO_STARDUST_V3
    for (uint8_t j = 0; j < 3; j++) {
    #endif
    #ifdef DIYINO_PRIME  
    for (uint8_t j = 0; j < 6; j++) {
    #endif
      digitalWrite(ledPins[j], LOW);
    }
  #endif      
}

void pixelblade_KillKey_Disable() {
  // Messing with these pins screws up the SD card
  #if defined USE_RAW_SPEAKER
    return;
  #endif

  #if defined PIXELBLADE or defined ADF_PIXIE_BLADE
    // cut power to the neopixels stripes by disconnecting their GND signal using the LS pins
    #if defined DIYINO_STARDUST_V2 or defined DIYINO_STARDUST_V3
      for (uint8_t j = 0; j < 3; j++) {
    #endif
    #ifdef DIYINO_PRIME  
      for (uint8_t j = 0; j < 6; j++) {
    #endif
      digitalWrite(ledPins[j], HIGH);
    }
  #endif
}

void lightClashEffect(uint8_t ledPins[], uint32_t color) {

  #if defined LEDSTRINGS
    for (uint8_t i = 0; i <= 5; i++) {
      analogWrite(ledPins[i], 255);
    }
    //delay(CLASH_FX_DURATION);  // clash duration
  #endif
  
  #if defined STAR_LED
    getColor(storage.sndProfile[storage.soundFont].clashColor);
    lightOn(ledPins, -1, currentColor);
    //delay(CLASH_FX_DURATION);  // clash duration
  #endif

  #ifdef ADF_PIXIE_BLADE
    getColor(storage.sndProfile[storage.soundFont].clashColor);
    lightOn(ledPins, -1, currentColor);
    //delay(CLASH_FX_DURATION);  // clash duration
  #endif
  
  #if defined PIXELBLADE
    if (fireblade) { // #if defined FIREBLADE  // simply flash white
          #ifdef ANIBLADE
          //getColor(storage.sndProfile[storage.soundFont].clashColor);
          //lightOn(ledPins, -1, currentColor);
          for( int i = 0; i < sizeof(heat); i++) {
            heat[i] = constrain(heat[i]+70,0,255); // white hot fire burst along the whole blade
          }
          #endif // ANIBLADE
    } // fireblade
    else { // #else
          getColor(storage.sndProfile[storage.soundFont].clashColor);
          lightOn(ledPins, -1, currentColor);
          //delay(CLASH_FX_DURATION);  // clash duration
    } // #endif
  #endif
  
}

void lightSwingEffect(uint8_t ledPins[]) {
  #ifndef LEDSTRINGS
    if (not fireblade) { 
      uint32_t swingColor; // inject a dash of clash color into main color when swinging
      uint8_t mainR, mainG, mainB;
      UnpackColor(storage.sndProfile[storage.soundFont].mainColor, mainR, mainG, mainB);
      uint8_t clashR, clashG, clashB;
      UnpackColor(storage.sndProfile[storage.soundFont].mainColor, clashR, clashG, clashB);
      
      swingColor = pixels.Color(
        (9 * mainR + clashR) / 10,
        (9 * mainG + clashG) / 10,
        (9 * mainB + clashB) / 10        
      );
      lightOn(ledPins, -1, swingColor);
    }
  #endif // not LEDSTRINGS
}

#if defined COLOR_PROFILE and not defined LEDSTRINGS
void getColorFix(uint8_t colorID) {
  switch (colorID) {
    case 0:
      //Red
      currentColor = pixels.Color(MAX_BRIGHTNESS, 0, 0);
      break;
    case 1:
      //Orange
      currentColor = pixels.Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS/4, 0);
      break;
    case 2:
      //Amber
      currentColor = pixels.Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS*66/100, 0);
      break;
    case 3:
      //Yellow
      currentColor = pixels.Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0);
      break;
    case 4:
      //Lime
      currentColor = pixels.Color(MAX_BRIGHTNESS*34/100, MAX_BRIGHTNESS, 0);
      break;
    case 5:
      //Green
      currentColor = pixels.Color(0, MAX_BRIGHTNESS, 0);
      break;
    case 6:
      //Cyan
      currentColor = pixels.Color(0, MAX_BRIGHTNESS, MAX_BRIGHTNESS*34/100);
      break;
    case 7:
      //Blue
      currentColor = pixels.Color(0, 0, MAX_BRIGHTNESS);
      break;
    case 8:
      //Light Blue
      currentColor = pixels.Color(0, MAX_BRIGHTNESS, MAX_BRIGHTNESS*63/100);
      break;
    case 9:
      //Ice Blue
      currentColor = pixels.Color(0, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;
    case 10:
      //Mauve
      currentColor = pixels.Color(MAX_BRIGHTNESS*12/100, 0, MAX_BRIGHTNESS);
      break;
    case 11:
      //Purple
      currentColor = pixels.Color(MAX_BRIGHTNESS*35/100, 0, MAX_BRIGHTNESS);
      break;
    case 12:
      //Pink
      currentColor = pixels.Color(MAX_BRIGHTNESS, 0, MAX_BRIGHTNESS);
      break;
    case 13:
      //Crimson
      currentColor = pixels.Color(MAX_BRIGHTNESS, 0, MAX_BRIGHTNESS*5/100);
      break;
    case 14:
      //White
      currentColor = pixels.Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;
    case 15:
      // LED1 and LED2 full on
      currentColor = pixels.Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0);
      break;
    case 16:
      // only LED3 is on
      currentColor = pixels.Color(0, 0, MAX_BRIGHTNESS);
      break;
    default:
      // White (if enough voltage)
      currentColor = pixels.Color(100, 100, 100);
      break;
  }
} //getColorFix
#endif

void BladeMeter (uint8_t ledPins[], int meterLevel) {  //expects input of 0-100
  //normalize data if to max and min if out of range
  if (meterLevel <= 0) { meterLevel = 0; } 
  if (meterLevel >= 100) { meterLevel = 100; }

  #ifdef LEDSTRINGS // light blade as 6 segment bar graph
    meterLevel = meterLevel*6/100;
      for (uint8_t i = 0; i < sizeof(ledPins); i++) {
        if (i <= meterLevel) {
          digitalWrite(ledPins[i], HIGH);
        } else {
          digitalWrite(ledPins[i], LOW);
        }
      }
  #endif

  #ifdef STAR_LED // light led in gradient from red to green
    analogWrite(LED_BLUE,0);
    analogWrite(LED_RED, (MAX_BRIGHTNESS * (100 - meterLevel))/255);
    analogWrite(LED_GREEN, (MAX_BRIGHTNESS * meterLevel)/255);
    //Serial.println((MAX_BRIGHTNESS * meterLevel)/255);
  //  unsigned int meterGreen = meterLevel * 255 / 100;
  //  unsigned int meterRed = (100 - meterLevel) * 255 / 100;
  #endif

  #ifdef PIXELBLADE // light blade as 3 color meter proportionate to length
    uint32_t value;
    //set first pixel for accent LED compatability
    if (meterLevel < 30) {
      value = pixels.Color(MAX_BRIGHTNESS/2, 0, 0);
    } else if (meterLevel < 60) {
      value = pixels.Color(MAX_BRIGHTNESS/2*0.8, MAX_BRIGHTNESS/2*0.6, 0);
    } else {
      value = pixels.Color(0, MAX_BRIGHTNESS/2, 0);
    }
    pixels.setPixelColor(0, value);

    //set rest of blade
    for (unsigned int i = 1; i < NUMPIXELS; i++) { // turn on/off one LED at a time
        if (i < NUMPIXELS * meterLevel / 100){
          if (i < (30 * NUMPIXELS / 100)) {
            value = pixels.Color(MAX_BRIGHTNESS, 0, 0);
          } else if (i < (60 * NUMPIXELS / 100)) {
            value = pixels.Color(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0);
          } else {
            value = pixels.Color(0, MAX_BRIGHTNESS, 0);
          }
        } else {
          value = 0;
        }      
        pixels.setPixelColor(i, value);
      }
      pixels.show(); // Sends the data to the LEDs
  //    delay(3);
  #endif
}

#ifdef ADF_PIXIE_BLADE
void InitAdafruitPixie(uint8_t ledPins[]) {
  //pixieSerial.setSerial(-1, PIXIEPIN);
  pixieSerial.begin(115200); // Pixie REQUIRES this baud rate
  strip.setBrightness(MAX_BRIGHTNESS);  // Adjust as necessary to avoid blinding
}
#endif



#if defined FoCSTRING
void FoCOn(uint8_t pin) {
	digitalWrite(FoCSTRING, HIGH);
//	PORTC &= ~(1 << PD3);

} //FoCOn
void FoCOff(uint8_t pin) {
	digitalWrite(FoCSTRING, LOW);
//	PORTC |= (1 << PD3);
} //FoCOff
#endif



#ifdef JUKEBOX
#ifndef PIXELBLADE
void JukeBox_Stroboscope(uint8_t ledPins[]) {
 uint16_t variation = 0;
 uint16_t temp_variation=0;
 for (uint8_t i=0; i<=SAMPLESIZEAVERAGE-1;i++) {
  temp_variation=temp_variation + abs(analogRead(SPK1) - analogRead(SPK2));
 }
 variation=temp_variation/SAMPLESIZEAVERAGE;
  if (variation>=80) {
    analogWrite(ledPins[0], MAX_BRIGHTNESS);
  }
  else analogWrite(ledPins[0], 0);
  if (variation>=110) {
    analogWrite(ledPins[1], MAX_BRIGHTNESS);
  }
  else analogWrite(ledPins[1], 0);  
  if (variation>=140) {
    analogWrite(ledPins[2], MAX_BRIGHTNESS);
  }
  else analogWrite(ledPins[2], 0);  
  if (variation>=170) {
    analogWrite(ledPins[3], MAX_BRIGHTNESS);
  }
  else analogWrite(ledPins[3], 0);
  if (variation>=200) {
    analogWrite(ledPins[4], MAX_BRIGHTNESS);
  }
  else analogWrite(ledPins[4], 0);
  if (variation>=230) {
    analogWrite(ledPins[5], MAX_BRIGHTNESS);
  }
  else analogWrite(ledPins[5], 0);
  //delay(50);
}
#endif
#endif


#ifdef JUKEBOX
#ifdef PIXELBLADE
 
void JukeBox_Stroboscope(uint32_t color) {

 uint16_t variation = 0;
 uint16_t temp_variation=0;
 uint32_t tempcolor;

 for (uint8_t i=0; i<=SAMPLESIZEAVERAGE-1;i++) {
  temp_variation=temp_variation + constrain(abs(analogRead(SPK1) - analogRead(SPK2)),0,512);
  //Serial.println(abs(analogRead(SPK1) - analogRead(SPK2)));
 }
 variation=temp_variation/SAMPLESIZEAVERAGE;
  // assumption -> variation max 280
  //Serial.print("\t");Serial.println(variation);


  for (uint16_t i = 1; i <= variation; i++) {
    pixels.setPixelColor(i, color);
  }
  tempcolor.r = 0;
  tempcolor.g = 0;
  tempcolor.b = 0; // RGB Value -> Off
  for (uint16_t i = (variation)+1; i <= NUMPIXELS; i++) {
    pixels.setPixelColor(i, tempcolor);
  }
  pixels.show();  

}
#endif
#endif

#ifdef PIXELBLADE
#ifdef ANIBLADE
void FireBlade(uint8_t DominantColor) {
// Array of temperature readings at each simulation cell
  int pixelnumber;
  
  // Step 1.  Cool down every cell a little
/*#ifdef CROSSGUARDSABER
    for( int i = 0; i < MN_STRIPE; i++) {
      heat[i] = constrain(heat[i] - random(((Fire_Cooling * 10) / MN_STRIPE) + 2),0,255);
    }
    for( int i = 0; i < CG_STRIPE; i++) {
      heat_cg[i] = constrain(heat_cg[i] - random(5),0,255);
    }
#else */
    for( int i = 0; i < sizeof(heat); i++) {
      // the random() function in this loop causes phantom swings
      heat[i] = constrain(heat[i] - random(((Fire_Cooling * 10) / sizeof(heat)) + 2),0,255);
    }
//#endif

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
/*#ifdef CROSSGUARDSABER
    for( int k= MN_STRIPE - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    for( int k= CG_STRIPE - 1; k >= 2; k--) {
      heat_cg[k] = (heat_cg[k - 1] + heat_cg[k - 2] + heat_cg[k - 2] ) / 3;
    }
#else*/
    for( int k= sizeof(heat) - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
//#endif
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
/*#ifdef CROSSGUARDSABER
    if( random(255) < Fire_Sparking ) {
      int y = random(7);
      heat[y] = constrain(heat[y] + random(95)+160,0,255 );
    }
    if( random(255) < 10 ) {
      int y = random(4);
      heat_cg[y] = constrain(heat_cg[0] + random(95)+160,0,255 );  
    } 
#else*/
    if( random(255) < Fire_Sparking ) {
      int y = random(7);
      heat[y] = constrain(heat[y] + random(95)+160,0,255 );
    }
//#endif

    // Step 4.  Map from heat cells to LED colors 
/*#ifdef CROSSGUARDSABER
    for( int j = 0; j < CG_STRIPE; j++) {
      uint32_t color = HeatColor( heat_cg[j],DominantColor);
      //if( gReverseDirection ) {
      //  pixelnumber = (CG_STRIPE-1) - j;
      //} else {
      //  pixelnumber = j;
      //}
      LED.setPixelColor(j, color); // Set value at LED found at index j
    }
    for( int j = CG_STRIPE; j < CG_STRIPE + MN_STRIPE; j++) {
      uint32_t color = HeatColor( heat[j],DominantColor);
      //if( gReverseDirection ) {
      //  pixelnumber = (CG_STRIPE + MN_STRIPE-1) - j;
      //} else {
      //  pixelnumber = j;
      //}
      pixels.setPixelColor(j, color); // Set value at LED found at index j
    }
#else*/
    for( int j = 0; j < sizeof(heat); j++) {
       uint32_t color = HeatColor( heat[j],DominantColor);
       if (PIXELSTEP*j+1<=NUMPIXELS-1) {
        for (int i=0 ; i<PIXELSTEP; i++) {
          pixels.setPixelColor(PIXELSTEP*j+i, color); // Set value at LED found at index j
        }
        //pixels.setPixelColor(2*j, color); // Set value at LED found at index j
        //pixels.setPixelColor(2*j+1, color); // Set value at LED found at index j
       }
    }
//#endif
}

// uint32_t HeatColor( uint8_t temperature)
//
// Approximates a 'black body radiation' spectrum for
// a given 'heat' level.  This is useful for animations of 'fire'.
// Heat is specified as an arbitrary scale from 0 (cool) to 255 (hot).
// This is NOT a chromatically correct 'black body radiation'
// spectrum, but it's surprisingly close, and it's fast and small.
//
// On AVR/Arduino, this typically takes around 70 bytes of program memory,
// versus 768 bytes for a full 256-entry RGB lookup table.

uint32_t HeatColor( uint8_t temperature, uint8_t DominantColor)
{
    uint32_t heatcolor;

    // Scale 'heat' down from 0-255 to 0-191,
    // which can then be easily divided into three
    // equal 'thirds' of 64 units each.

    uint8_t t192 = round((temperature/255.0)*191.0);
    // THIS ONE ISN'T WORKING, SO I JUST IMPLEMENTED MY OWN, MUCH LESS PERFORMANT VERSION ^^
    // uint8_t t192 = scale8_video(temperature, 192);
    
     //Serial.print(F("scale8_video_result: "));
     //Serial.print(temperature);Serial.print("/t");Serial.println(t192);

    // calculate a value that ramps up from
    // zero to 255 in each 'third' of the scale.
    uint8_t heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2; // scale up to 0..252

    // now figure out which third of the spectrum we're in:
    if( t192 & 0x80) {
        if (DominantColor==0 or DominantColor>2) { // red
          // we're in the hottest third
          heatcolor = pixels.Color(255,255,heatramp);
        }
        else if (DominantColor==1) { // green
          // we're in the hottest third
          heatcolor = pixels.Color(heatramp,255,255);
        }
        else if (DominantColor==2) { // blue
          // we're in the hottest third
          heatcolor = pixels.Color(heatramp,255,255);
        }
      } else if( t192 & 0x40 ) {
        // we're in the middle third
        if (DominantColor==0 or DominantColor>2) { // red
          heatcolor = pixels.Color(255,heatramp,0);
        }
        else if (DominantColor==1) { // green
          heatcolor = pixels.Color(0,255,heatramp);
        }
        else if (DominantColor==2) { // blue
          heatcolor = pixels.Color(0,heatramp,255);
        }
    } else {
        // we're in the coolest third
        if (DominantColor==0 or DominantColor>2) { // red
          heatcolor = pixels.Color(heatramp,0,0);
        }
        else if (DominantColor==1) { // green
          heatcolor = pixels.Color(0,heatramp,0);      
        }
        else if (DominantColor==2) { // blue
          heatcolor = pixels.Color(0,0,heatramp);   
        }
    }

    return heatcolor;
}

/*void SparkBladeFX(uint8_t AState=0)
{
  //init spark center position and size for the first time
  if (AState==AS_IGNITION or true) {
    for( uint8_t i = 0; i < (NUMPIXELS/10)-1; i++) {
      SparkBlade[i][0]=constrain(i*(NUMPIXELS/10)+random(0,NUMPIXELS/10),0,NUMPIXELS-1);
      SparkBlade[i][1]=random(0,NUMPIXELS/10);
    }
  }
  else { // flickering
    for( uint8_t i = 0; i < sizeof(SparkBlade); i++) {
      for( uint8_t j = 0; j < sizeof(SparkBlade[0]); j++) {
        for( uint8_t k=SparkBlade[i][0]-SparkBlade[i][1]; k<SparkBlade[i][0]+SparkBlade[i][1]; k++) {
          heat[k]=constrain(heat[k]+Spark_Step,0,MAX_BRIGHTNESS);
      }
    }
  }
    for( int j = 0; j < NUMPIXELS; j++) {
      uint32_t color = HeatColor( heat[j],1);
      pixels.setPixelColor(j, color); // Set value at LED found at index j
    }
    pixels.show();  
  }
}*/

/*
uint8_t scale8_video( uint8_t i, uint8_t scale)
{
//    uint8_t j = (((int)i * (int)scale) >> 8) + ((i&&scale)?1:0);
//    // uint8_t nonzeroscale = (scale != 0) ? 1 : 0;
//    // uint8_t j = (i == 0) ? 0 : (((int)i * (int)(scale) ) >> 8) + nonzeroscale;
//    return j;
    uint8_t j=0;
    asm volatile(
        "  tst %[i]\n\t"
        "  breq L_%=\n\t"
        "  mul %[i], %[scale]\n\t"
        "  mov %[j], r1\n\t"
        "  clr __zero_reg__\n\t"
        "  cpse %[scale], r1\n\t"
        "  subi %[j], 0xFF\n\t"
        "L_%=: \n\t"
        : [j] "+a" (j)
        : [i] "a" (i), [scale] "a" (scale)
        : "r0", "r1");

    return j;
}
*/
#endif
#endif // ANIBLADE

#ifdef COLOR_PROFILE
uint32_t setColorProfile(uint32_t currentcolor) {
  // Red  
  colorProfiles[0] = pixels.Color(255,0,0);
  // Green  
  colorProfiles[1] = pixels.Color(0,255,0);
  // Blue  
  colorProfiles[2] = pixels.Color(0,0,255);
  // Orange  
  colorProfiles[3] = pixels.Color(255,100,0);
  // Cyan  
  colorProfiles[4] = pixels.Color(0,100,255);

  // TODO: Figure out what this is actually supposed to return, currently I'm just returning currentColor to avoid an error
  return currentColor;
}
#endif


//#if defined ACCENT_LED
void accentLEDControl( AccentLedAction_En AccentLedAction) {
#if defined HARD_ACCENT
  if (AccentLedAction==AL_PULSE) {
        if (millis() - lastAccent <= 400) {
          analogWrite(ACCENT_LED, millis() - lastAccent);
        } else if (millis() - lastAccent > 400
            and millis() - lastAccent <= 800) {
          analogWrite(ACCENT_LED, 800 - (millis() - lastAccent));
        } else {
          lastAccent = millis();
        }
  }
  else if (AccentLedAction==AL_ON) {
    digitalWrite(ACCENT_LED,HIGH);
  }
  else {  // AL_OFF
    digitalWrite(ACCENT_LED,LOW);    
  }
#endif //HARD_ACCENT
}
//#endif

void AccentMeter (int MeterLevel) {
  #ifdef PIXEL_ACCENT
  if (NUM_ACCENT_PIXELS < 3){
    for (int i=0; i<=NUM_ACCENT_PIXELS; i++) {
        accentPixels.setPixelColor(i, {PIXEL_ACCENT_BRIGHTNESS * (100 - MeterLevel) / 100,PIXEL_ACCENT_BRIGHTNESS * MeterLevel / 100,0});
    }
  }
  accentpixels.show();
  #endif    
}

void pixelAccentUpdate() { 
  #ifdef PIXEL_ACCENT
  if (millis() - lastAccent > 900) lastAccent = millis();
  int tB = PIXEL_ACCENT_BRIGHTNESS;
  if (SaberState == S_STANDBY) tB = tB * (millis() - lastAccent) / 900;
  if (SaberState == S_SLEEP) tB = 0;
  if (SaberState == S_CONFIG) {
    if (ConfigModeSubStates == CS_SOUNDFONT) {
      accentPixels.setPixelColor(0, {storage.sndProfile[storage.soundFont].mainColor.r*tB / 255, storage.sndProfile[storage.soundFont].mainColor.g*tB / 255, storage.sndProfile[storage.soundFont].mainColor.b*tB / 255}); //{tB,tB,tB});//
    } else if (ConfigModeSubStates == CS_SOUNDFONT || ConfigModeSubStates == CS_MAINCOLOR || ConfigModeSubStates == CS_CLASHCOLOR || ConfigModeSubStates == CS_BLASTCOLOR) {
      switch (modification) {
        case (0): // red +
          accentPixels.setPixelColor(0, {tB, 0, 0});
          break;
        case (1): // red -
          accentPixels.setPixelColor(0, {tB / 4, 0, 0});
          break;
        case (2): // green +
          accentPixels.setPixelColor(0, {0, tB, 0});
          break;
        case (3): // green -
          accentPixels.setPixelColor(0, {0, tB / 4, 0});
          break;
        case (4): // blue +
          accentPixels.setPixelColor(0, {0, 0, tB});
          break;
        case (5): // blue -
          accentPixels.setPixelColor(0, {0, 0, tB / 4});
          break;
      }
    } else if (ConfigModeSubStates == CS_BATTERYLEVEL || ConfigModeSubStates == CS_VOLUME || ConfigModeSubStates == CS_SWINGSENSITIVITY){
    } else {
      accentPixels.setPixelColor(0, {storage.sndProfile[storage.soundFont].mainColor.r*tB / 255, storage.sndProfile[storage.soundFont].mainColor.g*tB / 255, storage.sndProfile[storage.soundFont].mainColor.b*tB / 255}); //{tB,tB,tB});//
    }
  } else {
    accentPixels.setPixelColor(0, {storage.sndProfile[storage.soundFont].mainColor.r*tB / 255, storage.sndProfile[storage.soundFont].mainColor.g*tB / 255, storage.sndProfile[storage.soundFont].mainColor.b*tB / 255}); //{tB,tB,tB});//
    if (ActionModeSubStates == AS_BLASTERDEFLECTMOTION && (millis() - lastAccent > 300) && (millis() - lastAccent <= 600)) accentPixels.setPixelColor(0, {storage.sndProfile[storage.soundFont].blasterboltColor.r*tB / 255, storage.sndProfile[storage.soundFont].blasterboltColor.g*tB / 255, storage.sndProfile[storage.soundFont].blasterboltColor.b*tB / 255}); //{tB,tB,tB});//
    if (lockuponclash && (millis() - lastAccent >= 600)) accentPixels.setPixelColor(0, {storage.sndProfile[storage.soundFont].clashColor.r*tB / 255, storage.sndProfile[storage.soundFont].clashColor.g*tB / 255, storage.sndProfile[storage.soundFont].clashColor.b*tB / 255}); //{tB,tB,tB});//
  }
  
  accentpixels.show();
  #endif
}

void UnpackColor(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b) {
  r = (color >> 16) & 0xFF; // Extract red component
  g = (color >> 8) & 0xFF;  // Extract green component
  b = color & 0xFF;         // Extract blue component
};
