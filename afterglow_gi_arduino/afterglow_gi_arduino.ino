/***********************************************************************
 *  Afterglow GI:
 *      Copyright (c) 2019 Christoph Schmid
 *
 ***********************************************************************
 *  This file is part of the Afterglow GI pinball LED project:
 *  https://github.com/smyp/afterglow_gi
 *
 *  Afterglow GI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  Afterglow GI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with Afterglow GI.
 *  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/
 
//------------------------------------------------------------------------------
/* This code assumes following pin layout:
 *
 *  +----------+---------------+-----------+---------------+--------------+
 *  | Name     | Function      | Nano Pin# | Register, Bit | Mode         |
 *  +----------+---------------+-----------+---------------+--------------+
 *  | STR1_IN  | D0 Data Bus   | D2        | DDRD, 2       | Input        |
 *  | STR2_IN  | D1 Data Bus   | D3        | DDRD, 3       | Input        |
 *  | STR3_IN  | D2 Data Bus   | D4        | DDRD, 4       | Input        |
 *  | STR4_IN  | D3 Data Bus   | D8        | DDRB, 0       | Input        |
 *  | STR5_IN  | D4 Data Bus   | D12       | DDRB, 4       | Input        |
 *  | ZC       | Zero Crossing | D7        | DDRD, 7       | Input        |
 *  | STR1_OUT | STR 1 PWM     | D5        | DDRD, 5       | Output       |
 *  | STR2_OUT | STR 2 PWM     | D9        | DDRB, 1       | Output       |
 *  | STR3_OUT | STR 3 PWM     | D10       | DDRB, 2       | Output       |
 *  | STR4_OUT | STR 4 PWM     | D11       | DDRB, 3       | Output       |
 *  | STR5_OUT | STR 5 PWM     | D6        | DDRD, 6       | Output       |
 *  | POT      | Potentiometer | A1        | DDRC, 1       | Input        |
 *  +----------+---------------+-----------+---------------+--------------+
*/

#include <EEPROM.h>
#include <avr/wdt.h>
#include <avr/boot.h>

//------------------------------------------------------------------------------
// Setup

// Afterglow GI version number
#define AFTERGLOW_GI_VERSION 101

// Afterglow GI board revision. Currently v1.1.
#define BOARD_REV 11

// turn debug output via serial on/off
#define DEBUG_SERIAL 1

// voltage measurement pin
#define V_MEAS_PIN A0

// local time interval (us)
#define TTAG_INT 1024

// Number of GI strings
#define NUM_STRINGS 5

// Number of brightness steps
#define NUM_BRIGHTNESS 8

// PWM resolution (given by the Arduino HW)
#define PWM_NUM_STEPS 255

// Number of center brightness values required to switch value
#define BRIGHTNESS_SWITCH_THRESH 5

// Brightness interval margin to account for overlapping intervals [us]
#define BRIGHTNESS_INTERVAL_MARGIN 150

// Uncertain brightness value
#define BRIGHTNESS_UNCERTAIN 255

// Brightness interpolation duration [2^n ms, e.g. 8=256ms]
#define BRIGHTNESS_INTERPOL_DURBITS 8

// Brightness interpolation steps (2^n, e.g. 3=8 steps)
#define BRIGHTNESS_INTERPOL_STEPBITS 3

// Duration of an interpolation step [2^n ms]
#define BRIGHTNESS_INTERPOL_INTBITS (BRIGHTNESS_INTERPOL_DURBITS - BRIGHTNESS_INTERPOL_STEPBITS)

// Default input voltage (12V)
#define INPUT_VOLTAGE_DEFAULT 120

// Measure the input voltage (if set to 1)
// If disabled the hardcoded default voltage (INPUT_VOLTAGE_DEFAULT) is used
#define MEASURE_VOLTAGE 1


//------------------------------------------------------------------------------
// global variables

// local time
static volatile uint32_t sTtag = 0;

static byte sLastPIND = 0;
static byte sLastPINB = 0;
static uint32_t sDataIntLast[NUM_STRINGS] = {0};
static uint8_t sBrightnessN[NUM_STRINGS] = {0};
static uint32_t sZCIntTime = 0;
static uint8_t sInterruptsSeen = 0;
static volatile uint8_t sBrightness[NUM_STRINGS] = {0};
static volatile uint8_t sBrightnessTarget[NUM_STRINGS] = {0};
static volatile uint32_t sBrightnessLastUpd[NUM_STRINGS] = {0};
static volatile uint8_t sBrightnessIntLastStep[NUM_STRINGS];
static volatile uint8_t sBrightnessHist[NUM_STRINGS][NUM_BRIGHTNESS+1] = {0}; // including 0 for 'off'
static volatile uint32_t sBrightnessPot = 0;

