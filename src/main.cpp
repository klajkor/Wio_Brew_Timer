#include <Arduino.h>

#include <TFT_eSPI.h>
#include "Free_Fonts.h"

#define SERIAL_DEBUG_ENABLED 1

#define COUNTER_STATE_RESET 0
#define COUNTER_STATE_DISABLED 1
#define COUNTER_STATE_START 2
#define COUNTER_STATE_COUNTING 3
#define COUNTER_STATE_STOP 4

#define DISPLAY_CLEAR_TRUE true
#define DISPLAY_CLEAR_FALSE false
#define DISPLAY_STOPPED_TRUE true
#define DISPLAY_STOPPED_FALSE false

#define SW_ON_LEVEL LOW   /* Switch on level */
#define SW_OFF_LEVEL HIGH /* Switch off level */

#define VIRT_REED_SWITCH_OFF 0
#define VIRT_REED_SWITCH_ON 1

#define REED_SWITCH_STATE_RESET 0
#define REED_SWITCH_STATE_START_TIMER 1
#define REED_SWITCH_STATE_STOP_TIMER 2
#define REED_SWITCH_STATE_READ_PIN 3
#define REED_SWITCH_STATE_ROTATE_BIN_COUNTER 4
#define REED_SWITCH_STATE_SET_LOGIC_SWITCH 5

#define DISPLAY_STATE_RESET 0
#define DISPLAY_STATE_TIMER_RUNNING 1
#define DISPLAY_STATE_TIMER_STOPPED 2
#define DISPLAY_STATE_HOLD_TIMER_ON 3
#define DISPLAY_STATE_TEMPERATURE 4
#define DISPLAY_STATE_DO_NOTHING 5

// EDIT THE FOLLOWING TO DECLARE BUTTON PINS FOR OTHER ARDUINO BOARDS
const auto start_button = WIO_KEY_C;
const auto stop_button = WIO_KEY_B;

unsigned long int timer_counter;
unsigned int brew_timer_counter;

//Switch Variables - "Reed_Switch"
int state_Reed_Switch = 0; //The actual ~state~ of the state machine
int pin_Reed_Switch = start_button;                //Input/Output (IO) pin for the switch, 10 = pin 10 a.k.a. D10
int value_Reed_Switch = 0;                      //Value of the switch ("HIGH" or "LOW")
unsigned long t_Reed_Switch = 0;                //Typically represents the current time of the switch
unsigned long t_0_Reed_Switch = 0;              //The time that we last passed through an interesting state
unsigned long bounce_delay_Reed_Switch = 5;     //The delay to filter bouncing
unsigned int bin_counter = 0;                   //binary counter for reed switch
int virtual_Reed_Switch = VIRT_REED_SWITCH_OFF; // virtual switch

//SM Display variables
int state_Display = 0;
unsigned long t_Display = 0;
unsigned long t_0_Display = 0;
unsigned long delay_For_Stopped_Timer = 5000; //millisec

//SM Counter variables
int state_counter1 = 0;      //The actual ~state~ of the state machine
int prev_state_counter1 = 0; //Remembers the previous state (useful to know when the state has changed)
int iSecCounter1 = -1;
int prev_iSecCounter1 = 0;
int iMinCounter1 = -1;
int prev_iMinCounter1 = 0;
unsigned long start_counter1 = 0;
unsigned long elapsed_counter1 = 0;

char TimeCounterStr[] = "0:00"; /** String to store time counter value, format: M:SS */

TFT_eSPI tft;

TFT_eSprite background(&tft);
TFT_eSprite brew_timer(&tft);
TFT_eSprite brew_temperature(&tft);

void update_TimeCounterStr(int tMinutes, int tSeconds);
void StateMachine_counter1(void);
void StateMachine_Reed_Switch(void);
void StateMachine_Display(void);
void Display_Running_Timer(void);
void Display_Stopped_Timer(void);
void display_Timer_On_All(bool need_Display_Clear, bool need_Display_Stopped);
void set_background(void);
void show_temperature(void);

