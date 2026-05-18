#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>       
#include <TouchScreen.h>
#include <BH1750.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_AM2320.h>
  

#define YP 12 
#define XM 11 
#define YM 1  
#define XP 2  

#define I2C_SDA 37
#define I2C_SCL 36

#define Peliter_pin 48
#define NEOs_pin 47
#define FAN1_pin 16
#define FAN2_pin 17
#define PUMP_pin 21
#define Soil_pin 35
#define ServoL_pin 14
#define ServoR_pin 17

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TFT_eSPI tft = TFT_eSPI();
BH1750 lightMeter;
Adafruit_AM2320 am;
Servo ServoL;
Servo ServoR;
Adafruit_NeoPixel pixels(48, NEOs_pin, NEO_GRB + NEO_KHZ800);
float currentTemp = 0;
float currentMoist = 0;
float currentLux = 0;

bool tempStatus = false;
bool moistStatus = false;
bool lightStatus = false;

enum SYSMOD{MENU, ARGULA, SPINACH};

SYSMOD sysmod = MENU;

//Parameters  and limits for the feedback system

int mode = 0; //0 for spinach, 1 for argula

float targetTemp = 0;
float min_temp = 0; 
float max_temp = 0;
float targetMoist = 0;
float targetlux = 0; 
float min_lux = 0;
float max_lux = 0;
int irrigation_time = 0;

float Argula_targetTemp = 20.0;
float Argula_min_temp = 18.0; 
float Argula_max_temp = 22.0;
float Argula_targetMoist = 60.0;
float Argula_targetlux = 10800; 
float Argula_min_lux = 9990;
float Argula_max_lux = 11610;
int Argula_irrigation_time = 11800;
int currentlevel = 0; 

//temp
float Spinach_targetTemp = 25.0;
float Spinach_min_temp = 23.0; 
float Spinach_max_temp = 27.0;

//moisture
float Spinach_targetMoist = 70.0;
//light
float Spinach_targetlux = 8100; 
float Spinach_min_lux = 7290;
float Spinach_max_lux = 8910;
float Spinach_irrigation_time = 18610;



float calibed_soil(float raw){
    float calibed = map(raw,3241,1491,0,100);
    if (calibed < 0){
      calibed = 0;
    }
    else if (calibed > 100){
      calibed = 100;
    }
    
      return calibed;
    
}

float calibed_lux(float raw){
    return 0.9985 * raw;
}

float calibed_temp(float raw){
    return 1.054 *raw - 0.2272;
}


float measurePtemp(class Adafruit_AM2320 am){
    return calibed_temp(am.readTemperature());
}

float measurelight(class BH1750 lightMeter){
    return calibed_lux(lightMeter.readLightLevel());
}
float measuremoist(){
    return calibed_soil(analogRead(Soil_pin));
}


void statics(){
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 320, 35, TFT_NAVY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Eruca Sync", 160, 8, 2);
    tft.drawFastHLine(0, 85, 300, TFT_SILVER);
    tft.drawFastHLine(0, 140, 300, TFT_SILVER);

    tft.drawFastVLine(160, 45, 145, TFT_SILVER);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("TEMP:",15, 45, 2);
    tft.drawString("MOIST:", 170, 45, 2);
    tft.drawString("LUX:", 15, 95, 2);
    tft.drawString("ACTUATORS:", 170, 95, 2);
}


void menu(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Select Plant", 160, 40, 4);


    tft.fillRect(20, 100, 130, 80, TFT_DARKGREEN);
    tft.fillRect(20,100,130,80, TFT_WHITE);
    tft.drawCentreString("Argula", 85, 130, 2);

    tft.fillRect(170, 100, 130, 80, TFT_MAROON);
    tft.fillRect(170,100,130,80, TFT_WHITE);  
    tft.drawCentreString("Spinach", 235, 130, 2);  

}