static uint8_t sDutyCycleTable[NUM_BRIGHTNESS+1] = {0}; // including 0 for 'off'
static uint8_t sVoltage = INPUT_VOLTAGE_DEFAULT;

#if DEBUG_SERIAL
static volatile uint32_t sMinDt[NUM_STRINGS] = {0};
static volatile uint32_t sMaxDt[NUM_STRINGS] = {0};
#endif


//------------------------------------------------------------------------------
void setup()
{
    // Use Timer1 to create an interrupt every TTAG_INT us.
    // This will be the heartbeat of our realtime task.
    noInterrupts(); // disable all interrupts
    TCCR1B = 0;

    // set compare match register for TTAG_INT us increments
    // prescaler is at 64, so counting 64 x real clock cycles
    OCR1A = (TTAG_INT / 16);  // [16MHz x 64 (prescaler) clock cycles]
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS10 and CS11 bits to set prescaler to 64
    TCCR1B |= (1 << CS10);
    TCCR1B |= (1 << CS11);
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

    // I/O pin setup
    // D2-D4, D7-D8 and D12 are inputs
    DDRD &= B01100011;
    DDRB &= B11101110;
    // D5, D6 and D9-D11 are outputs, pull them low
    DDRC |= B01100000;
    DDRB |= B00001110;
    PORTC &= B10011111;
    PORTB &= B11110001;

    // activate pin change interrupts on D2-D4, D7-D8 and D12 (interrupt masks 0 and 2)
    PCICR |= 0b00000101;
    PCMSK0 |= 0b00010001; // D8 and D12
    PCMSK2 |= 0b10011100; // D2-D4 and D7
    // clear any outstanding interrupts
    PCIFR = 0;

    // enable serial output at 115200 baudrate
    Serial.begin(115200);
    Serial.print("Afterglow GI v");
    Serial.print(AFTERGLOW_GI_VERSION);
    Serial.println(" (c) 2019 morbid cornflakes");
    // check the extended fuse for brown out detection level
    uint8_t efuse = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
    Serial.println("-----------------------------------------------");
    uint8_t bodBits = (efuse & 0x7);
    Serial.print("efuse BOD ");
    Serial.println((bodBits == 0x07) ? "OFF" : (bodBits == 0x04) ? "4.3V" : (bodBits == 0x05) ? "2.7V" : "1.8V");

#if MEASURE_VOLTAGE
    // measure the input voltage
    sVoltage = measureInputVoltage();
    Serial.print("Input voltage ");
    Serial.println(sVoltage);
#endif

    // populate the duty cycle table
    populateDutyCycleTable(sVoltage);

    // initialize data
    memset((void*)sBrightnessIntLastStep, 255, sizeof(sBrightnessIntLastStep));

    // read the global brightness (potientometer) value
    sBrightnessPot = analogRead(A1);

    // enable all interrupts
    interrupts();

    // enable a strict 15ms watchdog
    //wdt_enable(WDTO_15MS);
}

//------------------------------------------------------------------------------
// Timer1 interrupt handler
// This is the realtime task heartbeat. All the output magic happens here.
ISR(TIMER1_COMPA_vect)
{
    // Time is ticking
    sTtag++;

    // Update all GI strings
    updateGI();

    // Kick the dog
    //wdt_reset();

}

//------------------------------------------------------------------------------
void newBrightness(uint8_t string, uint8_t b)
{
    // Check whether the brightness has changed
    if (b <= NUM_BRIGHTNESS)
    {
        // Add the current brightness value to the histogram
        if (sBrightnessHist[string][b] < 0xff)
        {
            sBrightnessHist[string][b]++;
        }

        // Only switch when some measurements in the center of the
        // brightness interval have been seen
        if (sBrightnessHist[string][b] > BRIGHTNESS_SWITCH_THRESH)
        {
            if (b != sBrightnessTarget[string])
            {
                // abort current interpolation if not done yet
                sBrightness[string] = sBrightnessTarget[string];
    
                // switch to the new brightness target value
                sBrightnessTarget[string] = b;
                sBrightnessLastUpd[string] = sTtag;
            }

            // clear the histogram
            memset((void*)&(sBrightnessHist[string][0]), 0, NUM_BRIGHTNESS+1);
        }
    }
}

