// **** Include libraries here ****
// Standard libraries
#include <stdio.h>
#include <string.h>

//CSE13E Support Library
#include "BOARD.h"
#include "Oled.h"
#include "OledDriver.h"
#include "Leds.h"
#include "Buttons.h"
#include "Adc.h"
#include "Ascii.h"

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>



// **** Set any macros or preprocessor directives here ****
// Set a macro for resetting the timer, makes the code a little clearer.
#define TIMER_2HZ_RESET() (TMR1 = 0)

// **** Set any local typedefs here ****

typedef enum {
    SETUP, SELECTOR_CHANGE_PENDING, COOKING, RESET_PENDING, EXTRA_CREDIT
} OvenState;

typedef enum {
    BAKE, TOAST, BROIL
} CookMode; // establishes the cook modes

typedef enum {
    TIMER, TEMP
} SelectorSettings; // establishes the selector settings

typedef struct {
    OvenState state;
    //add more members to this struct
    uint16_t initialTime; // used to store the initial time 
    uint16_t remainingTime; // updates to the time left after cooking starts
    uint16_t temp; // temperature level
    uint16_t buttonDownTime; // measures how long a button was pressed
    CookMode cookMode; // holds the current CookMode
    SelectorSettings selector; // holds whether we are in TEMP changing or TIME changing mode based on SELECTOR_CHANGE_PENDING
} OvenData;

// **** Declare any datatypes here ****



// **** Define any module-level, global, or external variables here ****

#define ovenWidth 6 // the width of the char that holds the special oven characters
#define invertNumber 6 // the number of times the screen will invert for the extra credit
#define LONG_PRESS 1*5 // 1 hz * 5 to get one second
#define DEFAULT_TEMP 350 // the default temp for bake/toast
#define BROIL_DEFAULT_TEMP 500 // the default temp for broil
static OvenData ovendata; // module level variable for the OvenData struct
static uint8_t adcChanged = FALSE; // adcChange flag that starts as false until the flag is provoked
static uint8_t buttonFlag = BUTTON_EVENT_NONE; // button event flag that starts as false until the flag is provoked
static uint16_t TIMER_TICK = FALSE; // TIMER_TICK flag that starts as false until the flag is provoked
static uint16_t adcVal; // create a static varibale to hold the current value of AdcRead() from ADC.h
static uint8_t top8adcVal; // holds the top 8 bits of the adcVal that we want to display as directed in the lab manual
static uint16_t buttonTimer = 0; // holds the time gathered from ovendata.buttonDownTime
static uint16_t savedButtonTimer = 0; // saves the buttonTimmer info to use later
static uint16_t LedTimer; // holds the whole number of cookTime/8 to turn off the leds
static uint16_t LedTimerRemaining; // holds the remainder to make sure that if total time is 25, the the leds turn off at 25/8 sec. or 3.125 seconds
static uint16_t timerTick = 0; // holds the time since entering out TIMER_TICK flag if statements to determine the time since entering that loop
static char currentLEDSet; // holds the current LED info
static char nextLEDSet; // bit shift for next LED set info
static uint8_t invertFlag = FALSE; // flag for extra credit to invert 
static int breakCounter = 0; // used as a counter to hit the break out of the extra credit loop

// **** Put any helper functions here ****