void set_background(void)
{
    background.createSprite(320, 240);
    background.fillSprite(TFT_WHITE);
    background.fillRect(0, 0, 320, 60, TFT_DARKGREEN);
    background.setTextColor(TFT_WHITE);
    background.setFreeFont(FSSB18);
    background.setTextSize(1);
    background.drawString("LM Linea Mini", 50, 15);

    background.drawFastVLine(150, 60, 190, TFT_DARKGREEN);
    //background.drawFastHLine(0, 140, 320, TFT_DARKGREEN);
    background.pushSprite(0, 0);
}

void show_temperature(void)
{
    brew_temperature.createSprite(140, 60);
    brew_temperature.fillSprite(TFT_WHITE);
    brew_temperature.setCursor(0, 0);
    brew_temperature.setFreeFont(FSSB18);
    brew_temperature.setTextSize(1);
    brew_temperature.setTextColor(TFT_RED);
    brew_temperature.drawString("94.5 *C", 0, 0);
    brew_temperature.pushSprite(175, 135);
}

void setup()
{
    Serial.begin(115200);
#ifdef SERIAL_DEBUG_ENABLED
    Serial.println(F("Debugging is ON"));
#endif
    pinMode(start_button, INPUT_PULLUP);
    pinMode(stop_button, INPUT_PULLUP);

    StateMachine_counter1();
    StateMachine_Reed_Switch();
    state_Display = DISPLAY_STATE_RESET;
    StateMachine_Display();
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    set_background();
    brew_timer.createSprite(100, 60);
    display_Timer_On_All(true,true);
    show_temperature();
}

void loop()
{
    if (digitalRead(start_button) == LOW)
    {
    }
    if (digitalRead(stop_button) == LOW)
    {
    }
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
        state_counter1 = COUNTER_STATE_DISABLED;
        break;
    case COUNTER_STATE_DISABLED:
        //waiting for START event
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

void StateMachine_Display(void)
{

    switch (state_Display)
    {
    case DISPLAY_STATE_RESET:
        Display_Stopped_Timer();
        state_Display = DISPLAY_STATE_TEMPERATURE;
        break;

    case DISPLAY_STATE_TIMER_RUNNING:
        Display_Running_Timer();
        //Display_Temperature();
        state_Display = DISPLAY_STATE_TEMPERATURE;
        break;

    case DISPLAY_STATE_TIMER_STOPPED:
        Display_Stopped_Timer();
        t_0_Display = millis();
        state_Display = DISPLAY_STATE_TEMPERATURE;
        break;

    case DISPLAY_STATE_HOLD_TIMER_ON:
        t_Display = millis();
        if (t_Display - t_0_Display > delay_For_Stopped_Timer)
        {
            state_Display = DISPLAY_STATE_TEMPERATURE;
        }
        break;

    case DISPLAY_STATE_TEMPERATURE:
        //Display_Temperature();
        state_Display = DISPLAY_STATE_DO_NOTHING;
        break;

    case DISPLAY_STATE_DO_NOTHING:
        break;
    }
}

void Display_Running_Timer(void)
{
    display_Timer_On_All(DISPLAY_CLEAR_FALSE, DISPLAY_STOPPED_FALSE);
}

void Display_Stopped_Timer(void)
{
    display_Timer_On_All(DISPLAY_CLEAR_FALSE, DISPLAY_STOPPED_TRUE);
}

void display_Timer_On_All(bool need_Display_Clear, bool need_Display_Stopped)
{
    update_TimeCounterStr(iMinCounter1, iSecCounter1);
    // debug display
#ifdef SERIAL_DEBUG_ENABLED
    Serial.print(F("iMinCounter1: "));
    Serial.print(iMinCounter1, DEC);
    Serial.print(F(" iSecCounter1: "));
    Serial.println(iSecCounter1, DEC);
    Serial.println(TimeCounterStr);
#endif

    brew_timer.fillSprite(TFT_WHITE);
    brew_timer.setCursor(0, 0);
    brew_timer.setFreeFont(FSSB24);
    brew_timer.setTextSize(1);
    if (need_Display_Stopped)
    {
        brew_timer.setTextColor(TFT_DARKGREY);
    }
    else
    {
        brew_timer.setTextColor(TFT_NAVY);
    }
    brew_timer.drawString(TimeCounterStr, 0, 0);
    brew_timer.pushSprite(35, 130);
}

