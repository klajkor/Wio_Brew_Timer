#include "display.h"



// State Machine Display Global variables
int state_Display = 0;

// State Machine Display Local variables
static unsigned long t_Display = DISPLAY_STATE_RESET;
static unsigned long t_0_Display = 0;
static unsigned long delay_For_Stopped_Timer = 7000; // millisec
//static unsigned long t_Temp_Display = 0;
static unsigned long t_0_Temp_Display = 0;

static TFT_eSPI tft;

static TFT_eSprite background(&tft);
static TFT_eSprite brew_timer(&tft);
static TFT_eSprite brew_temperature(&tft);
static TFT_eSprite brew_millivolt(&tft);

void Display_Init(void)
{
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    Display_Set_Background();
    brew_timer.createSprite(100, 60);
    brew_temperature.createSprite(150, 105);
    //brew_millivolt.createSprite(100, 25);
}

void Display_Set_Background(void)
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
    background.deleteSprite();
}

void StateMachine_Display(void)
{
    unsigned long display_hold_timer;

    switch (state_Display)
    {
    case DISPLAY_STATE_RESET:
        t_0_Temp_Display = millis();
        Display_Stopped_Timer();
        t_0_Display = millis();
        state_Display = DISPLAY_STATE_TEMPERATURE;
        break;

    case DISPLAY_STATE_TIMER_RUNNING:
        Display_Running_Timer();
        state_Display = DISPLAY_STATE_TEMPERATURE;
        break;

    case DISPLAY_STATE_TIMER_STOPPED:
        Display_Stopped_Timer();
        t_0_Display = millis();
        state_Display = DISPLAY_STATE_TEMPERATURE;
        break;

    case DISPLAY_STATE_HOLD_TIMER_ON:
        t_Display = millis();
        display_hold_timer = t_Display - t_0_Display;
        if (display_hold_timer > delay_For_Stopped_Timer &&
            ((display_hold_timer % DISPLAY_TEMPERATURE_FREQ_MILLISEC) == 0))
        {
            state_Display = DISPLAY_STATE_TEMPERATURE;
        }
        else
        {
            if ((display_hold_timer % DISPLAY_TEMPERATURE_FREQ_MILLISEC) == 0)
            {
                state_Display = DISPLAY_STATE_TEMPERATURE;
            }
        }
        break;

    case DISPLAY_STATE_TEMPERATURE:
        Display_Temperature(TEMPERATURE_STR_V2, MILLI_VOLT_STR);
        if (virtual_Reed_Switch == VIRT_REED_SWITCH_OFF)
        {
            state_Display = DISPLAY_STATE_HOLD_TIMER_ON;
        }
        else
        {
            state_Display = DISPLAY_STATE_DO_NOTHING;
        }
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


void Display_Temperature(char *pTemperatureStr, char *pMillivoltStr)
{
    char disp_temp_string[9];
    char disp_mv_string[9];

    snprintf(disp_temp_string,9,"%s *C",pTemperatureStr);
    brew_temperature.fillSprite(TFT_WHITE);
    brew_temperature.setCursor(0, 0);
    brew_temperature.setFreeFont(FSSB18);
    brew_temperature.setTextSize(1);
    brew_temperature.setTextColor(TFT_RED);
    brew_temperature.drawString(disp_temp_string, 0, 0);

    snprintf(disp_mv_string,9,"%s mV",pMillivoltStr);
    brew_temperature.setCursor(25, 80);
    brew_temperature.setFreeFont(FSS9);
    brew_temperature.setTextSize(1);
    brew_temperature.setTextColor(TFT_BLUE);
    brew_temperature.drawString(disp_mv_string, 25, 80);

    brew_temperature.pushSprite(170, 135);
}
