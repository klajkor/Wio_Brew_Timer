#pragma once

#include <SPI.h>
#include <Wire.h>
#include <math.h>

// uncomment the line below if you would like to have debug messages
#define SERIAL_DEBUG_ENABLED 1

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
void StateMachine_Display(void);
void Display_Running_Timer(void);
void Display_Stopped_Timer(void);
void display_Timer_On_All(bool need_Display_Clear, bool need_Display_Stopped);
void show_temperature(char *pTemperatureStr);
void show_millivolt(char *pMillivoltStr);
