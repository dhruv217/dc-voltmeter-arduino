#include "LedControl.h" //  need the library
#include <AnalogScanner.h> // needs this liberary git clone location :-- https://github.com/merose/AnalogScanner.git

LedControl lc=LedControl(10, 11, 9, 1); // lc is our object // 9, 11
// pin 12 is connected to the MAX7219 pin 1
// pin 11 is connected to the CLK pin 13
// pin 10 is connected to LOAD pin 12
// 1 as we are only using 1 MAX7219
AnalogScanner scanner;
int scanOrder[] = {A0, A1};
volatile float ADC0VAL, ADC1VAL;
float voltage1, voltage2;
float averageA0, averageA1, ETA=0.8;

void setup()
{
  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0,false);// turn off power saving, enables display
  lc.setIntensity(0,8);// sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen
  // initialize serial communication with computer:
  Serial.begin(115200);

  scanner.setScanOrder(2, scanOrder);
  scanner.beginScanning();
  scanner.setCallback(A0, ValueChangeCallBack);
  scanner.setCallback(A1, ValueChangeCallBack);
  delay(1); // Wait for the first scans to occur.
}
void loop()
{
  Serial.println(ADC0VAL);
  voltage1 = (ADC0VAL * 5.0)/1024;
  Serial.println(voltage1);
  printFloatToLED(0, voltage1);
  
  Serial.println(ADC1VAL);
  voltage2 = (ADC1VAL * 5.0)/1024;
  Serial.println(voltage2);
  printFloatToLED(1, voltage2);
  Serial.println("_________-----------------___________");
  delay(333);
  
}

void ValueChangeCallBack(int index, int pin, int value){
  if (pin == 14){
    ADC0VAL = getResponsiveValueA0(value);
    // ADC0VAL = value;
  }
  if (pin == 15)
  {
    ADC1VAL = getResponsiveValueA1(value);
    // ADC1VAL = value;
  }
}

void printFloatToLED(int position, float value){
  if(round(value) == 5)
    value += 0.01;
  int valueArray[3];
  valueArray[0] = int(value)%10;
  valueArray[1] = int(value*10)% 10;
  valueArray[2] = int(value*100)% 10; 
  if(position == 0){
    lc.setDigit(0, 0, valueArray[2], false);
    lc.setDigit(0, 1, valueArray[1], false);
    lc.setDigit(0, 2, valueArray[0], true);
  }
  if(position == 1){
    lc.setDigit(0, 4, valueArray[2], false);
    lc.setDigit(0, 5, valueArray[1], false);
    lc.setDigit(0, 6, valueArray[0], true);
  }
}

float getResponsiveValueA0 (int newvalue){
  averageA0 = ETA * (float)newvalue + (1-ETA) * averageA0;
  return averageA0;
}

float getResponsiveValueA1 (int newvalue){
  averageA1 = ETA * (float)newvalue + (1-ETA) * averageA0;
  return averageA1;
}

