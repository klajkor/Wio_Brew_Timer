#include "temperature_meter.h"

//#define SERIAL_DEBUG_ENABLED

// Thermistor calculation values
// Original idea and code from Jimmy Roasts, https://github.com/JimmyRoasts/LaMarzoccoTempSensor

// resistance at 25 degrees C
#define THERMISTORNOMINAL_V1 (50000U) // version 1
#define THERMISTORNOMINAL_V2 (49120U) // version 2 updated calculation

// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL (25U)

// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT_V1 (4400U) // version 1
#define BCOEFFICIENT_V2 (3977U) // version 2, updated calculation
// the value of the 'other' resistor
#define SERIESRESISTOR_V1 (6960U)
#define SERIESRESISTOR_V2 (6190U) // version 2, measured on board

// reference Vcc voltage
#define LMREF ((float)(5.07)) // measured from LMBoard --- GND Board

// Input voltage divider resistance values:
// (required to ensure that the voltage at the input is less than 3.3V)
#define VOLT_DIV_R1 ((float)(3.41)) // MegaOhm
#define VOLT_DIV_R2 ((float)(5.0)) // MegaOhm
// To multiply the measured voltage with to get the real voltage before the divider:
#define VOLT_DIV_MULTIPLIER ((float)((VOLT_DIV_R1 + VOLT_DIV_R2) / VOLT_DIV_R2))


// State Machine "Volt Meter" variables
static int           state_volt_meter = VOLT_METER_STATE_RESET;
static unsigned long t_volt_meter = 0;
static unsigned long t_0_volt_meter = 0;
static unsigned long delay_between_2_measures = 95; // In milliseconds

// Global temperature strings
char TEMPERATURE_STR_V2[6] = "999.9";
char TEMPERATURE_STR_LED_V2[5] = "99.9";
char MILLI_VOLT_STR[6] = "99999";

float get_thermistor_voltage(void)
{
    float bus_voltage_V;
    int ADC_raw;

    bus_voltage_V = 1.23;
    ADC_raw=analogRead(pin_ADC_input);
    bus_voltage_V = ((3.3/1023) * ADC_raw) * VOLT_DIV_MULTIPLIER;

    // debug display
#ifdef SERIAL_DEBUG_ENABLED
    //Serial.print(F("Raw ADC: "));
    //Serial.print(ADC_raw);
    //Serial.println(F(" "));
#endif
    return bus_voltage_V;
}

void state_machine_volt_meter(void)
{
    float measured_V;
    float head_temperature;

    switch (state_volt_meter)
    {
    case VOLT_METER_STATE_RESET:
        state_volt_meter = VOLT_METER_STATE_START_TIMER;
        head_temperature = 0.0;
        break;

    case VOLT_METER_STATE_START_TIMER:
        t_0_volt_meter = millis();
        state_volt_meter = VOLT_METER_STATE_STOP_TIMER;
        break;

    case VOLT_METER_STATE_STOP_TIMER:
        t_volt_meter = millis();
        if (t_volt_meter - t_0_volt_meter > delay_between_2_measures)
        {
            state_volt_meter = VOLT_METER_STATE_READ_VOLTAGE;
        }
        break;

    case VOLT_METER_STATE_READ_VOLTAGE:
        measured_V = get_thermistor_voltage();
        head_temperature = calculate_temperature_v2(measured_V);
        update_temperature_str(head_temperature, TEMPERATURE_STR_V2, TEMPERATURE_STR_LED_V2);
        update_millivolt_str(measured_V, MILLI_VOLT_STR);
        state_volt_meter = VOLT_METER_STATE_START_TIMER;
        break;
    }
}

float calculate_temperature_v2(float thermistor_voltage)
{
    float steinhart = 0.0;
    float calc_Temperature_V2 = 0.0;
    float thermistor_Res = 0.00; // Thermistor calculated resistance

    thermistor_Res = SERIESRESISTOR_V2 * (1 / ((LMREF / thermistor_voltage) - 1.0));
    steinhart = thermistor_Res / THERMISTORNOMINAL_V2;
    steinhart = log(steinhart);                         // ln(R/Ro)
    steinhart = steinhart / BCOEFFICIENT_V2;            // 1/B * ln(R/Ro)
    steinhart += (1.0 / (TEMPERATURENOMINAL + 273.15)); // + (1/To)
    steinhart = 1.0 / steinhart;                        // Invert
    calc_Temperature_V2 = (float)steinhart - 273.15;    // convert to C
    if (calc_Temperature_V2 <= -10)
    {
        calc_Temperature_V2 = -9.9;
    }
    if (calc_Temperature_V2 > 199.9)
    {
        calc_Temperature_V2 = 199.9;
    }
    return calc_Temperature_V2;
}

void update_temperature_str(float temperature_i, char *p_temperature_str_o, char *p_temperature_str_led_o)
{
    dtostrf(temperature_i, 5, 1, p_temperature_str_o);
    if (temperature_i >= 100.0)
    {
        dtostrf(temperature_i, 4, 0, p_temperature_str_led_o);
    }
    else
    {
        dtostrf(temperature_i, 4, 1, p_temperature_str_led_o);
    }
// debug display
#ifdef SERIAL_DEBUG_ENABLED
    //Serial.print(TEMPERATURE_STR_V2);
    //Serial.println(F(" *C"));
#endif
}

void update_millivolt_str(float thermistor_voltage_i, char *p_millivolt_str_o)
{
    int millivolt;
    millivolt = (int)(thermistor_voltage_i * 1000);
    // convert to text
    dtostrf(millivolt, 5, 0, p_millivolt_str_o);
// debug display
#ifdef SERIAL_DEBUG_ENABLED
    Serial.print(p_millivolt_str_o);
    Serial.println(F(" mV"));
#endif
}
