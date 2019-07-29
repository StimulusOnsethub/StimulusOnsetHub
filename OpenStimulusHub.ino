// Copyright @ 2019 Jacob G. Martin and Charles E. Davis.  All Rights Reserved.
//
// The names of Jacob G. Martin and Charles E. Davis may not be used to endorse or promote products
// derived from or associated with this software without specific prior written permission.
// 
// No products may be derived from or associated with this software without specific prior written permission.

// This software is provided "AS IS," without a warranty of any
// kind. ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND
// WARRANTIES, INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY
// EXCLUDED. Jacob G. Martin and/or Charles E. Davis SHALL NOT BE LIABLE FOR ANY
// DAMAGES OR LIABILITIES SUFFERED BY ANY ORGANIZATION OR ITS
// LICENSEES AS A RESULT OF OR RELATING TO USE, MODIFICATION OR DISTRIBUTION OF THIS
// SOFTWARE OR ITS DERIVATIVES. IN NO EVENT WILL Jacob G. Martin and/or Charles E. Davis BE LIABLE
// FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT,
// SPECIAL, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER
// CAUSED AND REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF
// THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF Jacob G. Martin and/or Charles E. Davis HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES AND THE USER OF THIS CODE
// AGREES TO HOLD Jacob G. Martin and Charles E. Davis HARMLESS THEREFROM.


#define FASTADC 1
#define LIGHTSENABLED 1
#define DIGITALWRITEFAST 1
#define DIGITALMICROPHONE 1
#define DEBUG 0
#define ADDA 0


#include <LiquidCrystal_I2C.h>

#if ADDA
  #include <Wire.h> 
  #define PCF8591 (0x90 >> 1) // I2C bus address
  #define AIn0 0x00
  #define AIn1 0x01
#endif

#if DIGITALWRITEFAST
  #include "digitalWriteFast.h"
#endif


// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


// LED setup
const int guiLEDPower = 10;     //  Red    (for power button)
const int guiLEDPhotodiode = 8; //  Blue   (photodiode active light)
const int guiLEDMicrophone = 9; //  Green  (microphone active light) 

// LCD setup
LiquidCrystal_I2C lcd(0x27,16,2); 

//-----------------------------
//          INPUTS
//-----------------------------
// 1 Photodiode 3.5mm jack 
// 1 Microphone 3.5mm input jack
// 1 Knob for photodiode sensitivity level
// 1 Knob for microphone sensitivity level

// 3.5mm photodiode jack setup
const int inputPhotodiode = A0;
volatile int inputPhotodiodeThreshold = 70;
volatile int oldAPhotodiode = HIGH;
volatile int oldBPhotodiode = HIGH;
volatile bool photodiodeLCDUpdate = false;

// 3.5mm microphone jack setup
const int inputMicrophone = 31;
volatile int inputMicrophoneThreshold = 650;
volatile int oldAMicrophone = HIGH;
volatile int oldBMicrophone = HIGH;
volatile bool microphoneLCDUpdate = false;

// Rotary encoder pins for photodiode sensitivity
const int inputPhotodiodeSensitivityClock         = 2; 
const int inputPhotodiodeSensitivityDisplacement  = 3; 
const int inputPhotodiodeSensitivitySwitch        = 4; 

// Rotary encoder pins for microphone sensistivity
const int inputMicrophoneSensitivityClock         = 19; 
const int inputMicrophoneSensitivityDisplacement  = 18; 
const int inputMicrophoneSensitivitySwitch        = 17; 


//-----------------------------
//          OUTPUTS
//-----------------------------
const int output_photodiode = 47; 
const int output_microphone = 49; 

bool lastMicrophoneLevel = LOW;
bool lastPhotodiodeLevel = LOW;