/*This function will update your OLED to reflect the state .*/
void updateOvenOLED(OvenData ovendata) {
    OledClear(OLED_COLOR_BLACK);

    // make the string holders for each case (top on, top off, bottom on, bottom off)
    char top_on[ovenWidth]; // oven width of 5 + 1 for the null character
    char top_off[ovenWidth]; // oven width of 5 + 1 for the null character
    char bottom_on[ovenWidth]; // oven width of 5 + 1 for the null character
    char bottom_off[ovenWidth]; // oven width of 5 + 1 for the null character
    char oven_printed[100]; // arbitrary arrray length to hold the whole print to OLED statement 

    // make the sprintf() for each case (top on, top off, bottom on, bottom off)
    sprintf(top_on, "%s%s%s%s%s", OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON);
    sprintf(top_off, "%s%s%s%s%s", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF);
    sprintf(bottom_on, "%s%s%s%s%s", OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON);
    sprintf(bottom_off, "%s%s%s%s%s", OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF);

    // oven data state
    switch (ovendata.cookMode) {

        case BAKE:
            // makes sure that the oven state is not currently cooking or reseting
            if (ovendata.state != COOKING && ovendata.state != RESET_PENDING) {
                // makes sure that we are currently editing the timer
                if (ovendata.selector == TIMER) {
                    uint16_t oven_minutes = ovendata.initialTime / 60; // divides the total time by 60 to get minutes
                    uint16_t oven_seconds = ovendata.initialTime % 60; // uses the remainder from minutes to get seconds
                    sprintf(oven_printed, "|%s| MODE: BAKE\n|     | >TIME: %d:%02d\n|-----|  TEMP: %d%s\n|%s|", top_off, oven_minutes, oven_seconds,
                            ovendata.temp, DEGREE_SYMBOL, bottom_off);
                }// else we're in the mode to edit the temp
                else {
                    uint16_t oven_minutes = ovendata.initialTime / 60; // divides the total time by 60 to get minutes
                    uint16_t oven_seconds = ovendata.initialTime % 60; // uses the remainder from minutes to get seconds
                    sprintf(oven_printed, "|%s| MODE: BAKE\n|     |  TIME: %d:%02d\n|-----| >TEMP: %d%s\n|%s|", top_off, oven_minutes, oven_seconds,
                            ovendata.temp, DEGREE_SYMBOL, bottom_off);
                }
            }// enters the print state for if we've started to cook 
            else {
                uint16_t oven_minutes = ovendata.remainingTime / 60; // divides the total time by 60 to get minutes
                uint16_t oven_seconds = ovendata.remainingTime % 60; // uses the remainder from minutes to get seconds
                sprintf(oven_printed, "|%s| MODE: BAKE\n|     |  TIME: %d:%02d\n|-----|  TEMP: %d%s\n|%s|", top_on, oven_minutes, oven_seconds,
                        ovendata.temp, DEGREE_SYMBOL, bottom_on);
            }
            break;

        case BROIL:
            // makes sure that the oven state is not currently cooking or reseting
            if (ovendata.state != COOKING && ovendata.state != RESET_PENDING) {
                uint16_t oven_minutes = ovendata.initialTime / 60; // divides the total time by 60 to get minutes
                uint16_t oven_seconds = ovendata.initialTime % 60; // uses the remainder from minutes to get seconds
                sprintf(oven_printed, "|%s| MODE: BROIL\n|     |  TIME: %d:%02d\n|-----|  TEMP: %d%s\n|%s|", top_off, oven_minutes, oven_seconds,
                        BROIL_DEFAULT_TEMP, DEGREE_SYMBOL, bottom_off);
            }// enters the print state for if we've started to cook 
            else {
                uint16_t oven_minutes = ovendata.remainingTime / 60; // divides the total time by 60 to get minutes
                uint16_t oven_seconds = ovendata.remainingTime % 60; // uses the remainder from minutes to get seconds
                sprintf(oven_printed, "|%s| MODE: BROIL\n|     |  TIME: %d:%02d\n|-----|  TEMP: %d%s\n|%s|", top_on, oven_minutes, oven_seconds,
                        BROIL_DEFAULT_TEMP, DEGREE_SYMBOL, bottom_off);
            }
            break;

        case TOAST:
            // makes sure that the oven state is not currently cooking or reseting
            if (ovendata.state != COOKING && ovendata.state != RESET_PENDING) {
                uint16_t oven_minutes = ovendata.initialTime / 60; // divides the total time by 60 to get minutes
                uint16_t oven_seconds = ovendata.initialTime % 60; // uses the remainder from minutes to get seconds
                sprintf(oven_printed, "|%s| MODE: TOAST\n|     |  TIME: %d:%02d\n|-----|\n|%s|", top_off, oven_minutes, oven_seconds, bottom_off);
            }// enters the print state for if we've started to cook 
            else {
                uint16_t oven_minutes = ovendata.remainingTime / 60; // divides the total time by 60 to get minutes
                uint16_t oven_seconds = ovendata.remainingTime % 60; // uses the remainder from minutes to get seconds
                sprintf(oven_printed, "|%s| MODE: TOAST\n|     |  TIME: %d:%02d\n|-----|\n|%s|", top_off, oven_minutes, oven_seconds, bottom_on);
            }
            break;
    }

    OledDrawString(oven_printed); // draws the previously created string
    OledUpdate(); // updates the OLED with the OledDrawString command
}

/*This function will execute your state machine.  
 * It should ONLY run if an event flag has been set.*/
