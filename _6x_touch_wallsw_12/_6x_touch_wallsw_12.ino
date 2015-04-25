//***********************************************************************
// V 1.2 - april 2015
//
// TODO:  
//       
//***********************************************************************

#include "mpr121.h"
#include <Wire.h>
#include "LedControlMS.h"
#include <MySensor.h>
#include <SPI.h>
#include <MyBuffer.h>

#define CHILD_ID1 1   // first child
#define CHILD_NUM 6    // number of switches


//define working pins
 #define BUZZER 7   // buzzer pin
 #define DIN    6   // pin 6 is connected to the DataIn 
 #define CLK    4   // pin 4 is connected to the CLK 
 #define LOD    8   // pin 8 is connected to LOAD 
 #define IRQ    3   // irq pin for touch control
 #define BAKLIT 5   // pin PWM for backlight leds
 
// ********* MyBuffer ***********************************************************
MyBuffer buffer;               // define a new buffer
long previousMillis = 0;       // will store last time buffer was processed
long interval = 1000;           // interval at which to check buffer
// ******************************************************************************

MySensor gw;

//define a new led control
LedControl lc=LedControl(DIN,CLK,LOD,1);

//int irqpin = 3;  // Digital pin IRQ
boolean touchStates[12]; //to keep track of the previous touch states
boolean touchMemory[12]; //to keep track of the touch switch states

// correspondence between touch and led pins
int mapping[2][6]={{0,1,2,3,4,5}     // touch pins
                  ,{3,4,5,2,0,1}};   // led pins

int sound = 500;
int touchDelay=500;

void setup(){
  // init LED backlight
  pinMode(BAKLIT, OUTPUT);
  analogWrite(BAKLIT, 5);
  
  pinMode(IRQ, INPUT);
  digitalWrite(IRQ, HIGH); //enable pullup resistor
  Wire.begin();
  pinMode(BUZZER, OUTPUT);
  
  for (int i=0; i < 12; i++){
    touchMemory[i]=false;
  }
  mpr121_setup();
  
  //initialize MAX72XX
  lc.shutdown(0,false);
  lc.setIntensity(0,15);
  lc.clearDisplay(0);
  
  gw.begin(incomingMessage, AUTO); //do not make relay node
  gw.sendSketchInfo("WallTouch", "1.1");
  for (int i=0; i<CHILD_NUM;i++) {
   
    gw.present(CHILD_ID1+i, S_LIGHT);
    
    // Set touch button leds to last known state (using eeprom storage) 
    touchMemory[i] = gw.loadState(i)?true:false;
    setLed(mapping[1][i],touchMemory[i]);
  }
}

void loop(){
  gw.process();
  //watch for touch events
  readTouchInputs();
  //check for buffer items once in a while
  checkTime();
}

void checkTime(){
   unsigned long currentMillis = millis();
   if(currentMillis - previousMillis > interval) {
        previousMillis = currentMillis;  
        processBuffer();    
      }
}

void incomingMessage(const MyMessage &message) {
  buffer.push(message.sensor,message.type,message.getString());
}

//gets message from buffer if exists
void processBuffer(){
    if(!buffer.isEmpty()){
        String msg=buffer.pop();
        
        int mIndex = msg.indexOf(';');
        int secondmIndex = msg.indexOf(';', mIndex+1);
  
        String firstValue = msg.substring(0, mIndex);
        String secondValue = msg.substring(mIndex+1, secondmIndex);
        String thirdValue = msg.substring(secondmIndex+1);
        
        int sensor = firstValue.toInt();
        int type = secondValue.toInt();
        String data = thirdValue;
  
        Serial.println("    >> Process MSG: s:"+ firstValue +", t:"+secondValue+", d:"+thirdValue);
        
        processMsg(sensor, type, data);
    }
}

//process message from queue
void processMsg(int sensor, int type, String data){
  boolean msg;
   switch(type){
        case V_LIGHT: 
          if(sensor==254 && data=="RESET"){
            
              softReset();
            
          }else {//comando para 1 rele

             msg=data.toInt()?1:0;
             // Store state in eeprom
             gw.saveState(sensor, msg);      
             // Write some debug info
             Serial.print("--> Incoming change for child:");
             Serial.print(sensor-1);
             Serial.print(", New status: ");
             Serial.println(msg);
             
             //pasar parametro para ponerlo en ON y OFF real segun mensaje y no solo toggle
             setTouchButton(sensor-1, msg);
             }
          break;

      }
}


void setTouchButton(int pressed, int value){
     MyMessage msg(CHILD_ID1 + pressed,V_LIGHT);
      
     Serial.print("  pin ");
     Serial.print(pressed);
     Serial.println(" was just touched");
     Serial.print("  was: ");
     Serial.print(touchMemory[pressed]);
     Serial.print(",  set to: ");
     
     if(value == -1 || touchMemory[pressed] != value){ //only process if touch event or command different from state
          toggleTouch(pressed);
          gw.saveState(pressed, touchMemory[pressed]);
         }
         
     gw.send(msg.set(touchMemory[pressed] ? 1 : 0));
}

void toggleTouch(int sensor){
  Serial.print(", toggle to: ");
  
  touchMemory[sensor]=touchMemory[sensor]?false:true;
  setLed(mapping[1][sensor],touchMemory[sensor]?true:false);
  
  Serial.print(touchMemory[sensor]);
  Serial.println(touchMemory[sensor]?", ON":", OFF");
  
  touchSound(touchMemory[sensor]);
}


