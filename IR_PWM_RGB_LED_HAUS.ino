/* *********************************************************
 * IR PWM RGB LED HAUS
 *
 * 2012,2013 Moritz Kobel, http://www.kobelnet.ch
 *
 * Change color using the function buttons on a Sony RM-U265 remote
 *
 ***********************************************************
 */

/* Based on several libraries + examples:
 * http://www.elcojacobs.com/shiftpwm/
 * http://www.arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
 * http://wiki.play-zone.ch/index.php?title=Arduino_Fernbedienung
 */
 
 
/* *************
 * PWM Init
 * ************/
const int ShiftPWM_latchPin=8;
#define SHIFTPWM_NOSPI
const int ShiftPWM_dataPin = 11;
const int ShiftPWM_clockPin = 12;

const bool ShiftPWM_invertOutputs = false; 
const bool ShiftPWM_balanceLoad = false;

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!

unsigned char maxBrightness = 96;
unsigned char pwmFrequency = 100;
const int numRegisters = 3;
const int numRGBleds = (numRegisters*8/3)  -1; // the one led is disabled

/* *************
 * IR Init
 * ************/
#include <IRremote.h>

// start receiver on pin 13
IRrecv irrecv(13);

int numEnabled = 1;  // max amount of concurrent leds
int enabledLeds[numRGBleds]; // the enabled leds 
int currentLeds[numRGBleds]; // current intensity 
int colors[numRGBleds];  // colors : 0-5
int mode = 1; // 0=move slow (not implemented) / 1=random / 2=mute
int doShift = 0;  // counter, doShift = 0 -> change colors
int shiftLoops = 150;
int fadeDelay = 5;
int shiftDelay = 10;   

int ledmap[] = {0,1,2,3,4,6,7}; // map around disabled led nr. 5

void setup(){
  Serial.begin(9600);

  ShiftPWM.SetAmountOfRegisters(numRegisters);
  ShiftPWM.SetPinGrouping(8); 
  
  ShiftPWM.Start(pwmFrequency,maxBrightness);
  ShiftPWM.SetAll(0);
  
  irrecv.enableIRIn();
  
  for ( int i = 0 ; i < numRGBleds ; i++ )
  {
      enabledLeds[i] = 0;
      currentLeds[i] = 0;
      colors[i] = 0;
     
  }
  randomSeed(analogRead(0));
}

void loop() {
  decode_results results;
 
  // receive?
  if (irrecv.decode(&results)) {
 
    // Debug Value 
    Serial.println(results.value, HEX);
 
    switch ( results.value ) {
      case 0x896:  //button:present-
        if ( numEnabled > 1 ) numEnabled--;
        doShift=0;
        break;
      case 0x96:  //button:present+
        if ( numEnabled < numRGBleds ) numEnabled++;
        doShift=0;
        break;
      case 0xCD6:  //button:shift
        doShift = 0;
        break;
      case 0x281:  //button:mute
        if ( mode == 2 ) mode = 1;
        else if ( mode == 1 ) mode = 2;
        break;
       case 0x481:  //button:vol+
        if ( shiftLoops < 10000 )
          shiftLoops += 50;
        break;
       case 0xC81:  //button:vol-
         if ( shiftLoops > 50 )
        shiftLoops -= 50;
        break;
      default:      
        break;
    }        
    
    irrecv.resume(); // continue receiving
  }
  
  
  //Serial.println("mode=");
  //Serial.println(mode);
  if ( mode == 2 )
  {
      ShiftPWM.SetAll(0);
  } else if ( mode == 1 )
  {
      if ( doShift <= 0 )
      {
        for ( int i = 0 ; i < numRGBleds ; i++ )
        {
          enabledLeds[i] = 0;
        }
        for ( int i = 0 ; i < numEnabled ; i++ )
        {
            int pos = random(numRGBleds);
            if ( enabledLeds[pos] == 1 )
            {
              i--;
            } else {
              enabledLeds[pos] = 1;
              // change color only if currently not enabled
              if ( currentLeds[pos] < 1 )                
                colors[pos] = (int)random(12);                                
            }
        }
          
        int shiftComplete = numEnabled;
        while ( shiftComplete != 0 )
        {
            shiftComplete=0;
            //Serial.println(shiftComplete);
            for ( int i = 0 ; i < numRGBleds ; i++ )
            {
                if ( enabledLeds[i] == 1 && currentLeds[i] < 255 )
                {                    
                    currentLeds[i]++;
                    shiftComplete++;                    
                } else if ( enabledLeds[i] == 0 && currentLeds[i] > 0 )
                {
                    currentLeds[i]--;
                    shiftComplete++;
                }
                if ( currentLeds[i] > 0 )
                {
                    int rgbc[3];
                    colorToRGB(colors[i],&rgbc[0]);
                    float factor = currentLeds[i]/255.0;
                    ShiftPWM.SetRGB(ledmap[i], rgbc[0]*factor, rgbc[1]*factor, rgbc[2]*factor);
                } else {
                    ShiftPWM.SetRGB(ledmap[i], 0, 0, 0);
                }
            }
            delay(fadeDelay);
        }
        doShift = shiftLoops;
      } else {
        doShift--;
        delay(shiftDelay);
      }
  }
  
  
}

/**
 * Colors... 0-11 -> RGB
 */
void colorToRGB(int color, int *rcolor)
{
  
  switch ( color )
  {
    case 0:
      rcolor[0] = 255;
      rcolor[1] = 255;
      rcolor[2] = 128;
      break;
    case 1:
      rcolor[0] = 255;
      rcolor[1] = 0;
      rcolor[2] = 0;
      break;
    case 2:
      rcolor[0] = 0;
      rcolor[1] = 255;
      rcolor[2] = 0;
      break;
    case 3:
      rcolor[0] = 168;
      rcolor[1] = 168;
      rcolor[2] = 0;
      break;
    case 4:
      rcolor[0] = 168;
      rcolor[1] = 0;
      rcolor[2] = 128;
      break;
    case 5:
      rcolor[0] = 168;
      rcolor[1] = 96;
      rcolor[2] = 128;
    case 6:
      rcolor[0] = 255;
      rcolor[1] = 168;
      rcolor[2] = 128;
      break;
    case 7:
      rcolor[0] = 96;
      rcolor[1] = 168;
      rcolor[2] = 128;
      break;
    case 8:
      rcolor[0] = 128;
      rcolor[1] = 128;
      rcolor[2] = 64;
      break;
    case 9:
      rcolor[0] = 168;
      rcolor[1] = 96;
      rcolor[2] = 128;
      break;
    case 10:
      rcolor[0] = 168;
      rcolor[1] = 128;
      rcolor[2] = 128;
      break;
    case 11:
      rcolor[0] = 128;
      rcolor[1] = 168;
      rcolor[2] = 128;
      break;  
  }
  
}