void refreash(){
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(String(currentTemp, 1) + " C", 15, 60, 4);
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.drawString(String(currentMoist, 1) + " %", 170, 60, 4);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(String(currentLux, 0) + " lx", 15, 110, 4);

    tft.setTextSize(1);
    tft.setTextColor(tempStatus ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.drawString(tempStatus ? "Cooling: ON" : "Cooling: OFF", 170, 115, 2);
    tft.setTextColor(moistStatus ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.drawString(moistStatus ? "Irrigation: ON" : "Irrigation: OFF", 170, 135, 2);
    tft.setTextColor(lightStatus ? TFT_GREEN : TFT_RED, TFT_BLACK); 
    tft.drawString(lightStatus ? "Shading: ON" : "Shading: OFF", 170, 155, 2);

}



void logic(){
    //temp_logic
    if (mode == 1){
    targetlux = Argula_targetlux;
    targetMoist = Argula_targetMoist;
    targetTemp = Argula_targetTemp;

    max_lux = Argula_max_lux;
    min_lux = Argula_min_lux;

    max_temp = Argula_max_temp;
    min_temp = Argula_min_temp;
    irrigation_time = Argula_irrigation_time;
    }
    else if (mode == 0){
    targetlux = Spinach_targetlux;
    targetMoist = Spinach_targetMoist;
    targetTemp = Spinach_targetTemp;

    max_lux = Spinach_max_lux;
    min_lux = Spinach_min_lux;

    max_temp = Spinach_max_temp;
    min_temp = Spinach_min_temp;

    irrigation_time = Spinach_irrigation_time;
    
}

if(currentTemp > max_temp){
    analogWrite(FAN1_pin, 255);
    analogWrite(FAN2_pin, 255);
    analogWrite(Peliter_pin, 255);
    tempStatus = true;
    
}
else if (currentTemp < min_temp){
    analogWrite(FAN1_pin, 0);
    analogWrite(FAN2_pin, 0);
    analogWrite(Peliter_pin, 0);
    tempStatus = false;
} else {
    Serial.println("There is a problem with the temperature sensor");
}


if (currentMoist < 50 && currentMoist > 10){
    moistStatus = true;
    analogWrite(PUMP_pin, 255);
    delay(irrigation_time);
    analogWrite(PUMP_pin, 0);
    moistStatus = false;
} else if (currentMoist <= 10){
    analogWrite(PUMP_pin, 0);
    moistStatus = false;
} else if (currentMoist >= 85){
    analogWrite(PUMP_pin, 0);
    moistStatus = false;
}

if (currentLux < min_lux && currentLux >= 0){
    if(currentlevel > 0){
        lightStatus = true;
        ServoL.write(45); ServoR.write(135);
        currentlevel--;
        delay(9500);
        ServoL.write(90); ServoR.write(90);
        lightStatus = false;
    
    } else if (currentlevel == 0){
        for (int i = 0; i < 48; i++){
            pixels.setPixelColor(i, pixels.Color(128, 0, 32));
            pixels.show();
            delay(50);
        }
    }

} else if (currentLux > max_lux && currentlevel< 4){
    for (int i = 0; i < 48; i++){
            pixels.setPixelColor(i, pixels.Color(0, 0, 0));
            pixels.show();
            delay(50);
        }
        lightStatus = true;
    ServoL.write(135); ServoR.write(45);
    currentlevel++;
    delay(9500);
    ServoL.write(90); ServoR.write(90);
    lightStatus = false;
} else {
    ServoL.write(90); ServoR.write(90);
    lightStatus = false;
}



}

void setup() {
    Serial.begin(115200);
    tft.init();
    tft.setRotation(3);

    pinMode(Peliter_pin, OUTPUT);
    pinMode(FAN1_pin, OUTPUT);
    pinMode(FAN2_pin, OUTPUT);
    pinMode(PUMP_pin, OUTPUT);
    pinMode(Soil_pin, INPUT);
    pinMode(NEOs_pin, OUTPUT);
    ServoL.attach(ServoL_pin);
    ServoR.attach(ServoR_pin);
    pixels.begin();
    pixels.setBrightness(100);

    Wire.begin(I2C_SDA, I2C_SCL, 100000);
    am.begin();
    lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
    menu();
}

void loop() {
    if (sysmod == MENU){
        TSPoint p = ts.getPoint();

        pinMode(XP, OUTPUT);
        pinMode(YP, OUTPUT);

        if (p.z > ts.pressureThreshhold){

            int16_t x_calibed = map(p.x, 390, 3683, 0, 320);
            int16_t y_calibed = map(p.y, 3408, 471, 0, 240);

            if (x_calibed > 20 && x_calibed < 150 && y_calibed > 100 && y_calibed < 180){
                mode = 1;
                sysmod = ARGULA;
                tft.fillScreen(TFT_BLACK);
                statics();
            } else if (x_calibed > 170 && x_calibed < 300 && y_calibed > 100 && y_calibed < 180){
                mode = 0;
                sysmod = SPINACH;
                tft.fillScreen(TFT_BLACK);
                statics();
            }

        }





    }else {
    float Stemp = 0;
    float Slux = 0;
    float Smoist = 0;

    for (int i = 0; i < 3; i++){
        Stemp += measurePtemp(am);
        Slux += measurelight(lightMeter);
        Smoist += measuremoist();
        delay(1000);
    }
    currentTemp = Stemp/3;
    currentLux = Slux/3;
    currentMoist = Smoist/3;
    refreash();


    logic();
    }
}