void runOvenSM(void) {

    //write your SM logic here.
    switch (ovendata.state) {

        case SETUP:
            if (adcChanged == TRUE) {
                adcVal = AdcRead();
                top8adcVal = (adcVal >> 2) & 0xFF;

                // if in bake and editing temp
                if (ovendata.cookMode == BAKE && ovendata.selector == TEMP) {
                    ovendata.temp = top8adcVal + 300;
                }// if not in bake or editing temp
                else {
                    ovendata.initialTime = top8adcVal + 1;
                    ovendata.remainingTime = ovendata.initialTime;
                }
                // update OLED with info from previous if's
                updateOvenOLED(ovendata);
            }

            // bitwise AND to check if buttonFlag and BUTTON_EVENT_3DOWN are equal
            if (buttonFlag & BUTTON_EVENT_3DOWN) {
                // store button down time and send to SELECTOR_CHANGE_PENDING state
                ovendata.buttonDownTime = buttonTimer;
                ovendata.state = SELECTOR_CHANGE_PENDING;
            }

            // BUTTON_EVENT_4DOWN sets ovendata.state == COOKING 
            // bitwise AND to check if buttonFlag and BUTTON_EVENT_4DOWN are equal
            if (buttonFlag & BUTTON_EVENT_4DOWN) {
                savedButtonTimer = buttonTimer;
                LEDS_SET(0xFF); // update LEDS
                LedTimer = (ovendata.initialTime * 5) / 8; // interval for the LEDS to turn off
                LedTimerRemaining = (ovendata.initialTime * 5) % 8; // interval for the LEDS to turn off remainder
                ovendata.state = COOKING; // set state to COOKING to move to correct case
                updateOvenOLED(ovendata); // update the OLED as cooking
            }

            break;

        case SELECTOR_CHANGE_PENDING:
            // bitwise AND to check if buttonFlag and BUTTON_EVENT_3UP are equal
            if (buttonFlag & BUTTON_EVENT_3UP) {
                if ((buttonTimer - ovendata.buttonDownTime) >= LONG_PRESS) {
                    // only have to do a selector change if cookMode = BAKE since we can't change the 
                    // temp for toast and broil
                    if (ovendata.cookMode == BAKE) {
                        if (ovendata.selector == TEMP) {
                            ovendata.selector = TIMER;
                            // for testing
                            // printf("ovendata.selector = TIMER\n"); 
                        } else {
                            ovendata.selector = TEMP;
                            // for testing
                            // printf("ovendata.selector = TEMP\n"); 
                        }
                        updateOvenOLED(ovendata);
                        ovendata.state = SETUP;
                    } else {
                        ovendata.selector = TIMER;
                        updateOvenOLED(ovendata);
                        ovendata.state = SETUP;
                    }

                }// else if (buttonTimer - ovendata.buttonDownTime) < LONG_PRESS) then we change the cook mode 
                else {
                    if (ovendata.cookMode == BAKE) {
                        ovendata.cookMode = TOAST;
                        // for testing
                        // printf("ovendata.cookMode = TOAST\n");
                    } else if (ovendata.cookMode == TOAST) {
                        ovendata.cookMode = BROIL;
                        // for testing
                        // printf("ovendata.cookMode = BROIL\n");
                    } else {
                        ovendata.cookMode = BAKE;
                        ovendata.temp = DEFAULT_TEMP;
                        // for testing
                        // printf("ovendata.cookMode = BAKE\n");
                    }
                    updateOvenOLED(ovendata);
                    ovendata.state = SETUP;
                }
            }

            break;

        case COOKING:
            if (TIMER_TICK) {
                timerTick++;
                // Check if the LED update conditions are met 
                // (if greater than 0, then LedTimer +1)) 
                // (if == 0, then just LedTimer)
                if ((LedTimerRemaining == 0 && timerTick == LedTimer) || (LedTimerRemaining > 0 && timerTick == LedTimer + 1)) {
                    currentLEDSet = LEDS_GET();
                    nextLEDSet = currentLEDSet << 1;
                    LEDS_SET(nextLEDSet);
                    timerTick = 0;
                    LedTimerRemaining--;
                }
                // checks if the time since the button - 
                if ((buttonTimer - savedButtonTimer) % 5 == 0) {
                    ovendata.remainingTime--;
                    updateOvenOLED(ovendata);
                }

                // Check if the remainingTime == 0
                if (ovendata.remainingTime == 0) {
                    LEDS_SET(0x00); // turn off all LEDS
                    ovendata.remainingTime = ovendata.initialTime; // reset the time
                    ovendata.state = EXTRA_CREDIT; // send back to setup 
                    updateOvenOLED(ovendata); // update OLED
                }
            }
            if (buttonFlag & BUTTON_EVENT_4DOWN) {
                ovendata.buttonDownTime = buttonTimer; // store freerunning time
                ovendata.state = RESET_PENDING; // send to RESET_PENDING upon proper button press

            }

            break;

        case RESET_PENDING:

            if (TIMER_TICK) {
                timerTick++;

                /* have to include the led decrement and timer decrement from cook so that 
                   when pushing down on the button to reset, the timer and leds still move 
                   during that time rather than just freezing while the BUTTON_EVENT_4DOWN
                   event happens */

                // same logic from COOKING to implement the timer -- and leds shift while holding 
                // the button down to reset
                if ((LedTimerRemaining == 0 && timerTick == LedTimer) || (LedTimerRemaining > 0 && timerTick == LedTimer + 1)) {
                    currentLEDSet = LEDS_GET();
                    nextLEDSet = currentLEDSet << 1;
                    LEDS_SET(nextLEDSet);
                    timerTick = 0;
                    LedTimerRemaining--;
                }

                // checks if the time since the button - 
                if ((buttonTimer - savedButtonTimer) % 5 == 0) {
                    ovendata.remainingTime--;
                    updateOvenOLED(ovendata);
                }

                // reset logic
                if ((buttonTimer - ovendata.buttonDownTime) >= LONG_PRESS) {
                    LEDS_SET(0x00); // update LEDs
                    ovendata.remainingTime = ovendata.initialTime; // reset settings
                    ovendata.state = SETUP; // reset settings
                    updateOvenOLED(ovendata);
                }
            }

            if (buttonFlag & BUTTON_EVENT_4UP) {
                ovendata.state = COOKING;
            }

            break;

        case EXTRA_CREDIT:
            if (TIMER_TICK) {
                while (1) {
                    // Check if it's time to toggle inversion
                    // printf("%d\n", counter);

                    if (!invertFlag) {
                        invertFlag = TRUE;
                        OledSetDisplayInverted();
                    } else {
                        invertFlag = FALSE;
                        OledSetDisplayNormal();
                    }
                    OledUpdate();

                    // add a delay before re-entering the loop
                    for (int delay = 0; delay < 1000000; delay++) {
                    }

                    // Increment the break counter
                    breakCounter++;

                    // Check if we've reached the desired number of iterations
                    if (breakCounter == invertNumber) {
                        // printf("%d\n", breakCounter);
                        breakCounter = 0;
                        break;
                    }
                }
            }
            ovendata.state = SETUP;
            break;
    }
}