void setup() {
  
    //-----------------------------
    //       USER INTERFACE
    //-----------------------------
    // 1 LED for power
    // 1 LED for photodiode event detected
    // 1 LED for microphone event detected
    // 1 number screen for Photodiode sensitivity level AND Microphone sensitivity level

    #if FASTADC
    // set prescale 
        cbi(ADCSRA,ADPS2);
        cbi(ADCSRA,ADPS1);
        sbi(ADCSRA,ADPS0);
    #endif
    

    // LED setup
    pinMode(guiLEDPower, OUTPUT);
    pinMode(guiLEDMicrophone, OUTPUT);
    pinMode(guiLEDPhotodiode, OUTPUT);

    #if LIGHTSENABLED
        digitalWrite(guiLEDPower, HIGH);
    #endif

    // LCD setup
    lcd.init(); //initialize the lcd
    lcd.backlight(); //open the backlight 
    lcd.setCursor(0,0);  // set cursor position to start of first line on the LCD
    lcd.print("Photodiode:  ");
    lcd.print(inputPhotodiodeThreshold);
    lcd.setCursor(0,1);
    lcd.print("Microphone:  ");
    lcd.print(inputMicrophoneThreshold);


    //-----------------------------
    //          INPUTS
    //-----------------------------
    // 1 Photodiode 3.5mm jacks 
    // 1 Microphone input jack
    // 1 Knob for photodiode sensitivity level
    // 1 Knob for microphone sensitivity level

    // No setup needed for Analog pins (i.e. Photodiode and Microphone inputs are analog)
    pinMode(inputPhotodiode, INPUT);
    pinMode(inputMicrophone, INPUT_PULLUP);

    #if ADDA
        Wire.begin();
    #endif

    // Rotary encoder pins for photodiode sensitivity
    pinMode(inputPhotodiodeSensitivityClock, INPUT_PULLUP);
    pinMode(inputPhotodiodeSensitivityDisplacement, INPUT_PULLUP);
    pinMode(inputPhotodiodeSensitivitySwitch, INPUT_PULLUP);
    

    // Rotary encoder pins for microphone sensistivity
    pinMode(inputMicrophoneSensitivityClock, INPUT_PULLUP);
    pinMode(inputMicrophoneSensitivityDisplacement, INPUT_PULLUP);
    pinMode(inputMicrophoneSensitivitySwitch, INPUT_PULLUP);
    

    // Set up the rotary interrupts
    attachInterrupt(digitalPinToInterrupt(inputPhotodiodeSensitivityDisplacement),isrRotaryPhotodiodeSensitivity,CHANGE);
    attachInterrupt(digitalPinToInterrupt(inputMicrophoneSensitivityDisplacement),isrRotaryMicrophoneSensitivity,CHANGE);
    
    
    //-----------------------------
    //          OUTPUTS
    //-----------------------------
    
    // DB25 output 1 setup
    pinMode(output_photodiode, OUTPUT); 
    pinMode(output_microphone, OUTPUT); 

   
  
    Serial.begin(9600);  
}



