#include <Arduino.h>

#include "main.h"
#include "temperature_meter.h"
#include "display.h"

// EDIT THE FOLLOWING TO DECLARE BUTTON PINS FOR OTHER ARDUINO BOARDS
// const auto start_button = WIO_KEY_C;
// const auto stop_button = WIO_KEY_B;

unsigned long int timer_counter;
unsigned int brew_timer_counter;

//Switch Variables - "Reed_Switch"
int state_Reed_Switch = 0; //The actual ~state~ of the state machine
int pin_Reed_Switch = D0;                //Input/Output (IO) pin for the switch, 10 = pin 10 a.k.a. D10
int value_Reed_Switch = 0;                      //Value of the switch ("HIGH" or "LOW")
unsigned long t_Reed_Switch = 0;                //Typically represents the current time of the switch
unsigned long t_0_Reed_Switch = 0;              //The time that we last passed through an interesting state
unsigned long bounce_delay_Reed_Switch = 5;     //The delay to filter bouncing
unsigned int bin_counter = 0;                   //binary counter for reed switch
int virtual_Reed_Switch = VIRT_REED_SWITCH_OFF; // virtual switch

//ADC variables
int pin_ADC_input = A1;

//SM Counter variables
int state_counter1 = 0;      //The actual ~state~ of the state machine
int prev_state_counter1 = 0; //Remembers the previous state (useful to know when the state has changed)
int iSecCounter1 = -1;
int prev_iSecCounter1 = 0;
int iMinCounter1 = -1;
int prev_iMinCounter1 = 0;
unsigned long start_counter1 = 0;
unsigned long elapsed_counter1 = 0;
unsigned long autozero_start = 0;
unsigned int  autozero_delay = 7000; // In millisec

char TimeCounterStr[] = "0:00"; /** String to store time counter value, format: M:SS */

void GPIO_init(void)
{
    pinMode(pin_Reed_Switch, INPUT_PULLUP);
    pinMode(pin_ADC_input, INPUT);
}

void setup()
{
    Serial.begin(115200);
#ifdef SERIAL_DEBUG_ENABLED
    Serial.println(F("Debugging is ON"));
#endif
    GPIO_init();
    StateMachine_counter1();
    StateMachine_Reed_Switch();
    state_machine_volt_meter();
    Display_Init();
    state_Display = DISPLAY_STATE_RESET;
    StateMachine_Display();
    display_Timer_On_All(true,true);
    Display_Temperature(TEMPERATURE_STR_V2, MILLI_VOLT_STR);
    show_millivolt(MILLI_VOLT_STR);
}

void loop()
{
    StateMachine_Reed_Switch();

    //Provide events that can force the state machines to change state
    switch (virtual_Reed_Switch)
    {
    case VIRT_REED_SWITCH_OFF:
        if (state_counter1 == COUNTER_STATE_COUNTING)
        {
            state_counter1 = COUNTER_STATE_STOP;
        }
        break;
    case VIRT_REED_SWITCH_ON:
        if (state_counter1 == COUNTER_STATE_DISABLED)
        {
            state_counter1 = COUNTER_STATE_START;
        }
        break;
    }
    StateMachine_counter1();
    state_machine_volt_meter();
    StateMachine_Display();
}

void update_TimeCounterStr(int tMinutes, int tSeconds)
{
    TimeCounterStr[0] = (char)((tMinutes % 10) + 0x30);
    TimeCounterStr[2] = (char)((tSeconds / 10) + 0x30);
    TimeCounterStr[3] = (char)((tSeconds % 10) + 0x30);
}

