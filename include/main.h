#pragma once

#include <SPI.h>
#include <Wire.h>
#include <math.h>

// uncomment the line below if you would like to have debug messages
#define SERIAL_DEBUG_ENABLED 1

#define COUNTER_STATE_RESET 0
#define COUNTER_STATE_DISABLED 1
#define COUNTER_STATE_START 2
#define COUNTER_STATE_COUNTING 3
#define COUNTER_STATE_STOP 4

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

extern int  state_counter1;
extern int  prev_state_counter1;
extern int  iSecCounter1;
extern int  prev_iSecCounter1;
extern int  iMinCounter1;
extern int  prev_iMinCounter1;
extern char TimeCounterStr[];

//ADC variables
extern int pin_ADC_input;

extern int virtual_Reed_Switch;

/* Function declarations */

void set_background(void);
void GPIO_init(void);

void update_TimeCounterStr(int tMinutes, int tSeconds);
void StateMachine_counter1(void);
void StateMachine_Reed_Switch(void);