void setLed(int led, boolean lit){
  lc.setLed(0,0,led,lit);
}

void readTouchInputs(){
  if(!checkInterrupt()){
    
    //read the touch state from the MPR121
    Wire.requestFrom(0x5A,2); 
    
    byte LSB = Wire.read();
    byte MSB = Wire.read();
    
    uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states

    
    for (int i=0; i < 12; i++){  // Check what electrodes were pressed
      if(touched & (1<<i)){
      
        if(touchStates[i] == 0){
           //pin i was just touched
          setTouchButton(i, -1);  
          delay(touchDelay);
          }else if(touchStates[i] == 1){
            //pin i is still being touched
          }  
      
        touchStates[i] = 1;      
      }else{
       /* if(touchStates[i] == 1){
          Serial.print("pin ");
          Serial.print(i);
        Serial.println(" is no longer being touched");
          
          //pin i is no longer being touched
       }*/
        
        touchStates[i] = 0;
      }
    
    }
  }
}

void touchSound(boolean type){
  if(type){ //on sound
    tone(BUZZER, sound, 3);
  }else{//off sound
    tone(BUZZER, sound, 3);
    delay(80);
    tone(BUZZER, sound, 3);
  }
}


void mpr121_setup(void){

  set_register(0x5A, ELE_CFG, 0x00); 
  
  // Section A - Controls filtering when data is > baseline.
  set_register(0x5A, MHD_R, 0x01);
  set_register(0x5A, NHD_R, 0x01);
  set_register(0x5A, NCL_R, 0x00);
  set_register(0x5A, FDL_R, 0x00);

  // Section B - Controls filtering when data is < baseline.
  set_register(0x5A, MHD_F, 0x01);
  set_register(0x5A, NHD_F, 0x01);
  set_register(0x5A, NCL_F, 0xFF);
  set_register(0x5A, FDL_F, 0x02);
  
  // Section C - Sets touch and release thresholds for each electrode
  set_register(0x5A, ELE0_T, TOU_THRESH);
  set_register(0x5A, ELE0_R, REL_THRESH);
 
  set_register(0x5A, ELE1_T, TOU_THRESH);
  set_register(0x5A, ELE1_R, REL_THRESH);
  
  set_register(0x5A, ELE2_T, TOU_THRESH);
  set_register(0x5A, ELE2_R, REL_THRESH);
  
  set_register(0x5A, ELE3_T, TOU_THRESH);
  set_register(0x5A, ELE3_R, REL_THRESH);
  
  set_register(0x5A, ELE4_T, TOU_THRESH);
  set_register(0x5A, ELE4_R, REL_THRESH);
  
  set_register(0x5A, ELE5_T, TOU_THRESH);
  set_register(0x5A, ELE5_R, REL_THRESH);
  /*
  set_register(0x5A, ELE6_T, TOU_THRESH);
  set_register(0x5A, ELE6_R, REL_THRESH);
  
  set_register(0x5A, ELE7_T, TOU_THRESH);
  set_register(0x5A, ELE7_R, REL_THRESH);
  
  set_register(0x5A, ELE8_T, TOU_THRESH);
  set_register(0x5A, ELE8_R, REL_THRESH);
  
  set_register(0x5A, ELE9_T, TOU_THRESH);
  set_register(0x5A, ELE9_R, REL_THRESH);
  
  set_register(0x5A, ELE10_T, TOU_THRESH);
  set_register(0x5A, ELE10_R, REL_THRESH);
  
  set_register(0x5A, ELE11_T, TOU_THRESH);
  set_register(0x5A, ELE11_R, REL_THRESH);
  */
  // Section D
  // Set the Filter Configuration
  // Set ESI2
 set_register(0x5A, FIL_CFG, 0x12);  //12 mas  menos
  
   //set_register(0x5A, FIL_CFG, 0x24);  //segundo filtro
 // set_register(0x5A, 0x5C, 0x28);    // primer filtro (mio)
  //ajuste de la corrente de carga de cada pin
   /*  set_register(0x5A, 0x5F, CDC_sensor[0]);   //el 0
      set_register(0x5A, 0x60, CDC_sensor[1]);   //el 1
      set_register(0x5A, 0x61, CDC_sensor[2]);   //el 2
      set_register(0x5A, 0x62, CDC_sensor[3]);   //el 3
      set_register(0x5A, 0x63, CDC_sensor[4]);   //el 4
      set_register(0x5A, 0x64, CDC_sensor[5]);   //el 5
  */
  
  
  // Section E
  // Electrode Configuration
  // Set ELE_CFG to 0x00 to return to standby mode
  set_register(0x5A, ELE_CFG, 0x0C);  // Enables all 12 Electrodes
  
  
  // Section F
  // Enable Auto Config and auto Reconfig
  /*set_register(0x5A, ATO_CFG0, 0x0B);
  set_register(0x5A, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(0x5A, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
  set_register(0x5A, ATO_CFGT, 0xB5);*/  // Target = 0.9*USL = 0xB5 @3.3V
  
  set_register(0x5A, ELE_CFG, 0x0C);
  
}


boolean checkInterrupt(void){
  return digitalRead(IRQ);
}


void set_register(int address, unsigned char r, unsigned char v){
    Wire.beginTransmission(address);
    Wire.write(r);
    Wire.write(v);
    Wire.endTransmission();
}

//reboot the sketch when called
void softReset() {
asm volatile ("  jmp 0");
}