/**
* @brief Counter 1 state machine - counts the seconds
*/
void StateMachine_counter1(void)
{

    prev_state_counter1 = state_counter1;

    //State Machine Section
    switch (state_counter1)
    {
    case COUNTER_STATE_RESET:
        iSecCounter1 = 0;
        iMinCounter1 = 0;
        elapsed_counter1 = 0;
        autozero_start = 0;
        state_counter1 = COUNTER_STATE_DISABLED;
        break;
    case COUNTER_STATE_DISABLED:
        if (iSecCounter1 > 0 || iMinCounter1 > 0)
        {
            if (millis() - autozero_start > autozero_delay)
            {
                iSecCounter1 = 0;
                iMinCounter1 = 0;
                state_Display = DISPLAY_STATE_TIMER_STOPPED;
            }
        }
        // waiting for START event from Reed Switch
        break;
    case COUNTER_STATE_START:
        iSecCounter1 = 0;
        iMinCounter1 = 0;
        elapsed_counter1 = 0;
        start_counter1 = millis();
        state_counter1 = COUNTER_STATE_COUNTING;
        state_Display = DISPLAY_STATE_TIMER_RUNNING;
        break;
    case COUNTER_STATE_COUNTING:
        prev_iSecCounter1 = iSecCounter1;
        elapsed_counter1 = millis() - start_counter1;
        iSecCounter1 = int((elapsed_counter1 / 1000) % 60);
        iMinCounter1 = int((elapsed_counter1 / 1000) / 60);
        if (iSecCounter1 != prev_iSecCounter1)
        {
            state_Display = DISPLAY_STATE_TIMER_RUNNING;
        }
        break;
    case COUNTER_STATE_STOP:
        autozero_start = millis();
        state_counter1 = COUNTER_STATE_DISABLED;
        state_Display = DISPLAY_STATE_TIMER_STOPPED;
        break;
    }
    if (prev_state_counter1 == state_counter1)
    {
        //do nothing
    }
    else
    {
// debug display
#ifdef SERIAL_DEBUG_ENABLED
        Serial.print(F("state_counter1: "));
        Serial.println(state_counter1, DEC);
#endif
    }
}

/**
* @brief Reed switch state machine, extended with a logic switch which handles the 50Hz switching of reed switch on the solenoid
*/
void StateMachine_Reed_Switch(void)
{
    int prev_virtual_Reed_Switch;
    //State Machine Section
    switch (state_Reed_Switch)
    {
    case REED_SWITCH_STATE_RESET: //RESET!
        // variables initialization
        bin_counter = 0;
        virtual_Reed_Switch = VIRT_REED_SWITCH_OFF;
        value_Reed_Switch = SW_OFF_LEVEL;
        state_Reed_Switch = REED_SWITCH_STATE_START_TIMER;
        break;

    case REED_SWITCH_STATE_START_TIMER: //Start SW timer
        //Start debounce timer and proceed to state3, "OFF, armed to ON"
        t_0_Reed_Switch = millis();
        state_Reed_Switch = REED_SWITCH_STATE_STOP_TIMER;
        break;

    case REED_SWITCH_STATE_STOP_TIMER: //Timer stop
        //Check to see if debounce delay has passed
        t_Reed_Switch = millis();
        if (t_Reed_Switch - t_0_Reed_Switch > bounce_delay_Reed_Switch)
        {
            state_Reed_Switch = REED_SWITCH_STATE_READ_PIN;
        }
        break;

    case REED_SWITCH_STATE_READ_PIN: //Read Switch pin
        value_Reed_Switch = digitalRead(pin_Reed_Switch);
        state_Reed_Switch = REED_SWITCH_STATE_ROTATE_BIN_COUNTER;
        break;
    case REED_SWITCH_STATE_ROTATE_BIN_COUNTER: //Rotate binary counter
        bin_counter = bin_counter << 1;
        if (value_Reed_Switch == SW_ON_LEVEL)
        {
            bin_counter++;
        }
        state_Reed_Switch = REED_SWITCH_STATE_SET_LOGIC_SWITCH;
        break;

    case REED_SWITCH_STATE_SET_LOGIC_SWITCH:
        prev_virtual_Reed_Switch = virtual_Reed_Switch;
        if (bin_counter > 0)
        {
            if (prev_virtual_Reed_Switch == VIRT_REED_SWITCH_OFF)
            {
                virtual_Reed_Switch = VIRT_REED_SWITCH_ON;
// debug display
#ifdef SERIAL_DEBUG_ENABLED
                Serial.println(F("Virtual Reed switch ON"));
#endif
            }
        }
        else
        {
            if (prev_virtual_Reed_Switch == VIRT_REED_SWITCH_ON)
            {
                virtual_Reed_Switch = VIRT_REED_SWITCH_OFF;
// debug display
#ifdef SERIAL_DEBUG_ENABLED
                Serial.println(F("Virtual Reed switch OFF"));
#endif
            }
        }
        state_Reed_Switch = REED_SWITCH_STATE_START_TIMER;

        break;
    }
}
