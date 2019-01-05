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
#define AFTERGLOW_GI_VERSION 100

// Afterglow configuration version
#define AFTERGLOW_CFG_VERSION 1

// Afterglow GI board revision. Currently v1.0.
#define BOARD_REV 10

// turn debug output via serial on/off
#define DEBUG_SERIAL 1

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
static volatile uint8_t sBrightnessChanged = 0;
static volatile uint16_t sBrightnessHist[NUM_BRIGHTNESS+1] = {0};

static uint8_t sDutyCycleTable[NUM_BRIGHTNESS+1] = {0};
static uint8_t sVoltage = 120; // 12V

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

    // populate the duty cycle table
    populateDutyCycleTable(sVoltage);

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

                // Check whether the brightness has changed
                if ((b != BRIGHTNESS_UNCERTAIN) && (b != sBrightness[string]))
                {
                    // Add the current brightness value to the histogram
                    if (sBrightnessHist[b] < 0xffff)
                    {
                        sBrightnessHist[b]++;
                    }

                    // Only switch when some measurements in the center of the
                    // brightness interval have been seen
                    if (sBrightnessHist[b] > BRIGHTNESS_SWITCH_THRESH)
                    {
                        // switch to the new brightness value
                        sBrightness[string] = b;
                        sBrightnessChanged |= stringBit;
                        memset((void*)sBrightnessHist, 0, sizeof(sBrightnessHist));
                    }
                }
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

    // check which pins triggered this interrupt
    byte newPinB = (sLastPINB ^ PINB);
    sLastPINB = PINB;
    byte newPinD = (sLastPIND ^ PIND);
    sLastPIND = PIND;

    // what time is it?
    uint32_t t = micros();

    if (newPinD & B10000000) // ZC on D7
    {
        // Handle zero crossing interrupts

        // The zero crossing signal should appear at 100Hz. If we're closer to
        // last interrupt then this is the falling edge and we should ignore it.
        if ((t - sZCIntTime) > 4000)
        {
            // just remember the last zero crossing interrupt time
            sZCIntTime = t;

            // All strings without interrupt in the past interval are either fully
            // on or off
            if ((sInterruptsSeen & 0x01) == 0) sBrightness[0] = (PIND & 0B00000100) ? 0: NUM_BRIGHTNESS;
            if ((sInterruptsSeen & 0x02) == 0) sBrightness[1] = (PIND & 0B00001000) ? 0: NUM_BRIGHTNESS;
            if ((sInterruptsSeen & 0x04) == 0) sBrightness[2] = (PIND & 0B00010000) ? 0: NUM_BRIGHTNESS;
            if ((sInterruptsSeen & 0x08) == 0) sBrightness[3] = (PINB & 0B00000001) ? 0: NUM_BRIGHTNESS;
            if ((sInterruptsSeen & 0x10) == 0) sBrightness[4] = (PINB & 0B00010000) ? 0: NUM_BRIGHTNESS;
    
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
    if (dt < (1200+BRIGHTNESS_INTERVAL_MARGIN))
    {
        // full power, this shouldn't really happen
        b = NUM_BRIGHTNESS;
    }
    else if (dt < (2200-BRIGHTNESS_INTERVAL_MARGIN))
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
    // Adjust the PWM if required
    if (sBrightnessChanged & 0x01) analogWrite(5, sDutyCycleTable[sBrightness[0]]);
    if (sBrightnessChanged & 0x02) analogWrite(9, sDutyCycleTable[sBrightness[1]]);
    if (sBrightnessChanged & 0x04) analogWrite(10, sDutyCycleTable[sBrightness[2]]);
    if (sBrightnessChanged & 0x08) analogWrite(11, sDutyCycleTable[sBrightness[3]]);
    if (sBrightnessChanged & 0x10) analogWrite(6, sDutyCycleTable[sBrightness[4]]);
    sBrightnessChanged = 0;
}

//------------------------------------------------------------------------------
void populateDutyCycleTable(uint8_t voltage)
{
    // Assume 6.3V LEDs with 3.2V drop -> 124 Ohm resistor for 25mA current
    uint32_t c = ((uint32_t)voltage - 32)*100/124;
    uint8_t dc = (uint8_t)(25 * 100 / c);
    uint8_t maxDC = (PWM_NUM_STEPS * dc / 100);

    Serial.println("CYCLE TABLE");
    Serial.print("max dc ");
    Serial.print(dc);
    Serial.println("%");
    for (uint8_t i=1; i<=NUM_BRIGHTNESS; i++)
    {
        sDutyCycleTable[i] = (i*maxDC/NUM_BRIGHTNESS);
        Serial.print(i);
        Serial.print(" - ");
        Serial.println(sDutyCycleTable[i]);
    }
}
