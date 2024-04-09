/*
 * Config.cpp
 *
 * author: 		Sebastien CAPOU (neskweek@gmail.com) and Andras Kun (kun.andras@yahoo.de)
 * Source :  https://github.com/Protonerd/FX-SaberOS
 *      Author: neskw
 */
#include "ConfigMenu.h"
#include "Light.h"
#include "Soundfont.h"

// global Saber state and Sub State variables
SaberStateEnum SaberState;
SaberStateEnum PrevSaberState;
ActionModeSubStatesEnum ActionModeSubStates;
ConfigModeSubStatesEnum ConfigModeSubStates;
ActionModeSubStatesEnum PrevActionModeSubStates;
ConfigModeSubStatesEnum PrevConfigModeSubStates;
//SubStateEnum SubState;

extern int8_t modification;
extern bool play;
extern int16_t value;


extern uint8_t ledPins[];
extern uint32_t currentColor;
#if defined PIXELBLADE
extern uint32_t color;
#endif
extern void HumRelaunch();
#ifdef USE_DFPLAYER
  extern void SinglePlay_Sound(uint8_t track);
#elif defined(USE_RAW_SPEAKER)
  extern void SinglePlay_Sound(String track);
#endif
extern void LoopPlay_Sound(uint8_t track);
extern void Pause_Sound();
extern void Resume_Sound();
extern void Set_Loop_Playback();
extern void Set_Volume();
extern void Disable_FTDI(bool ftdi_off);
extern void Disable_MP3(bool mp3_off);
extern void confParseValue(uint16_t variable, uint16_t min, uint16_t max,
    short int multiplier);
extern uint8_t GravityVector();
#ifdef BATTERY_CHECK
extern void BatLevel_ConfigEnter();
#endif

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
// ====================================================================================
// ===           	  	 			CONFIG MODE FUNCTIONS	                		===
// ====================================================================================

// this function ensures that config menu items which have values between a min and a max value
// wrap back to min/max upon reaching max/min. It also plays a sound notifying the user if either min or max value has beeb reached.
// This function is also in charge of changing the actual value of a setting via the value global variable.
void confParseValue(uint16_t variable, uint16_t min, uint16_t max,
		short int multiplier) {

	value = variable + (multiplier * 1);

	if (value < (int) min) {
		value = max;
	} else if (value > (int) max) {
		value = min;
	} else if (value == (int) min and play) {
		play = false;
    #ifdef USE_DFPLAYER
		  SinglePlay_Sound(10); 
    #elif defined(USE_RAW_SPEAKER)
      SinglePlay_Sound("010-Min");
    #endif
		delay(150);
	} else if (value == (int) max and play) {
		play = false;
    #ifdef USE_DFPLAYER
		  SinglePlay_Sound(9);
    #elif defined(USE_RAW_SPEAKER)
      SinglePlay_Sound("009-Max");
    #endif
		delay(150);
	}
} //confParseValue