//------------------------------------------------------------------------------
void handlePinChange(uint32_t t, uint8_t newPinMask, uint8_t pinBit, uint8_t string)
{
    // Measure and store the time since the last zero crossing interrupt
    if (newPinMask & pinBit)
    {
        // Handle only once
        uint32_t dtLast = (t - sDataIntLast[string]);
        if (dtLast > 1000)
        {
            uint8_t stringBit = (1<<string);
            uint32_t dt = (t - sZCIntTime);
            if (dt < 10000)
            {
                // Translate delta time into brightness
                uint8_t b = dtToBrightness(dt);

                // Handle the new brightness value
                newBrightness(string, b);

#if DEBUG_SERIAL
                if ((dt < sMinDt[string]) || (sMinDt[string] == 0)) sMinDt[string] = dt;
                if (dt > sMaxDt[string]) sMaxDt[string] = dt;
#endif
            }
            sDataIntLast[string] = t;
            sInterruptsSeen |= stringBit;
        }
    }
}

//------------------------------------------------------------------------------
void handlePinInterrupts()
{
    // The WPC CPU issues a short signal to turn on the triac controlling the
    // AC voltage. The Triac will stay on until the next zero crossing of the
    // AC signal.
    // The closer the signal is to the zero crossing, the brighter the lamps
    // will be. If no signal is issued at all (stays high), the lamps will light
    // at full power. If the signal remains low, the lamps are turned off.
    // The zero crossing signal is issued with twice the AC frequency, i.e. with
    // 100Hz or every 10ms in Europe.
    // The scope says the ZC signal is 1ms long while the Triac signal is only
    // ~8us long.

    // ZC Sig          TR Sig        ZC Sig          ZC     Zero Crossing Signal
    // |                |            |               TR     Triac enable signal
    // |--+  |  |  |  | v|  |  |  |  |--+            B0-B6  Brightness 1-7 levels (WPC GI)
    // |ZC|                          |ZC|
    // |  |B7 B6 B5 B4 B3 B2 B1      |  |
    // +-----------------------------+------
    // 0ms                           10ms

    // Check which pins triggered this interrupt
    byte newPinB = (sLastPINB ^ PINB);
    sLastPINB = PINB;
    byte newPinD = (sLastPIND ^ PIND);
    sLastPIND = PIND;

    // What time is it?
    uint32_t t = micros();

    if (newPinD & B10000000) // ZC on D7
    {
        // Handle zero crossing interrupts

        // The zero crossing signal should appear at 100Hz. If we're closer to
        // last interrupt then this is the falling edge and we should ignore it.
        if ((t - sZCIntTime) > 4000)
        {
            // Just remember the last zero crossing interrupt time
            sZCIntTime = t;

            // All strings without interrupt in the past interval are either fully
            // on or off
            if ((sInterruptsSeen & 0x01) == 0) newBrightness(0, (PIND & 0B00000100) ? 0: NUM_BRIGHTNESS);
            if ((sInterruptsSeen & 0x02) == 0) newBrightness(1, (PIND & 0B00001000) ? 0: NUM_BRIGHTNESS);
            if ((sInterruptsSeen & 0x04) == 0) newBrightness(2, (PIND & 0B00010000) ? 0: NUM_BRIGHTNESS);
            if ((sInterruptsSeen & 0x08) == 0) newBrightness(3, (PINB & 0B00000001) ? 0: NUM_BRIGHTNESS);
            if ((sInterruptsSeen & 0x10) == 0) newBrightness(4, (PINB & 0B00010000) ? 0: NUM_BRIGHTNESS);
    
            // Clear the interrupts mask
            sInterruptsSeen = 0;
        }
    }

    // Handle all strings
    handlePinChange(t, newPinD, B00000100, 0); // String 0 on D2
    handlePinChange(t, newPinD, B00001000, 1); // String 1 on D3
    handlePinChange(t, newPinD, B00010000, 2); // String 2 on D4
    handlePinChange(t, newPinB, B00000001, 3); // String 3 on D8
    handlePinChange(t, newPinB, B00010000, 4); // String 4 on D12
}

//------------------------------------------------------------------------------
// Pin change interrupt on D2-D7 handler
// This is measuring the zero-crossing to blank signal time.
ISR(PCINT0_vect)
{
    // handle the interrupts
    handlePinInterrupts();
}

