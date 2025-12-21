#include "Wire.h"
#include "Adafruit_PWMServoDriver.h"
#include "DFRobotDFPlayerMini.h"
#include "MX1508.h"

// I2C Definitions
#define PICO_ADDR 0x05
unsigned long lastI2CRequest = 0;
uint32_t controller_state = 0;
char button_press = '\0';

// Motor Definitions
Adafruit_PWMServoDriver Body_Servo_Driver = Adafruit_PWMServoDriver(0x40, Wire);
Adafruit_PWMServoDriver Dome_Servo_Driver = Adafruit_PWMServoDriver(0x41, Wire); //Second Driver

int DomeMotor1  = 10;
int DomeMotor2  = 11;
int LeftMotor1  = 6;
int LeftMotor2  = 9;
int RightMotor1 = 5;
int RightMotor2 = 3;

int MotorSpeed = 255;

MX1508 left_motor(LeftMotor1, LeftMotor2, SLOW_DECAY, 2);
MX1508 right_motor(RightMotor1, RightMotor2, SLOW_DECAY, 2);
MX1508 dome_motor(DomeMotor1, DomeMotor2, SLOW_DECAY, 2);


// Controller Definitions
bool lastTriggerStateLeft          = false; 
bool lastTriggerStateRight         = false;
bool lastTriggerStateRightReverse  = false;
bool lastTriggerStateLeftReverse   = false;
bool lastTriggerStateDomeL         = false;
bool lastTriggerStateDomeR         = false;

// Time
float Elapsed_Time;
float Previous_Time;
float Time;

unsigned long Current_Millis; //current time
unsigned long Dome_Red_LED_1_Millis   = 0;
unsigned long Dome_Red_LED_2_Millis   = 0;
unsigned long Dome_Blue_LED_1_Millis  = 0;
unsigned long Dome_Blue_LED_2_Millis  = 0;
unsigned long Dome_White_LED_1_Millis = 0;
unsigned long Dome_White_LED_2_Millis = 0;
unsigned long Dome_White_LED_3_Millis = 0;
unsigned long Dome_Holo_Top_Millis    = 0;
unsigned long Dome_Holo_Back_Millis   = 0;
unsigned long Dome_Holo_Front_Millis  = 0;
unsigned long Dome_Periscope_Millis   = 0;

unsigned long Body_Top_Grabber_Millis = 0;
unsigned long Body_Bot_Grabber_Millis = 0;
unsigned long Body_Third_Leg_Millis   = 0;
unsigned long Body_Legs_Millis        = 0;

// Dome Variables
int Periscope_Up           = 200;
int Periscope_Down         = 370;
int Holo_High_Upper        = 300;
int Holo_High_Lower        = 370;
int Holo_Low_Upper         = 280;
int Holo_Low_Lower         = 100;

int Periscope_State        = 0; //0 => Down, 1 => Up
int Holo_Top_State         = 0;
int Holo_Back_State        = 0;
int Holo_Front_State       = 0;
int Dome_Red_LED_1_State   = 0;
int Dome_Blue_LED_1_State  = 0;
int Dome_Blue_LED_2_State  = 0;
int Dome_White_LED_1_State = 0;
int Dome_White_LED_2_State = 0;
int Dome_White_LED_3_State = 0;

//Body Variables
int Top_Grabber_State      = 0; //0 => In, 1 => Out
int Bot_Grabber_State      = 0; 
int Third_Leg_State        = 0;
int Legs_State             = 0;

int Top_Grabber_Out        = 300;
int Bot_Grabber_Out        = 300;
int Top_Grabber_In         = 80;
int Bot_Grabber_In         = 80;
int Third_Leg_Out          = 90;
int Third_Leg_In           = 285;
int Legs_Out               = 90;
int Legs_In                = 295;

