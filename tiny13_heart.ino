/*
  tiny13
 
  low power flashy heart based on an attiny13a micro
  this bit of code sleeps the micro and flashes an LED at some interval.
  time between flashes is shorter when an attached switch is closed.
 */

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

// which pin goes to which thing
#define LED_PIN 2
#define SWITCH_PIN 1

// switch debounce values
#define SWITCH_STABLE_COUNT 5
#define SWITCH_MAX_COUNT 20

// LED is PWM'd with a delay loop
// the first number is how many ms to delay() after turning on the LED
// the second number is how many ms to delay() after turning off the LED
// the third number is how many times to repear the ON-OFF sequence
const byte fadeTableOn[] =   {  0,  1,  2,  12,  2,  1,  0, } ;
const byte fadeTableOff[] =  {  1,  1,  0,  0,   0,  1,  1, } ;
const byte fadeTableLoop[] = {  5,  4,  5,  1,   5,  4,  5, } ;

// size of the fade table
#define FADE_TABLE_ENTRIES 7

// minimum sleep time and maximum sleep time in watchdog timeouts
#define SLEEP_LOOP_MIN 1
#define SLEEP_LOOP_MAX 6

// how many watchdog timeouts to wait before slowing down the flash rate
#define NO_SWITCH_RELAX_COUNT 4

// current number of watchdog timeouts to wait between flashes
byte sleepLoopCount = SLEEP_LOOP_MAX ;

// consecutive watchdog timeouts that have passed without a switch down
byte noSwitchCount = 0 ;

// whether the switch is open or closed
volatile byte switchState = 0 ;

// debounce and set the switchState global
void PollSwitch() {
  byte maxCount = SWITCH_MAX_COUNT ;
  byte stableCount = SWITCH_STABLE_COUNT ;
  byte onoff ;
  
  while (stableCount && --maxCount) {
    onoff = digitalRead(SWITCH_PIN) ;
    
    if (onoff == switchState)
       --stableCount ;
    else {
      switchState = onoff ;
      stableCount = SWITCH_STABLE_COUNT ;
    }
  }  
}

ISR(WDT_vect) {
}

ISR(PCINT0_vect) {
  PollSwitch() ;
}


// returns TRUE if a switch was down at some point during sleeptime
byte sleep() {
  byte retVal = switchState ;
  byte sleepLoop = sleepLoopCount ;

  while (--sleepLoop) {
    sei() ;
    sleep_mode() ;
    
    // woken up by a switch event?
    // drag--can't really tell how long execution has slept
    // so just carry on...  
    retVal |= switchState ;
  }
  
  return retVal ;
}

// make the led fade in and out for a heartbeat
void beat() {
  byte index, count ;

  for (index = 0 ;
       index < FADE_TABLE_ENTRIES ;
       index++) {
    for (count = 0 ;
         count < fadeTableLoop[index] ;
         count++) {
      digitalWrite(LED_PIN, 1) ;
      delay(fadeTableOn[index]) ;
      digitalWrite(LED_PIN, 0) ;
      delay(fadeTableOff[index]) ;
    }
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT) ;
  pinMode(SWITCH_PIN, INPUT) ;
  
  // turn off the adc
  ACSR = (1<<ACD) ;
  ADCSRA = (1<<ADEN) ;
  
  // turn on pin change interrupts for the switch pin
  GIMSK = _BV(PCIE) ;
  PCMSK = (1<<SWITCH_PIN) ;
  
  // enable the watchdog timer for 1sec
  WDTCR |= (1<<WDP2) | (1<<WDP1) ;
  WDTCR |= (1<<WDTIE) ;
  
  sei() ;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN) ;
}

void loop() {
  if (sleep() &&
      sleepLoopCount > SLEEP_LOOP_MIN) {
     sleepLoopCount-- ;
     noSwitchCount = 0 ;
  } else if ((sleepLoopCount < SLEEP_LOOP_MAX) &&
            (++noSwitchCount > NO_SWITCH_RELAX_COUNT)) {
     sleepLoopCount++ ;
     noSwitchCount = 0 ;
  }
  beat() ;
}