//------------------------------------------------------------------------------
// Pin change interrupt on D2-D7 handler
// This is measuring the zero-crossing to blank signal time.
ISR(PCINT2_vect)
{
    // handle the interrupts
    handlePinInterrupts();
}

//------------------------------------------------------------------------------
void loop()
{
    // The main loop is used for low priority serial communication only.
    // All the fun stuff happens in the interrupt handlers.

    // Count the loops (used for debug output below)
    static uint32_t loopCounter = 0;
    loopCounter++;
#if DEBUG_SERIAL
    if ((loopCounter % 10) == 0)
    {
        Serial.print("T ");
        Serial.println(sTtag);
        Serial.print("ZC - ");
        Serial.print(sZCIntTime);
        Serial.println("us");
        Serial.print("Global Br ");
        Serial.println(sBrightnessPot);
        for (uint8_t i=0; i<NUM_STRINGS; i++)
        {
            Serial.print(i);
            Serial.print(" - ");
            Serial.print(sBrightness[i]);
            Serial.print(" min ");
            Serial.print(sMinDt[i]);
            Serial.print(" max ");
            Serial.println(sMaxDt[i]);
        }
	}
    if ((loopCounter % 100) == 0)
    {
        for (uint8_t i=0; i<NUM_STRINGS; i++)
        {
            sMinDt[i] = sMaxDt[i] = 0;
        }
   }
#endif

    // wait 500ms
    delay(500);
}

//------------------------------------------------------------------------------
uint8_t dtToBrightness(uint32_t dt)
{
    // This function leaves some margin at the interval borders to account for
    // the fact the intervals are slightly overlapping and therefore a
    // unambiguous brightness determination is not always possible. In this case
    // the function returns 255.

    uint8_t b;
    if (dt < (1200-BRIGHTNESS_INTERVAL_MARGIN))
    {
        // full power, this shouldn't really happen
        b = NUM_BRIGHTNESS;
    }
    else if ((dt > (1200+BRIGHTNESS_INTERVAL_MARGIN)) &&
             (dt < (2200-BRIGHTNESS_INTERVAL_MARGIN)))
    {
        b = 6;
    }
    else if ((dt > (2200+BRIGHTNESS_INTERVAL_MARGIN)) &&
             (dt < (3200-BRIGHTNESS_INTERVAL_MARGIN)))
    {
        b = 5;
    }
    else if ((dt > (3200+BRIGHTNESS_INTERVAL_MARGIN)) &&
             (dt < (4200-BRIGHTNESS_INTERVAL_MARGIN)))
    {
        b = 4;
    }
    else if ((dt > (4200+BRIGHTNESS_INTERVAL_MARGIN)) &&
             (dt < (5200-BRIGHTNESS_INTERVAL_MARGIN)))
    {
        b = 3;
    }
    else if ((dt > (5200+BRIGHTNESS_INTERVAL_MARGIN)) &&
             (dt < (6200-BRIGHTNESS_INTERVAL_MARGIN)))
    {
        b = 2;
    }
    else if ((dt > (6200+BRIGHTNESS_INTERVAL_MARGIN)) &&
             (dt < (7200-BRIGHTNESS_INTERVAL_MARGIN)))
    {
        b = 1;
    }
    else
    {
        // invalid interval
        b = BRIGHTNESS_UNCERTAIN;
    }
    return b;
}