/////////////////////////////// SETUP //////////////////////////////////
void setup()
{
  Wire.begin();              
  Serial.begin(9600);

  //Dome Setup
  Dome_Servo_Driver.begin();
  Dome_Servo_Driver.setPWMFreq(50);
  Dome_Servo_Driver_Setup(); 

  //Body Setup
  Body_Servo_Driver.begin();
  Body_Servo_Driver.setPWMFreq(50);
  Body_Servo_Driver_Setup(); 

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(LeftMotor1, OUTPUT);
  pinMode(LeftMotor2, OUTPUT);
  pinMode(RightMotor1, OUTPUT);
  pinMode(RightMotor2, OUTPUT);
  pinMode(DomeMotor1, OUTPUT);
  pinMode(DomeMotor2, OUTPUT);

  left_motor.motorGo(0);
  right_motor.motorGo(0);
  dome_motor.motorGo(0);

  delay(1000);
  Serial.println("Ready!");
}

/////////////////////////////// LOOP //////////////////////////////////
void loop()
{
  
  Current_Millis = millis();
  Previous_Time = Time;
  Time = millis();
  Elapsed_Time = Time - Previous_Time;
  Dome();

  if (millis() - lastI2CRequest > 30) // every 30ms
  {  
    Wire.requestFrom(PICO_ADDR, 4);

    if (Wire.available() == 4) 
    {
      controller_state  = Wire.read();
      controller_state |= Wire.read() << 8;
      controller_state |= Wire.read() << 16;
      controller_state |= Wire.read() << 24;
    }

    lastI2CRequest = millis();
  }

  // Printing Controller Inputs
  if (controller_state & (1 << 0))  Serial.println("A pressed");
  if (controller_state & (1 << 1))  Serial.println("B pressed");
  if (controller_state & (1 << 2))  Serial.println("X pressed");
  if (controller_state & (1 << 3))  Serial.println("Y pressed");
  if (controller_state & (1 << 4))  Serial.println("Left Shoulder pressed");
  if (controller_state & (1 << 5))  Serial.println("Right Shoulder pressed");
  if (controller_state & (1 << 6))  Serial.println("Right Trigger pressed");
  if (controller_state & (1 << 7))  Serial.println("Left Trigger pressed");
  if (controller_state & (1 << 8))  Serial.println("Dpad Up pressed");
  if (controller_state & (1 << 9))  Serial.println("Dpad Left pressed");
  if (controller_state & (1 << 10)) Serial.println("Dpad Down pressed");
  if (controller_state & (1 << 11)) Serial.println("Dpad Right pressed");
  if (controller_state & (1 << 12)) Serial.println("Left Joystick Up pressed");
  if (controller_state & (1 << 13)) Serial.println("Left Joystick Left pressed");
  if (controller_state & (1 << 14)) Serial.println("Left Joystick Down pressed");
  if (controller_state & (1 << 15)) Serial.println("Left Joystick Right pressed");
  if (controller_state & (1 << 16)) Serial.println("Right Joystick Up pressed");
  if (controller_state & (1 << 17)) Serial.println("Right Joystick Left pressed");
  if (controller_state & (1 << 18)) Serial.println("Right Joystick Down pressed");
  if (controller_state & (1 << 19)) Serial.println("Right Joystick Right pressed");
  
  process_button_press();

}

