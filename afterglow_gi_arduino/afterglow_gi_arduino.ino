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
#define DEBUG_SERIAL 0

// local time interval (us)
#define TTAG_INT (250)

// number of GI strings
#define NUM_STRINGS 5


//------------------------------------------------------------------------------
// global variables

   
// local time
static uint32_t sTtag = 0;


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
    // 74LS165 LOAD and CLK are output, DATA is input
    // 74HC595 LOAD, CLK and DATA are output
    DDRD = B11111001;
    // nano LED output on pin 13, testmode jumper on pin 10
    DDRB = B00100000;
    // activate the pullups for the testmode pins
    PORTB |= B00001111;
    // OE on A1, DBG on A2, current meas on A0
    DDRC = B00000110;
    // keep OE high
    PORTC |= B00000010;

    // Configure the ADC clock to 1MHz by setting the prescaler to 16.
    // This should allow for fast analog pin sampling without much loss of precision.
    // defines for setting and clearing register bits.
    _SFR_BYTE(ADCSRA) |= _BV(ADPS2);
    _SFR_BYTE(ADCSRA) &= ~_BV(ADPS1);
    _SFR_BYTE(ADCSRA) &= ~_BV(ADPS0);

    // initialize the data
    memset(sMatrixState, 0, sizeof(sMatrixState));

    // load the configuration from EEPROM
    int err;
    bool cfgLoaded = loadCfg(&err);
    if (cfgLoaded == false)
    {
        // set default configuration
        setDefaultCfg();

        // store the configuration to EEPROM
        saveCfgToEEPROM();
    }

    // Apply the configuration
    // This will prepare all values for the interrupt handlers.
    applyCfg();

    // enable serial output at 115200 baudrate
    Serial.begin(115200);
    Serial.print("afterglow v");
    Serial.print(AFTERGLOW_VERSION);
    Serial.println(" (c) 2018 morbid cornflakes");
    // check the extended fuse for brown out detection level
    uint8_t efuse = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
    Serial.println("-----------------------------------------------");
    uint8_t bodBits = (efuse & 0x7);
    Serial.print("efuse BOD ");
    Serial.println((bodBits == 0x07) ? "OFF" : (bodBits == 0x04) ? "4.3V" : (bodBits == 0x05) ? "2.7V" : "1.8V");
#ifdef REPLAY_ENABLED
    Serial.print("Replay Table Size: ");
    Serial.println(numReplays());
#endif
#if DEBUG_SERIAL
    Serial.print("CFG from ");
    Serial.print(cfgLoaded ? "EEPROM" : "DEFAULT");
    if (err)
    {
        Serial.print(" err ");
        Serial.print(err);
    }
    Serial.println("");
#endif

    // enable all interrupts
    interrupts();

    // enable a strict 15ms watchdog
    wdt_enable(WDTO_15MS);
}

//------------------------------------------------------------------------------
void start()
{
    // enable the timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

    // enable a strict 15ms watchdog
    wdt_enable(WDTO_15MS);
}

//------------------------------------------------------------------------------
void stop()
{
    // disable the watchdog
    wdt_disable();

    // disable the timer compare interrupt
    TIMSK1 &= ~(1 << OCIE1A);

    // pull OE high to disable all outputs
    PORTC |= B00000010;
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
	}
#endif

    // wait 500ms
    delay(500);
}
