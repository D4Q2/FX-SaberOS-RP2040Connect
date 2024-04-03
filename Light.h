/*
 * Light.h
 *
 *  Created on: 21 Octber 2016
 * author: 		Sebastien CAPOU (neskweek@gmail.com) and Andras Kun (kun.andras@yahoo.de)
 * Source :  https://github.com/Protonerd/FX-SaberOS
 */

#if not defined LIGHT_H_
#define LIGHT_H_

#include <Arduino.h>
#include "Config_SW.h"

enum AccentLedAction_En {AL_PULSE, AL_ON, AL_OFF};

#ifdef PIXELBLADE
  #include <Adafruit_NeoPixel.h>
#endif

// ====================================================================================
// ===              	    			LED FUNCTIONS		                		===
// ====================================================================================

void BladeMeter (uint8_t ledPins[], int meterLevel); 

// unified light functions
void lightOn(uint8_t ledPins[], int8_t segment = -1, uint32_t color=0, int8_t StartPixel=-1, int8_t StopPixel=-1);
void lightOff(uint8_t ledPins[], int8_t segment = -1, int8_t StartPixel=-1, int8_t StopPixel=-1);
void getColor(uint32_t color=0); //getColor
void RampBlade(uint16_t RampDuration, bool DirectionUpDown, int8_t StartPixel=-1, int8_t StopPixel=-1);
void lightIgnition(uint8_t ledPins[], uint16_t time, uint8_t type, uint32_t color=0, int8_t StartPixel=-1, int8_t StopPixel=-1);
void lightRetract(uint8_t ledPins[], uint16_t time, uint8_t type,uint32_t color=0, int8_t StartPixel=-1, int8_t StopPixel=-1);
void lightFlicker(uint8_t ledPins[],uint8_t type, uint8_t value = 0,uint32_t maincolor=0, uint32_t clashcolor=0,uint8_t AState=0, int8_t StartPixel=-1, int8_t StopPixel=-1);
void ColorMixing(uint32_t colorID=0, int8_t mod=-1, uint8_t maxBrightness=MAX_BRIGHTNESS, bool Saturate=false);
void lightBlasterEffect(uint8_t ledPins[], uint8_t pixel, uint8_t range, uint16_t B_time=BLASTER_FX_DURATION, uint32_t SndFnt_MainColor=0);
void lightClashEffect(uint8_t ledPins[], uint32_t color=0);
void lightSwingEffect(uint8_t ledPins[]);
uint32_t CombineColors(uint32_t color1, uint32_t color2, int8_t percentage1);

#ifdef JUKEBOX
  #ifdef PIXELBLADE
    void JukeBox_Stroboscope(uint32_t color=0);
  #else
    void JukeBox_Stroboscope(unit8_t ledPins[], uint32_t color=0);
  #endif
#endif
void pixelblade_KillKey_Enable();
void pixelblade_KillKey_Disable();

void getColorFix(uint8_t colorID);

void FoCOn (uint8_t pin);
void FoCOff (uint8_t pin);

#ifdef ADF_PIXIE_BLADE
  void InitAdafruitPixie(uint8_t ledPins[]);
#endif

#ifdef PIXELBLADE
  #ifdef ANIBLADE
  void FireBlade(uint8_t DominantColor=0);
  uint32_t HeatColor( uint8_t temperature, uint8_t DominantColor);
  uint8_t scale8_video( uint8_t i, uint8_t scale);
  uint8_t DominantMainColor(uint32_t color=0);
  #endif
#endif

void accentLEDControl(AccentLedAction_En AccentLedAction);
//void PWM();
//#ifdef PIXEL_ACCENT
  void pixelAccentUpdate();
  void AccentMeter (int MeterLevel);
//#endif

void UnpackColor(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b);

#endif /* LIGHT_H_ */