void loop() {

  
    #if DEBUG
        unsigned long currentMicros = micros();
    #endif
    
    int photodiodeLevel = analogRead(inputPhotodiode);
    #if ADDA
        Wire.beginTransmission(PCF8591); // Start your PCF8591
        Wire.write(AIn0); // Tell it to make an Analog Measurement
        Wire.endTransmission(); //
        Wire.requestFrom(PCF8591, 1); // Get the Measured Data
        //
        int microphoneLevel = Wire.read();
    #else
        int microphoneLevel = digitalReadFast(inputMicrophone);      
    #endif
    #if DEBUG
        Serial.print("Microphone level: ");
        Serial.print(microphoneLevel);
        Serial.println("");   
    #endif

    
    // DETECT SOUND
    if ((photodiodeLevel > inputPhotodiodeThreshold)) {
        #if DEBUG
            Serial.print("Detected Light at level: ");
            Serial.print(photodiodeLevel);
            Serial.println("");
        #endif
        
        #if DIGITALWRITEFAST
            digitalWriteFast(output_photodiode,HIGH);
        #else
            digitalWrite(output_photodiode,HIGH);
        #endif
        lastPhotodiodeLevel = HIGH;
       
        #if LIGHTSENABLED
            #if DIGITALWRITEFAST
                digitalWriteFast(guiLEDPhotodiode,HIGH); //turn the LED on
            #else
                digitalWrite(guiLEDPhotodiode,HIGH); //turn the LED on
            #endif
        #endif
    }
    else if (lastPhotodiodeLevel == HIGH && (photodiodeLevel < inputPhotodiodeThreshold)) {
        #if DEBUG
            Serial.print("Not detected Light at level: ");
            Serial.print(photodiodeLevel);
            Serial.println("");
        #endif
        
        #if DIGITALWRITEFAST
            digitalWriteFast(output_photodiode,LOW);
        #else
            digitalWrite(output_photodiode,LOW);
        #endif
        lastPhotodiodeLevel = LOW;

        #if LIGHTSENABLED
            #if DIGITALWRITEFAST
                digitalWriteFast(guiLEDPhotodiode,LOW); 
            #else
                digitalWrite(guiLEDPhotodiode,LOW); 
            #endif
        #endif
    }

    // DETECT MICROPHONE
    #if DIGITALMICROPHONE
        if ((microphoneLevel == 0) && (lastMicrophoneLevel == LOW)) {   
    #else  
        if ((microphoneLevel > inputMicrophoneThreshold) && (lastMicrophoneLevel == LOW)) {        
    #endif

    #if DEBUG
        Serial.print("Detected sound at level: ");
        Serial.print(microphoneLevel);
        Serial.println("");
    #endif
    
    #if DIGITALWRITEFAST
        digitalWriteFast(output_microphone,HIGH);
    #else
        digitalWrite(output_microphone,HIGH);
    #endif

    lastMicrophoneLevel = HIGH;
    
    #if LIGHTSENABLED
        #if DIGITALWRITEFAST
            digitalWriteFast(guiLEDMicrophone,HIGH); //turn the LED on
        #else
            digitalWrite(guiLEDMicrophone,HIGH); //turn the LED on
        #endif
    #endif
    }
    #if DIGITALMICROPHONE
        else if ((microphoneLevel > 0 ) && (lastMicrophoneLevel == HIGH)) {         
    #else
        else if (lastMicrophoneLevel == HIGH && (microphoneLevel < inputMicrophoneThreshold)) {
    #endif
    
    #if DEBUG
        Serial.print("Not detected sound at level: ");
        Serial.print(microphoneLevel);
        Serial.println("");
    #endif
        
    #if DIGITALWRITEFAST
        digitalWriteFast(output_microphone,LOW);
    #else
        digitalWrite(output_microphone,LOW);
    #endif
    
    lastMicrophoneLevel = LOW;
      
    #if LIGHTSENABLED
        #if DIGITALWRITEFAST
            digitalWriteFast(guiLEDMicrophone,LOW); //turn the LED off
        #else
            digitalWrite(guiLEDMicrophone,LOW); //turn the LED off
        #endif
    #endif
    }                                                        

    // Check if we need to update the LCD:
    if (microphoneLCDUpdate) {
        updateLCDMicrophone();    
        microphoneLCDUpdate = false;
    }
    if (photodiodeLCDUpdate) {
        updateLCDPhotodiode();    
        photodiodeLCDUpdate = false;
    }
    #if DEBUG
        unsigned long elapsedTime = micros() - currentMicros;
        //  Serial.print("Elapsed time is: ");
        //  Serial.print(elapsedTime);
        //  Serial.println("");
    #endif
}

void serialEventRun() {}

void updateLCDPhotodiode() {
    //Serial.println("Updating LCD photo");
    lcd.setCursor(0,0);  // set cursor position to start of first line on the LCD
    lcd.print("Photodiode:  ");  //text to print
    lcd.print(inputPhotodiodeThreshold);
    if (inputPhotodiodeThreshold < 100)
        lcd.print(" ");
}
void updateLCDMicrophone() {
    //Serial.println("Updating LCD micro");
    lcd.setCursor(0,1);  // set cursor position to start of second line on the LCD
    lcd.print("Microphone:  ");  //text to print
    lcd.print(inputMicrophoneThreshold);
    if (inputMicrophoneThreshold < 100)
        lcd.print(" ");
}


void isrRotaryPhotodiodeSwitch() {
  
  //int switchvalue = digitalRead(inputPhotodiodeSensitivitySwitch);
  //Serial.println("fired");
  
  //if (switchvalue == HIGH) {
    //lightsEnabled = true;
  //}
  //else
    //lightsEnabled = false;
  //  lightsEnabled = !lightsEnabled;
  
    //#if LIGHTSENABLED
    //  digitalWrite(guiLEDPower, HIGH);
    //else
    //  digitalWrite(guiLEDPower, LOW);
  //}
  
}

void isrRotaryPhotodiodeSensitivity() {
    int pinA = digitalRead(inputPhotodiodeSensitivityClock);
    int pinB = digitalRead(inputPhotodiodeSensitivityDisplacement);
    int turn = -1;
    int result = 0;
    //  Serial.println("Turning photo");
    if (pinA != oldAPhotodiode && pinB != oldBPhotodiode) {
        if (oldAPhotodiode == HIGH && pinA == LOW) {
            result = (oldBPhotodiode * 2 -1);
    //         Serial.println(result);
        }
    }
    oldAPhotodiode = pinA;
    oldBPhotodiode = pinB;
    if (result == 0)
        return;
    turn = result * -10;

    int photodiodeSensitivityController = turn;
    if (photodiodeSensitivityController != 0) {
        //Serial.println(photodiodeSensitivityController);
        if (inputPhotodiodeThreshold + photodiodeSensitivityController > 0 && inputPhotodiodeThreshold + photodiodeSensitivityController < 1000) {
        inputPhotodiodeThreshold = inputPhotodiodeThreshold + photodiodeSensitivityController;
        photodiodeLCDUpdate = true;
        }
    }
}

void isrRotaryMicrophoneSensitivity() {
    int pinA = digitalRead(inputMicrophoneSensitivityClock);
    int pinB = digitalRead(inputMicrophoneSensitivityDisplacement);
    //Serial.println("Turning mciro");
    int turn = -1;
    int result = 0;
    if (pinA != oldAMicrophone && pinB != oldBMicrophone) {
        if (oldAMicrophone == HIGH && pinA == LOW) {
            result = (oldBMicrophone * 2 -1);
        //   Serial.println(result);
        }
    }
    oldAMicrophone = pinA;
    oldBMicrophone = pinB;
    if (result == 0)
    return;
    
    turn = result * 10;

    //int photodiodeSensitivityController = getEncoderTurn(inputMicrophoneSensitivityClock,inputMicrophoneSensitivityDisplacement) * -1;
    int microphoneSensitivityController = turn;
    if (microphoneSensitivityController != 0) {
        //Serial.println(photodiodeSensitivityController);
        if (inputMicrophoneThreshold + microphoneSensitivityController > 0 && inputMicrophoneThreshold + microphoneSensitivityController < 1000) {
            inputMicrophoneThreshold = inputMicrophoneThreshold + microphoneSensitivityController;
            microphoneLCDUpdate = true;
        }
    }

}

int getEncoderTurn(int clockPin, int displacementPin)
{
    static int oldA = HIGH; //set the oldA as HIGH
    static int oldB = HIGH; //set the oldB as HIGH
    int result = 0;
    int newA = digitalRead(clockPin);        //read the value of clkPin to newA
    int newB = digitalRead(displacementPin); //read the value of dtPin to newB
    if (newA != oldA || newB != oldB)  {      //if the value of clkPin or the dtPin has changed
        // something has changed
        if (oldA == HIGH && newA == LOW)
        {
            result = (oldB * 2 - 1);
        }
    }
    oldA = newA;
    oldB = newB;
    return result;
}
