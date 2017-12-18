#include "LedControl.h" //  need the library
#include <AnalogScanner.h> // needs this liberary git clone location :-- https://github.com/merose/AnalogScanner.git

LedControl lc=LedControl(10, 11, 9, 1); // lc is our object // 9, 11
// pin 12 is connected to the MAX7219 pin 1
// pin 11 is connected to the CLK pin 13
// pin 10 is connected to LOAD pin 12
// 1 as we are only using 1 MAX7219
AnalogScanner scanner;
int scanOrder[] = { A0, A1};
int rawValueA0;
int updatedValue;
volatile int ADC0VAL, ADC1VAL;
float voltage1, voltage2;
float averageA0, averageA1, ETA=0.8;

float activityThreshold = 4.0;
int analogResolution = 1024;
float smoothValue[2];
float errorEMA [2] = { 0.0, 0.0};
bool sleeping[]= { false, false};
float snapMultiplier = 0.01;

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
  voltage1 = (ADC0VAL * 200.0)/1024;
  Serial.println(voltage1);
  printFloatToLED(0, voltage1);
  
  Serial.println(ADC1VAL);
  voltage2 = (ADC1VAL * 150.0)/1024;
  Serial.println(voltage2);
  printFloatToLED(1, voltage2);

  Serial.println("_________-----------------___________");
  Serial.println();
  delay(100);
  
}

void ValueChangeCallBack(int index, int pin, int value){
  switch (pin) {
    case 14:
      ADC0VAL = getResponsiveValue( pin, value);
      break;
    case 15:
      ADC1VAL = getResponsiveValue( pin, value);
      break;
  }
}

void printFloatToLED(int position, float value){
  if(round(value) == 5)
    value += 0.01;
  int valueArray[4];

  /* valueArray[0] = int(value)%10;
  valueArray[1] = int(value*10)% 10;
  valueArray[2] = int(value*100)% 10;  */
  valueArray[0] = int(value / 100) % 10;
  valueArray[1] = int(value / 10) % 10;
  valueArray[2] = int(value) % 10;
  valueArray[3] = int(value * 10) % 10;
  if(position == 0){
    lc.setDigit(0, 0, valueArray[3], false);
    lc.setDigit(0, 1, valueArray[2], true);
    lc.setDigit(0, 2, valueArray[1], false);
    lc.setDigit(0, 3, valueArray[0], false);
  }
  if(position == 1){
    lc.setDigit(0, 4, valueArray[3], false);
    lc.setDigit(0, 5, valueArray[2], true);
    lc.setDigit(0, 6, valueArray[1], false);
    lc.setDigit(0, 7, valueArray[0], false);
    }
}


int getResponsiveValue(int pin, int newValue)
{
  pin = pin - 14;
  // if sleep and edge snap are enabled and the new value is very close to an edge, drag it a little closer to the edges
  // This'll make it easier to pull the output values right to the extremes without sleeping,
  // and it'll make movements right near the edge appear larger, making it easier to wake up
  if (newValue < activityThreshold)
  {
    newValue = (newValue * 2) - activityThreshold;
  }
  else if (newValue > analogResolution - activityThreshold)
  {
    newValue = (newValue * 2) - analogResolution + activityThreshold;
  }

  // get difference between new input value and current smooth value
  unsigned int diff = abs(newValue - smoothValue[pin]);

  // measure the difference between the new value and current value
  // and use another exponential moving average to work out what
  // the current margin of error is
  errorEMA[pin] += ((newValue - smoothValue[pin]) - errorEMA[pin]) * 0.4;
  // recalculate sleeping status
  sleeping[pin] = abs(errorEMA[pin]) < activityThreshold;

  // if we're allowed to sleep, and we're sleeping
  // then don't update responsiveValue this loop
  // just output the existing responsiveValue
  if (sleeping[pin])
  {
    return (int)smoothValue[pin];
  }

  // use a 'snap curve' function, where we pass in the diff (x) and get back a number from 0-1.
  // We want small values of x to result in an output close to zero, so when the smooth value is close to the input value
  // it'll smooth out noise aggressively by responding slowly to sudden changes.
  // We want a small increase in x to result in a much higher output value, so medium and large movements are snappy and responsive,
  // and aren't made sluggish by unnecessarily filtering out noise. A hyperbola (f(x) = 1/x) curve is used.
  // First x has an offset of 1 applied, so x = 0 now results in a value of 1 from the hyperbola function.
  // High values of x tend toward 0, but we want an output that begins at 0 and tends toward 1, so 1-y flips this up the right way.
  // Finally the result is multiplied by 2 and capped at a maximum of one, which means that at a certain point all larger movements are maximally snappy

  // then multiply the input by SNAP_MULTIPLER so input values fit the snap curve better.
  float snap = snapCurve(diff * snapMultiplier);

  // when sleep is enabled, the emphasis is stopping on a responsiveValue quickly, and it's less about easing into position.
  // If sleep is enabled, add a small amount to snap so it'll tend to snap into a more accurate position before sleeping starts.
  snap *= 0.5 + 0.5;

  // calculate the exponential moving average based on the snap
  smoothValue[pin] += (newValue - smoothValue[pin]) * snap;

  // ensure output is in bounds
  if (smoothValue[pin] < 0.0)
  {
    smoothValue[pin] = 0.0;
  }
  else if (smoothValue[pin] > analogResolution - 1)
  {
    smoothValue[pin] = analogResolution - 1;
  }
  // expected output is an integer
  return (int)smoothValue[pin];
}

float snapCurve(float x)
{
  float y = 1.0 / (x + 1.0);
  y = (1.0 - y) * 2.0;
  if (y > 1.0)
  {
    return 1.0;
  }
  return y;
}