void NextConfigState(){
  if (ConfigModeSubStates!=CS_MAINCOLOR and ConfigModeSubStates!=CS_CLASHCOLOR and ConfigModeSubStates!=CS_BLASTCOLOR) {  
    lightOff(ledPins, -1);
  }
  ConfigModeSubStates=static_cast<ConfigModeSubStatesEnum>(ConfigModeSubStates+1); // change to next config state in the ordered list
  if (ConfigModeSubStates== CS_LASTMEMBER) {
    ConfigModeSubStates=static_cast<ConfigModeSubStatesEnum>(0); // after the last config menu item go back to the first
  }

  Serial.println(ConfigModeSubStates);
    
  if (ConfigModeSubStates== (CS_STORAGEACCESS+1)) {
    Disable_FTDI(false);
    delay(200);
    Disable_MP3(false);
    delay(200);
  }
  switch(ConfigModeSubStates) {
      case CS_VOLUME: 
        #if defined LS_FSM
          Serial.print(F("Volume"));
        #endif  
        ConfigModeSubStates=CS_VOLUME;
        BladeMeter(ledPins, storage.volume*100/30);
        AccentMeter(storage.volume*100/30);
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(4);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("004-Volume");
        #endif
        delay(500);
        break;
      case CS_SOUNDFONT: 
        #if defined LS_FSM
          Serial.print(F("Sound font"));
        #endif        
        lightOff(ledPins, -1);
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(5);
          delay(600);
          SinglePlay_Sound(soundFont.getMenu((storage.soundFont)*NR_FILE_SF));
          delay(500);  
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("005-SoundFont");
          delay(600);
          SinglePlay_Sound("00_boot");
          delay(500);
        #endif
        break;
      case CS_MAINCOLOR: 
        #if defined LS_FSM
          Serial.print(F("Main color"));
        #endif        
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(6);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("006-MainColor");
        #endif
        delay(500); 
        getColor(storage.sndProfile[storage.soundFont].mainColor);
        pixelblade_KillKey_Disable();
        lightOn(ledPins, -1, currentColor, NUMPIXELS/2, NUMPIXELS-6);
        break;  
      case CS_CLASHCOLOR: 
        #if defined LS_FSM
          Serial.print(F("Clash color"));
        #endif        
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(7);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("007-ClashColor");
        #endif
        delay(500); 
        getColor(storage.sndProfile[storage.soundFont].clashColor);
        pixelblade_KillKey_Disable();
        lightOn(ledPins, -1, storage.sndProfile[storage.soundFont].clashColor, 1, NUMPIXELS/2-1); 
        break;
      case CS_BLASTCOLOR: 
        #if defined LS_FSM
          Serial.print(F("Blaster deflect color"));
        #endif        
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(8);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("008-BlastColor");
        #endif
        delay(500); 
        getColor(storage.sndProfile[storage.soundFont].blasterboltColor);
        pixelblade_KillKey_Disable();
        lightOn(ledPins, -1, storage.sndProfile[storage.soundFont].blasterboltColor, NUMPIXELS*3/4-5, NUMPIXELS*3/4); 
        break;         
      case CS_FLICKERTYPE: 
        #if defined LS_FSM
          Serial.print(F("Flicker type"));
        #endif  
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(25);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("025-flickerStyle");
        #endif
        delay(700);
        LoopPlay_Sound(soundFont.getHum((storage.soundFont)*NR_FILE_SF));
        break;           
      case CS_POWERONOFFTYPE: 
        #if defined LS_FSM
          Serial.print(F("Power on/off type"));
        #endif  
        ConfigModeSubStates=CS_POWERONOFFTYPE;
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(24);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("024-ignitionStyle");
        #endif
        delay(500); 
        break; 
      case CS_SWINGSENSITIVITY:
        #if defined LS_FSM
          Serial.print(F("Swing Sensitivity"));
        #endif  
        BladeMeter(ledPins, (storage.sndProfile[storage.soundFont].swingSensitivity)/100);
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(26);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("026-swingSensitivity");
        #endif
        delay(500); 
        break;  
      case CS_SLEEPINIT: 
        #if defined LS_FSM
          Serial.print(F("Initialize sleep mode"));
        #endif        
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(29);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("029-sleepModeInit");
        #endif
        delay(500);
        break;   
#ifdef BATTERY_CHECK        
      case CS_BATTERYLEVEL: 
        #if defined LS_FSM
          Serial.print(F("Display battery level"));
        #endif        
        BatLevel_ConfigEnter();
        break;   
#endif        
      case CS_STORAGEACCESS: 
        #if defined LS_FSM
          Serial.print(F("USB Media storage access"));
        #endif        
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(28);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("028-storageMediaAccess");
        #endif
        delay(500);
        Disable_FTDI(true); // disable FTDI to be able to manipulate storage media on board via USB
        break;   
      case CS_UARTMODE: 
        #if defined LS_FSM
          Serial.print(F("USB board programming access"));
        #endif        
        #ifdef USE_DFPLAYER
          SinglePlay_Sound(27);
        #elif defined(USE_RAW_SPEAKER)
          SinglePlay_Sound("027-programingMode");
        #endif
        delay(500);
        Disable_MP3(true);
        //delay(1000);
        Disable_FTDI(false); // enable FTDI again
        break;   
        }  
}

                        

