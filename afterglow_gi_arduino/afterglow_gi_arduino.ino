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
 *  | IN_DATA  | 74LS165 Q_H   | D2        | DDRD, 2       | Input        |
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
#define TTAG_INT (250)

// number of GI strings
#define NUM_STRINGS 5


//------------------------------------------------------------------------------
// global variables

// local time
static uint32_t sTtag = 0;

// D0-D4 interrupt timers
volatile byte sLastPIND = 0;
volatile uint32_t sDataIntDt[NUM_STRINGS] = {0};
volatile uint32_t sDataIntLast[NUM_STRINGS] = {0};
volatile uint32_t sZCIntTime = 0;



//------------------------------------------------------------------------------
void setup()
{
    // Use Timer1 to create an interrupt every TTAG_INT us.
    // This will be the heartbeat of our realtime task.
    noInterrupts(); // disable all interrupts
    TCCR1A = 0;
    TCCR1B = 0;
    // set compare match register for TTAG_INT us increments
    // prescaler is at 1, so counting real clock cycles
    OCR1A = (TTAG_INT * 16);  // [16MHz clock cycles]
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS10 bit so timer runs at clock speed
    TCCR1B |= (1 << CS10);  
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

    // I/O pin setup
    // D2-D7 are inputs
    DDRD = 0;

    // activate pin change interrupts on D2-D7
    PCICR |= 0b00000100;
    PCMSK2 |= 0b11111100;
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

    // enable all interrupts
    interrupts();

    // enable a strict 15ms watchdog
    wdt_enable(WDTO_15MS);
}

//------------------------------------------------------------------------------
// Timer1 interrupt handler
// This is the realtime task heartbeat. All the magic happens here.
ISR(TIMER1_COMPA_vect)
{
    // time is running
    uint16_t startCnt = TCNT1;
    sTtag++;

    // kick the dog
    wdt_reset();

}

//------------------------------------------------------------------------------
// Pin change interrupt on D2-D7 handler
// This is measuring the zero-crossing to blank signal time.
ISR(PCINT2_vect)
{
    // check which pin triggered this interrupt
    byte newPins = (sLastPIND ^ PIND);
    sLastPIND = PIND;

    // what time is it?
    uint32_t t = micros();

    if (newPins == B10000000)
    {
        // handle zero crossing interrupts

        // The zero crossing signal should appear at 100Hz. If we're closer to
        // last interrupt then this is the falling edge and we should ignore it.
        if ((t - sZCIntTime) > 4000)
        {
            // just remember the last zero crossing interrupt time
            sZCIntTime = t;
        }
    }
    else
    {
        // which pins triggered?
        uint8_t pinBit = 0B00000100; // start with D2
        for (uint8_t pinNum=0; pinNum<NUM_STRINGS; pinNum++, pinBit<<=1)
        {
            // measure and store the time since the last zero crossing interrupt
            if (newPins & pinBit)
            {
                // handle only once
                uint32_t dtLast = (t - sDataIntLast[pinNum]);
                if (dtLast > 1000)
                {
                    uint32_t dt = (t - sZCIntTime);
                    if (dt < 10000)
                    {
                        // store the delta time
                        sDataIntDt[pinNum] = dt;
                    }
                    sDataIntLast[pinNum] = t;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void loop()
{
    // The main loop is used for low priority serial communication only.
    // All the fun stuff happens in the interrupt handlers.

    // count the loops (used for debug output below)
    static uint32_t loopCounter = 0;
    loopCounter++;

#if DEBUG_SERIAL
    if ((loopCounter % 10) == 0)
    {
        Serial.print("ZC - ");
        Serial.print(sZCIntTime);
        Serial.println("us");
        for (uint8_t i=0; i<NUM_STRINGS; i++)
        {
            Serial.print(i);
            Serial.print(" - ");
            Serial.print(sDataIntDt[i]);
            Serial.println("us");
        }
	}
#endif

    // wait 500ms
    delay(500);
}
