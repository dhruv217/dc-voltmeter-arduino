#include "LedControl.h" //  need the library
#include <AnalogScanner.h> // needs this liberary git clone location :-- https://github.com/merose/AnalogScanner.git

LedControl lc=LedControl(10, 9, 11, 1); // lc is our object
// pin 12 is connected to the MAX7219 pin 1
// pin 11 is connected to the CLK pin 13
// pin 10 is connected to LOAD pin 12
// 1 as we are only using 1 MAX7219
AnalogScanner scanner;


char volt_input_1 [5];
char volt_input_2 [5];
int scanOrder[] = {A0}; //, A1};
bool sleepEnable = true;
bool edgeSnapEnable = true;
float activityThreshold = 4.0;
int analogResolution = 1024;
float smoothValue;
float errorEMA = 0.0;
bool sleeping = false;
float snapMultiplier;


volatile int ADC0VAL;
volatile int ADC1VAL;
void setup()
{
  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0,false);// turn off power saving, enables display
  lc.setIntensity(0,8);// sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen
  // initialize serial communication with computer:
  Serial.begin(115200);

  scanner.setScanOrder(1, scanOrder);
  scanner.beginScanning();
  scanner.setCallback(A0, ValueChangeCallBack);
  delay(1); // Wait for the first scans to occur.
}
void loop()
{
  Serial.println(ADC0VAL);
  IntToCharArray(ADC0VAL, volt_input_1);
  Serial.println(volt_input_1);
  printToLED(0, volt_input_1);
  
  delay(100);
  
}

void ValueChangeCallBack(int index, int pin, int value){
  if (pin == 14){
    // ADC0VAL = getResponsiveValue(value);
    ADC0VAL = value;
  }
  if (pin == 15)
  {
    // ADC1VAL = getResponsiveValue(value);
    ADC1VAL = value;
  }
}

void IntToCharArray(int val, char charArray[])
{
  sprintf(charArray, "%03i", val);

  if (charArray[3] == '\0')
  {
    for (int i = 4; i > 0; i--)
    {
      charArray[i] = charArray[i - 1];
    }
    charArray[0] = '0';
  }
}

void printToLED(int position, char charArray[]) {
  for (int a = 3; a >= 0; a--)
  {
    lc.setChar(position, (3 - a), charArray[a], false);
  }
}

int getResponsiveValue(int newValue)
{
  // if sleep and edge snap are enabled and the new value is very close to an edge, drag it a little closer to the edges
  // This'll make it easier to pull the output values right to the extremes without sleeping,
  // and it'll make movements right near the edge appear larger, making it easier to wake up
  if (sleepEnable && edgeSnapEnable)
  {
    if (newValue < activityThreshold)
    {
      newValue = (newValue * 2) - activityThreshold;
    }
    else if (newValue > analogResolution - activityThreshold)
    {
      newValue = (newValue * 2) - analogResolution + activityThreshold;
    }
  }

  // get difference between new input value and current smooth value
  unsigned int diff = abs(newValue - smoothValue);

  // measure the difference between the new value and current value
  // and use another exponential moving average to work out what
  // the current margin of error is
  errorEMA += ((newValue - smoothValue) - errorEMA) * 0.4;

  // if sleep has been enabled, sleep when the amount of error is below the activity threshold
  if (sleepEnable)
  {
    // recalculate sleeping status
    sleeping = abs(errorEMA) < activityThreshold;
  }

  // if we're allowed to sleep, and we're sleeping
  // then don't update responsiveValue this loop
  // just output the existing responsiveValue
  if (sleepEnable && sleeping)
  {
    return (int)smoothValue;
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
  if (sleepEnable)
  {
    snap *= 0.5 + 0.5;
  }

  // calculate the exponential moving average based on the snap
  smoothValue += (newValue - smoothValue) * snap;

  // ensure output is in bounds
  if (smoothValue < 0.0)
  {
    smoothValue = 0.0;
  }
  else if (smoothValue > analogResolution - 1)
  {
    smoothValue = analogResolution - 1;
  }

  // expected output is an integer
  return (int)smoothValue;
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

void setSnapMultiplier(float newMultiplier)
{
  if (newMultiplier > 1.0)
  {
    newMultiplier = 1.0;
  }
  if (newMultiplier < 0.0)
  {
    newMultiplier = 0.0;
  }
  snapMultiplier = newMultiplier;
}