/////////////////////////////// PROCESS BUTTON PRESS //////////////////////////////////
void process_button_press()
{

  //////////////////// A Button: Control Body Grabber 1 /////////////////
  if (controller_state & (1 << 0))
  {
    if(Top_Grabber_State == 0 && Current_Millis - Body_Top_Grabber_Millis >= 500)
    {
      Top_Grabber_State = 1;
      Body_Servo_Driver.setPWM(0, 0, Top_Grabber_In);
      Body_Top_Grabber_Millis = Current_Millis;
      
    }
    else if(Current_Millis - Body_Top_Grabber_Millis >= 500)
    {
      Top_Grabber_State = 0;
      Body_Servo_Driver.setPWM(0, 0, Top_Grabber_Out);
      Body_Top_Grabber_Millis = Current_Millis;
      
    }
  }

  //////////////////// Y Button: Control Body Grabber 2 /////////////////
  if (controller_state & (1 << 3))
  {
    if(Bot_Grabber_State == 0 && Current_Millis - Body_Bot_Grabber_Millis >= 500)
    {
      Bot_Grabber_State = 1;
      Body_Servo_Driver.setPWM(1, 0, Bot_Grabber_In);
      Body_Bot_Grabber_Millis = Current_Millis;
      
    }
    else if(Current_Millis - Body_Bot_Grabber_Millis >= 500)
    {
      Bot_Grabber_State = 0;
      Body_Servo_Driver.setPWM(1, 0, Bot_Grabber_Out);
      Body_Bot_Grabber_Millis = Current_Millis;
      
    }
  }

  //////////////////// B Button: Control Dome Periscope /////////////////
  if (controller_state & (1 << 1))
  {
    if(Periscope_State == 0 && Current_Millis - Dome_Periscope_Millis >= 500)
    {
      Periscope_State = 1;
      Dome_Servo_Driver.setPWM(3, 0, Periscope_Up);
      Dome_Periscope_Millis = Current_Millis;
      
    }
    else if(Current_Millis - Dome_Periscope_Millis >= 500)
    {
      Periscope_State = 0;
      Dome_Servo_Driver.setPWM(3, 0, Periscope_Down);
      Dome_Periscope_Millis = Current_Millis;
    
    }
  
  }

  /////////// X Button: control third leg ///////////////////////////
  if (controller_state & (1 << 2))
  {
    //Uncomment if you want to use
    /*
    if(Third_Leg_State == 0 && Current_Millis - Body_Third_Leg_Millis >= 1000)
    {
      Third_Leg_State = 1;
      Body_Servo_Driver.setPWM(3, 0, Legs_Out);
      Body_Servo_Driver.setPWM(2, 0, Third_Leg_Out);
      Body_Third_Leg_Millis = Current_Millis;
    }
    else if(Current_Millis - Body_Third_Leg_Millis >= 1000)
    {
      Third_Leg_State = 0;
      Body_Servo_Driver.setPWM(3, 0, Legs_In);
      Body_Servo_Driver.setPWM(2, 0, Third_Leg_In);
      Body_Third_Leg_Millis = Current_Millis;
    }
    */
  }

  //////////////////// Left Trigger: Left Leg Control /////////////////

  bool triggerPressedLeft = controller_state & (1 << 7);

  if (triggerPressedLeft && !lastTriggerStateLeft) 
  {
    left_motor.motorGo(-MotorSpeed);
  }
  else if (!triggerPressedLeft && lastTriggerStateLeft) 
  {
    left_motor.motorGo(0);
  }

  lastTriggerStateLeft = triggerPressedLeft;

  //////////////////// Right Trigger: Right Leg Control /////////////////

  bool triggerPressedRight = controller_state & (1 << 6);

  if (triggerPressedRight && !lastTriggerStateRight) 
  {  
    right_motor.motorGo(-MotorSpeed);
  }
  else if (!triggerPressedRight && lastTriggerStateRight) 
  {
    right_motor.motorGo(0);
  }

  lastTriggerStateRight = triggerPressedRight;


  //////////////////// Left Bumper: Left Leg  Reverse Control /////////////////

  bool triggerPressedLeftReverse = controller_state & (1 << 4);

  if (triggerPressedLeftReverse && !lastTriggerStateLeftReverse) 
  {
    left_motor.motorGo(MotorSpeed);
  }
  else if (!triggerPressedLeftReverse && lastTriggerStateLeftReverse) 
  {
    left_motor.motorGo(0);
  }

  lastTriggerStateLeftReverse = triggerPressedLeftReverse;

  //////////////////// Right Bumper: Right Leg  Reverse Control /////////////////
 
  bool triggerPressedRightReverse = controller_state & (1 << 5);

  if (triggerPressedRightReverse && !lastTriggerStateRightReverse) 
  {
    right_motor.motorGo(MotorSpeed);
  }
  else if (!triggerPressedRightReverse && lastTriggerStateRightReverse) 
  {
    right_motor.motorGo(0);
  }

  lastTriggerStateRightReverse = triggerPressedRightReverse;

  //////////////////// Left Dpad: Dome Rotate Left /////////////////

  bool triggerPressedDomeL = controller_state & (1 << 9);

  if (triggerPressedDomeL && !lastTriggerStateDomeL) 
  {
    dome_motor.motorGo(150);
  }
  else if (!triggerPressedDomeL && lastTriggerStateDomeL) 
  {
    dome_motor.motorGo(0);
  }

  lastTriggerStateDomeL = triggerPressedDomeL;

  //////////////////// Right Dpad: Dome Rotate Right /////////////////

  bool triggerPressedDomeR = controller_state & (1 << 11);

  if (triggerPressedDomeR && !lastTriggerStateDomeR) 
  {
    dome_motor.motorGo(-150);
  }
  else if (!triggerPressedDomeR && lastTriggerStateDomeR) 
  {
    dome_motor.motorGo(0);
  }

  lastTriggerStateDomeR = triggerPressedDomeR;

}

