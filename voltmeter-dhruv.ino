#include "LedControl.h" //  need the library

LedControl lc=LedControl(10, 9, 11, 1); // lc is our object
// pin 12 is connected to the MAX7219 pin 1
// pin 11 is connected to the CLK pin 13
// pin 10 is connected to LOAD pin 12
// 1 as we are only using 1 MAX7219

volatile boolean adcBusy;
volatile int adcVal;

int inputPin = A0;
char volt_input_1 [5];

void setup()
{
  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0,false);// turn off power saving, enables display
  lc.setIntensity(0,8);// sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen
  // initialize serial communication with computer:
  Serial.begin(115200);

  //set up the adc
    ADMUX = _BV(REFS0);                                //use AVcc as reference
    ADCSRA  = _BV(ADEN)  | _BV(ADATE) | _BV(ADIE);     //enable ADC, auto trigger, interrupt when conversion complete
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);    //ADC prescaler: divide by 128
    ADCSRB = _BV(ADTS2) | _BV(ADTS0);                  //trigger ADC on Timer/Counter1 Compare Match B
}
void loop()
{

  while (adcBusy);                     //if a conversion is in progress, wait for it to complete
  sprintf ( volt_input_1, "%03i",adcVal);
  
  Serial.println(analogRead(inputPin));

  if(volt_input_1[3] == '\0'){
    for(int i=4; i>0; i--){
      volt_input_1[i] = volt_input_1[i-1];
    }
    volt_input_1[0] = '0';
  }

  Serial.println(volt_input_1);

  for (int a=3; a>=0; a--)
  {
    
    lc.setChar(0, (3-a), volt_input_1[a], false);
  }

  delay(333);
  
}
ISR(ADC_vect)
{
    adcBusy = false;
    adcVal = ADC;
}

ISR(TIMER1_COMPB_vect)
{
    adcBusy = true;
}