int main() {
    BOARD_Init();

    //initalize timers and timer ISRs:

    T2CON = 0; // T2 off
    T2CONbits.TCKPS = 0b100; // 1:16 prescaler
    PR2 = BOARD_GetPBClock() / 16 / 100; // interrupt at .5s intervals
    T2CONbits.ON = 1; // turn the flag on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T2IF = 0; // turn the flag off
    IPC2bits.T2IP = 4; // priority of  4
    IPC2bits.T2IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T2IE = 1; // turn the flag on

    T3CON = 0; // T3 off
    T3CONbits.TCKPS = 0b111; // 1:256 prescaler
    PR3 = BOARD_GetPBClock() / 256 / 5; // interrupt at .5s intervals
    T3CONbits.ON = 1; // turn the flag on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T3IF = 0; // turn the flag off
    IPC3bits.T3IP = 4; // priority of  4
    IPC3bits.T3IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T3IE = 1; // turn the flag on

    printf("Welcome to kjconway's Lab07 (Toaster Oven).  Compiled on %s %s.", __TIME__, __DATE__);

    // initialize state machine (and anything else you need to init) here
    // our starting settings

    ovendata.buttonDownTime = 0;
    ovendata.initialTime = 1;
    ovendata.remainingTime = 1;
    ovendata.temp = DEFAULT_TEMP;
    ovendata.state = SETUP;
    ovendata.cookMode = BAKE;

    // initilize the boards/buttons/LEDS/OLED
    OledInit();
    ButtonsInit();
    AdcInit();
    LEDS_INIT();
    updateOvenOLED(ovendata);

    while (1) {
        // Add main loop code here:
        // check for events
        if (buttonFlag != BUTTON_EVENT_NONE || buttonFlag != adcChanged || buttonFlag != TIMER_TICK) {
            // on event, run runOvenSM()
            runOvenSM();
            // clear event flags
            buttonFlag = BUTTON_EVENT_NONE;
            adcChanged = FALSE;
            TIMER_TICK = FALSE;
        }
    }
}

/*The 5hz timer is used to update the free-running timer and to generate TIMER_TICK events*/
void __ISR(_TIMER_3_VECTOR, ipl4auto) TimerInterrupt5Hz(void) {
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;

    //add event-checking code here
    TIMER_TICK = TRUE; // TIMER_TICK interupt flag
    buttonTimer++;
}

/*The 100hz timer is used to check for button and ADC events*/
void __ISR(_TIMER_2_VECTOR, ipl4auto) TimerInterrupt100Hz(void) {
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    //add event-checking code here
    adcChanged = AdcChanged(); // updates the static variable to true or false based on if the adc value has changed
    buttonFlag = ButtonsCheckEvents(); // updates the static variable to whichever button event has occured
}