/////////////////////////////// DOME //////////////////////////////////
void Dome()
{
  //// Red LED pin 8, blink (Back of Dome) ////
  if(Current_Millis - Dome_Red_LED_1_Millis >= 2000)
  {
    Dome_Red_LED_1_Millis = Current_Millis;
    if(Dome_Red_LED_1_State == 0)
    {
        Dome_Red_LED_1_State = 1;
        Dome_Servo_Driver.setPWM(8, 0, 4095);
    }
    else
    {
        Dome_Red_LED_1_State = 0;
        Dome_Servo_Driver.setPWM(8, 0, 4096);
    }
  }

  //// Red LED pin 9, always on (Front of Dome) ////
  Dome_Servo_Driver.setPWM(9, 0, 4095);

  //// Blue LEDs pins 11 and 12 (front screens) //////
  if(Current_Millis - Dome_Blue_LED_1_Millis >= random(100, 2000))
  {
    Dome_Blue_LED_1_Millis = Current_Millis;
    if(Dome_Blue_LED_1_State == 0)
    {
        Dome_Blue_LED_1_State = 1;
        Dome_Servo_Driver.setPWM(11, 0, 4095);
    }
    else
    {
        Dome_Blue_LED_1_State = 0;
        Dome_Servo_Driver.setPWM(11, 0, 4096);
    }
  }

  if(Current_Millis - Dome_Blue_LED_2_Millis >= random(300, 3000))
  {
    Dome_Blue_LED_2_Millis = Current_Millis;
    if(Dome_Blue_LED_2_State == 0)
    {
        Dome_Blue_LED_2_State = 1;
        Dome_Servo_Driver.setPWM(12, 0, 4095);

    }
    else
    {
        Dome_Blue_LED_2_State = 0;
        Dome_Servo_Driver.setPWM(12, 0, 4096);

    }
  }

  //// 3 White LED's Pins 13 - 15 (Back Bar) ////
  if(Current_Millis - Dome_White_LED_1_Millis >= random(300, 1000))
  {
    Dome_White_LED_1_Millis = Current_Millis;
    if(Dome_White_LED_1_State == 0)
    {
        Dome_White_LED_1_State = 1;
        Dome_Servo_Driver.setPWM(13, 0, 4095);

    }
    else
    {
        Dome_White_LED_1_State = 0;
        Dome_Servo_Driver.setPWM(13, 0, 4096);

    }
  }

  if(Current_Millis - Dome_White_LED_2_Millis >= random(200, 3000))
  {
    Dome_White_LED_2_Millis = Current_Millis;
    if(Dome_White_LED_2_State == 0)
    {
        Dome_White_LED_2_State = 1;
        Dome_Servo_Driver.setPWM(14, 0, 4095);

    }
    else
    {
        Dome_White_LED_2_State = 0;
        Dome_Servo_Driver.setPWM(14, 0, 4096);

    }
  }
  if(Current_Millis - Dome_White_LED_3_Millis >= random(300, 3000))
  {
    Dome_White_LED_3_Millis = Current_Millis;
    if(Dome_White_LED_3_State == 0)
    {
        Dome_White_LED_3_State = 1;
        Dome_Servo_Driver.setPWM(15, 0, 4095);
    }
    else
    {
        Dome_White_LED_3_State = 0;
        Dome_Servo_Driver.setPWM(15, 0, 4096);
    }
  }

  if(Current_Millis - Dome_Holo_Top_Millis >= random(4000, 10000))
  {
    Dome_Holo_Top_Millis = Current_Millis;
    if(Holo_Top_State == 0)
    {
        Holo_Top_State = 1;
        Dome_Servo_Driver.setPWM(2, 0, random(Holo_High_Lower, Holo_High_Upper));
    }
    else
    {
        Holo_Top_State = 0;
        Dome_Servo_Driver.setPWM(2, 0, random(Holo_Low_Lower, Holo_Low_Upper));
    }
  }

  if(Current_Millis - Dome_Holo_Front_Millis >= random(3000, 8000))
  {
    Dome_Holo_Front_Millis = Current_Millis;
    if(Holo_Front_State == 0)
    {
        Holo_Front_State = 1;
        Dome_Servo_Driver.setPWM(1, 0, random(Holo_High_Lower, Holo_High_Upper));

    }
    else
    {
        Holo_Front_State = 0;
        Dome_Servo_Driver.setPWM(1, 0, random(Holo_Low_Lower, Holo_Low_Upper));

    }
  }

  if(Current_Millis - Dome_Holo_Back_Millis >= random(5000, 10000))
  {
    Dome_Holo_Back_Millis = Current_Millis;
    if(Holo_Back_State == 0)
    {
        Holo_Back_State = 1;
        Dome_Servo_Driver.setPWM(0, 0, random(Holo_High_Lower, Holo_High_Upper));

    }
    else
    {
        Holo_Back_State = 0;
        Dome_Servo_Driver.setPWM(0, 0, random(Holo_Low_Lower, Holo_Low_Upper));

    }
  }

}

/////////////////////////////// DOME SETUP //////////////////////////////////
// Setup the LED's and Servo's in the dome
// Set all LED's to initally be off
void Dome_Servo_Driver_Setup()
{
  //Servo's
  Dome_Servo_Driver.setPWM(0,0,Periscope_Down); //Holo Projector 1
  Dome_Servo_Driver.setPWM(1,0,Periscope_Down); //Holo Projector 2
  Dome_Servo_Driver.setPWM(2,0,Periscope_Down); //Holo Projector 3
  Dome_Servo_Driver.setPWM(3,0,Periscope_Down); //Periscope Servo

  //LED's
  for(int i = 4; i < 16; ++i)
  {
    Dome_Servo_Driver.setPWM(i, 0, 4096);//Fully Off
  }
}

/////////////////////////////// BODY SETUP //////////////////////////////////
void Body_Servo_Driver_Setup()
{
  Body_Servo_Driver.setPWM(0,0,Top_Grabber_In);
  Body_Servo_Driver.setPWM(1,0,Bot_Grabber_In);
  //Body_Servo_Driver.setPWM(2,0,Third_Leg_In); //Uncomment if you want to use
  //Body_Servo_Driver.setPWM(3,0,Legs_In); //Uncomment if you want to use
}