//------------------------------------------------------------------------------
void updateGI()
{
    static const uint8_t skPWMPins[NUM_STRINGS] = {5, 9, 10, 11, 6};

    // update all brightness values
    for (uint8_t i=0; i<NUM_STRINGS; i++)
    {
        if (sBrightness[i] != sBrightnessTarget[i])
        {
            uint32_t dt = (sTtag - sBrightnessLastUpd[i]);
            uint32_t step = (dt >> BRIGHTNESS_INTERPOL_INTBITS);
            int16_t b1 = 0;
            int16_t b2 = 0;
            uint8_t b;
            bool newB = false;
            if (step >= (1 << BRIGHTNESS_INTERPOL_STEPBITS))
            {
                // target brightness reached
                sBrightness[i] = sBrightnessTarget[i];
                sBrightnessIntLastStep[i] = 255;
                step = (1 << BRIGHTNESS_INTERPOL_STEPBITS);
                b = sDutyCycleTable[sBrightness[i]];
                newB = true;
            }
            else if (step != sBrightnessIntLastStep[i])
            {
                // Adjust the PWM if required
                b1 = sDutyCycleTable[sBrightness[i]];
                b2 = sDutyCycleTable[sBrightnessTarget[i]];
                int16_t diff = (b2 - b1);
                int16_t stepSize = (diff > 0) ? (diff >> BRIGHTNESS_INTERPOL_STEPBITS) : -((-diff) >> BRIGHTNESS_INTERPOL_STEPBITS);
                b = (uint8_t)(b1 + stepSize * step);
                newB = true;
                sBrightnessIntLastStep[i] = step;
            }

            // apply the new PWM value
            if (newB)
            {
                // scale the brightness with the potentiometer value
                uint8_t scaledB = (uint8_t)(((uint32_t)b * sBrightnessPot) >> 10);
                if ((scaledB == 0) && (b != 0))
                {
                    b = 1; // minimum brightness
                }
                else
                {
                    b = scaledB; // scaled brightness
                }

                // apply the PWM
                analogWrite(skPWMPins[i], b);
 #if DEBUG_SERIAL && 0
                if (i==1)
                {
                    Serial.print("[");
                    Serial.print(i);
                    Serial.print(" / ");
                    Serial.print(step);
                    Serial.print("] ");
                    Serial.print(sTtag);
                    Serial.print(" ");
                    Serial.print(dt);
                    Serial.print(" - ");
                    Serial.print(sBrightness[i]);
                    Serial.print(" (");
                    Serial.print(b1);
                    Serial.print(")");
                    Serial.print("->");
                    Serial.print(sBrightnessTarget[i]);
                    Serial.print(" (");
                    Serial.print(b2);
                    Serial.print(")");
                    Serial.print(" ");
                    Serial.println(b);
                }
#endif
            }
        }
    }
}

//------------------------------------------------------------------------------
void populateDutyCycleTable(uint8_t voltage)
{
    // Assume 6.3V LEDs with 3.2V drop -> 124 Ohm resistor for 25mA current
    uint32_t c = ((uint32_t)voltage - 32)*100/124;
    uint8_t dc = (uint8_t)(25 * 100 / c);
    uint8_t maxDC = (PWM_NUM_STEPS * dc / 100);

    Serial.print("CYCLE TABLE");

#if 0
    // linear distribution
    sDutyCycleTable[1] = 1; // darkest possible
    for (uint8_t i=2; i<=NUM_BRIGHTNESS; i++)
    {
        sDutyCycleTable[i] = ((i-1)*maxDC/(NUM_BRIGHTNESS-1));
    }
    Serial.println(" LIN");
#else
    // logarithmic distribution
    uint32_t r = (maxDC - 1);
    double m = log10(NUM_BRIGHTNESS);
    double s = 1.0/m;
    for (uint8_t i=1; i<=NUM_BRIGHTNESS; i++)
    {
        sDutyCycleTable[i] = 1 + r - floor(log10(NUM_BRIGHTNESS-i+1) * s * (double)r);
    }
    Serial.println(" LOG");
#endif

    // print the table
    Serial.print("max dc ");
    Serial.print(dc);
    Serial.print("% = ");
    Serial.print(maxDC);
    Serial.println("");
    for (uint8_t i=1; i<=NUM_BRIGHTNESS; i++)
    {
        Serial.print(i);
        Serial.print(" - ");
        Serial.println(sDutyCycleTable[i]);
    }
}

#if MEASURE_VOLTAGE
//------------------------------------------------------------------------------
uint8_t measureInputVoltage()
{
    // do some analog readings
    uint32_t v = 0;
    for (int i=0; i<10; i++)
    {
        int vmeas = analogRead(V_MEAS_PIN);
        v += vmeas;
#if DEBUG_SERIAL
        Serial.println(vmeas);
#endif
    }

    // take the average
    v /= 10;

    // convert to [0.1 volts]
    // respecting the 1MOhm/300kOhm voltage divider at the input
    // v has 10 bits [0..1023]
    // V_in = V_out * ((R1+R2)/R2) = V_out * 4.333
    v *= 100;
    v /= 472;

    // paranoia ckeck
    // force voltage to be between 8V and 15V
    if ((v<80) || (v>150))
    {
        // use default
#if DEBUG_SERIAL
        Serial.println("Forcing voltage to default voltage!");
#endif
        v = INPUT_VOLTAGE_DEFAULT;
    }
    
    return (uint8_t)v;
}
#endif
