/*
  Charliplexing.cpp - Using timer0 with 1ms resolution
  
  Alex Wenger <a.wenger@gmx.de> http://arduinobuch.wordpress.com/
  Matt Mets <mahto@cibomahto.com> http://cibomahto.com/
  
  Timer init code from MsTimer2 - Javier Valencia <javiervalencia80@gmail.com>
  Misc functions from Benjamin Sonnatg <benjamin@sonntag.fr>
  EveryCook ATmega644p optimizing from Samuel Werder <samuel@everycook.org>
  
  History:
    2009-12-30 - V0.0 wrote the first version at 26C3/Berlin
    2010-01-01 - V0.1 adding misc utility functions 
      (Clear, Vertical,  Horizontal) comment are Doxygen complaints now
    2010-05-27 - V0.2 add double-buffer mode
    2010-08-18 - V0.9 Merge brightness and grayscale
	2014-01-26 - V0.91 Changed for use with ATmega644p in EveryCook Project

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <inttypes.h>
#include <math.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include "Charliplexing.h"
#include "mytypes.h"


/* -----------------------------------------------------------------  */
/// Determines whether the display is in single or double buffer mode
uint8_t displayMode = SINGLE_BUFFER;


/** Table for the LED multiplexing cycles
 * Each frame is made of 24 bytes (for the 12 display cycles)
 * There are SHADES-1 frames per buffer in grayscale mode (one for each
 * brightness) and twice that many to support double-buffered grayscale.
 */
typedef struct videoPage {
    uint16_t pixels[12*(SHADES-1)];
}; 

/// Display buffers; only account two if DOUBLE_BUFFER is configured
#ifdef DOUBLE_BUFFER
volatile boolean videoFlipPage = false;
struct videoPage leds[2], *displayBuffer, *workBuffer;
#else
struct videoPage leds;
#define displayBuffer (&leds)
#define workBuffer (&leds)
#endif

/// Pointer inside the buffer that is currently being displayed
uint16_t* displayPointer;


// Timer counts to display each page for, plus off time
typedef struct timerInfo {
    uint8_t counts[SHADES];
    uint8_t prescaler[SHADES];
};

/// Timing buffers (see SetBrightness())
volatile boolean videoFlipTimer = false;
struct timerInfo timer[2], *frontTimer, *backTimer;


// Number of ticks of the prescaled timer per cycle per frame, based on the
// CPU clock speed and the desired frame rate.
#define	TICKS		(F_CPU + 6 * (FRAMERATE << SLOWSCALERSHIFT)) / (12 * (FRAMERATE << SLOWSCALERSHIFT))

// Cutoff below which we need to use a lower prescaler.  This is designed
// so that TICKS is always <128, to avoid arithmetic overflow calculating
// individual "page" times.
// TODO: Technically the 128 cutoff depends on SHADES, FASTSCALERSHIFT,
// and the gamma curve.
#define	CUTOFF(scaler)	((128 * 12 - 6) * FRAMERATE * scaler)

const uint8_t
        fastPrescaler = _BV(CS01),						// 8
        slowPrescaler = _BV(CS01) | _BV(CS00);			// 64
#       define SLOWSCALERSHIFT 7
#       define FASTSCALERSHIFT 2


static boolean initialized = false;


/// Uncomment to set analog pin 5 high during interrupts, so that an
/// oscilloscope can be used to measure the processor time taken by it
#define MEASURE_ISR_TIME
#ifdef MEASURE_ISR_TIME
#include "input.h"
struct pinInfo statusPIN = PD_2;
#endif


/* -----------------------------------------------------------------  */
/** Table for LED Position in leds[] ram table
 */
typedef struct LEDPosition {
    uint8_t high;
    uint8_t cycle;
};

#define	P(pin)	(pin)
#define L(high, low)	{ (P(high) -2), (P(low) - 2) }

const struct LEDPosition PROGMEM ledMap[126] = {
    L(13, 5), L(13, 6), L(13, 7), L(13, 8), L(13, 9), L(13,10), L(13,11), L(13,12),
    L(13, 4), L( 4,13), L(13, 3), L( 3,13), L(13, 2), L( 2,13),
    L(12, 5), L(12, 6), L(12, 7), L(12, 8), L(12, 9), L(12,10), L(12,11), L(12,13),
    L(12, 4), L( 4,12), L(12, 3), L( 3,12), L(12, 2), L( 2,12),
    L(11, 5), L(11, 6), L(11, 7), L(11, 8), L(11, 9), L(11,10), L(11,12), L(11,13),
    L(11, 4), L( 4,11), L(11, 3), L( 3,11), L(11, 2), L( 2,11),
    L(10, 5), L(10, 6), L(10, 7), L(10, 8), L(10, 9), L(10,11), L(10,12), L(10,13),
    L(10, 4), L( 4,10), L(10, 3), L( 3,10), L(10, 2), L( 2,10),
    L( 9, 5), L( 9, 6), L( 9, 7), L( 9, 8), L( 9,10), L( 9,11), L( 9,12), L( 9,13),
    L( 9, 4), L( 4, 9), L( 9, 3), L( 3, 9), L( 9, 2), L( 2, 9),
    L( 8, 5), L( 8, 6), L( 8, 7), L( 8, 9), L( 8,10), L( 8,11), L( 8,12), L( 8,13),
    L( 8, 4), L( 4, 8), L( 8, 3), L( 3, 8), L( 8, 2), L( 2, 8),
    L( 7, 5), L( 7, 6), L( 7, 8), L( 7, 9), L( 7,10), L( 7,11), L( 7,12), L( 7,13),
    L( 7, 4), L( 4, 7), L( 7, 3), L( 3, 7), L( 7, 2), L( 2, 7),
    L( 6, 5), L( 6, 7), L( 6, 8), L( 6, 9), L( 6,10), L( 6,11), L( 6,12), L( 6,13),
    L( 6, 4), L( 4, 6), L( 6, 3), L( 3, 6), L( 6, 2), L( 2, 6),
    L( 5, 6), L( 5, 7), L( 5, 8), L( 5, 9), L( 5,10), L( 5,11), L( 5,12), L( 5,13),
    L( 5, 4), L( 4, 5), L( 5, 3), L( 3, 5), L( 5, 2), L( 2, 5),
};
#undef P(pin)
#undef L(high, low)

/* -----------------------------------------------------------------  */
/** Constructor : Initialize the interrupt code. 
 * should be called in setup();
 */
void LedSign_Init(uint8_t mode)
{
#ifdef MEASURE_ISR_TIME
    pinMode(statusPIN, OUTPUT);
    digitalWrite(statusPIN, LOW);
#endif

    TIMSK0 &= ~(_BV(TOIE0) | _BV(OCIE0A));
    TCCR0A &= ~(_BV(WGM01) | _BV(WGM00));
    TCCR0B &= ~_BV(WGM02);
	
    // Record whether we are in single or double buffer mode
    displayMode = mode;

#ifdef DOUBLE_BUFFER
    videoFlipPage = false;
    // If we are in single buffered mode, point the work buffer
    // at the same physical buffer as the display buffer.  Otherwise,
    // point it at the second physical buffer.
    if (displayMode & DOUBLE_BUFFER)
        workBuffer = &leds[1];
    else
        workBuffer = &leds[0];
    displayBuffer = &leds[0];
#endif

    // Point the display buffer to the first physical buffer
    displayPointer = displayBuffer->pixels;

    // Set up the timer buffering
    videoFlipTimer = false;
    backTimer = &timer[1];
    frontTimer = &timer[0];

    LedSign_SetBrightness(127);

    // Then start the display
    TIMSK0 |= _BV(TOIE0);
    TCCR0B = fastPrescaler;
    TCNT0 = 255;

    initialized = true;
}

#ifdef DOUBLE_BUFFER
/* -----------------------------------------------------------------  */
/** Signal that the front and back buffers should be flipped
 * @param blocking if true : wait for flip before returning, if false :
 *                 return immediately.
 */
void LedSign_Flip(boolean blocking)
{
   // Just set the flip flag, the buffer will flip between redraws
    videoFlipPage = true;

    // If we are blocking, sit here until the page flips.
    if (blocking)
        while (videoFlipPage)
            ;
}
#endif


/* -----------------------------------------------------------------  */
/** Clear the screen completely
 * @param set if 1 : make all led ON, if not set or 0 : make all led OFF
 */
void LedSign_Clear(uint8_t c) {
	uint8_t x=0;
    for (; x<DISPLAY_COLS; x++) {
		uint8_t y=0;
        for (; y<DISPLAY_ROWS; y++) {
            LedSign_Set(x, y, c);
		}
	}
}


/* -----------------------------------------------------------------  */
/** Clear an horizontal line completely
 * @param y is the y coordinate of the line to clear/light [0-8]
 * @param set if 1 : make all led ON, if not set or 0 : make all led OFF
 */
void LedSign_Horizontal(uint8_t y, uint8_t c) {
	uint8_t x=0;
    for (; x<DISPLAY_COLS; x++)  
        LedSign_Set(x, y, c);
}


/* -----------------------------------------------------------------  */
/** Clear a vertical line completely
 * @param x is the x coordinate of the line to clear/light [0-13]
 * @param set if 1 : make all led ON, if not set or 0 : make all led OFF
 */
void LedSign_Vertical(uint8_t x, uint8_t c) {
	uint8_t y=0;
    for (; y<DISPLAY_ROWS; y++)  
        LedSign_Set(x, y, c);
}


/* -----------------------------------------------------------------  */
/** Set : switch on and off the leds. All the position #for char in frameString:
 * calculations are done here, so we don't need to do in the
 * interrupt code
 */
void LedSign_Set(uint8_t x, uint8_t y, uint8_t c)
{
#ifdef GRAYSCALE
    // If we aren't in grayscale mode, just map any pin brightness to max
    if (c > 0 && !(displayMode & GRAYSCALE))
        c = SHADES-1;
#else
    if (c)
        c = SHADES-1;
#endif

    const struct LEDPosition *map = &ledMap[x+y*DISPLAY_COLS];
    uint16_t mask = 1 << pgm_read_byte_near(&map->high);
    uint8_t cycle = pgm_read_byte_near(&map->cycle);

    uint16_t *p = &workBuffer->pixels[cycle*(SHADES-1)];
    uint8_t i;
    for (i = 0; i < c; i++)
	*p++ |= mask;   // ON;
    for (; i < SHADES-1; i++)
	*p++ &= ~mask;   // OFF;
}


/* Set the overall brightness of the screen
 * @param brightness LED brightness, from 0 (off) to 127 (full on)
 */

void LedSign_SetBrightness(uint8_t brightness)
{
    // An exponential fit seems to approximate a (perceived) linear scale
    const unsigned long brightnessPercent = ((unsigned int)brightness * (unsigned int)brightness + 8) >> 4; /*7b*2-4b = 10b*/

    /*   ---- This needs review! Please review. -- thilo  */
    // set up page counts
    // TODO: make SHADES a function parameter. This would require some refactoring.
    uint8_t i;
    const int ticks = TICKS;
    const unsigned long m = (ticks << FASTSCALERSHIFT) * brightnessPercent; /*10b*/
#define	C(x)	((m * (unsigned long)(x * 1024) + (1<<19)) >> 20) /*10b+10b-20b=0b*/
#if SHADES == 2
    const int counts[SHADES] = {
	0.0f,
	C(1.0f),
    };
#elif SHADES == 8
    const int counts[SHADES] = {
	0.0f,
	C(0.030117819624378613658712f),
	C(0.104876339357015456218728f),
	C(0.217591430058779512857041f),
	C(0.365200625214741116475101f),
	C(0.545719579451565749226202f),
	C(0.757697368024318811680598f),
	C(1.0f),
    };
#else
    // NOTE: Changing "scale" invalidates any tables above!
    const float scale = 1.8f;
    int counts[SHADES]; 

    counts[0] = 0.0f;
    for (i=1; i<SHADES; i++)
        counts[i] = C(pow(i / (float)(SHADES - 1), scale));
#endif

    // Wait until the previous brightness request goes through
    while (videoFlipTimer)
        ;

    // Compute on time for each of the pages
    // Use the fast timer; slow timer is only useful for < 3 shades.
    for (i = 0; i < SHADES - 1; i++) {
        int interval = counts[i + 1] - counts[i];
        backTimer->counts[i] = 256 - (interval ? interval : 1);
        backTimer->prescaler[i] = fastPrescaler;
    }

    // Compute off time
    int interval = ticks - (counts[i] >> FASTSCALERSHIFT);
    backTimer->counts[i] = 256 - (interval ? interval : 1);
    backTimer->prescaler[i] = slowPrescaler;

    if (!initialized)
        *frontTimer = *backTimer;

    /*   ---- End of "This needs review! Please review." -- thilo  */

    // Have the ISR update the timer registers next run
    videoFlipTimer = true;
}


/* -----------------------------------------------------------------  */
/** The Interrupt code goes here !  
 */
ISR(TIMER0_OVF_vect) {
#ifdef MEASURE_ISR_TIME
    digitalWrite(statusPIN, HIGH);
#endif

    // For each cycle, we have potential SHADES pages to display.
    // Once every page has been displayed, then we move on to the next
    // cycle.

    // 24 Cycles of Matrix
    static uint8_t cycle = 0;

    // SHADES pages to display
    static uint8_t page = 0;
    TCCR0B = frontTimer->prescaler[page];
    TCNT0 = frontTimer->counts[page];
	
    static uint16_t sink = 0;

    // Set sink pin to Vcc/source, turning off current.
    PINC = sink;
    PINB = (PINB & 0xE0) | (sink >> 8);
    //delayMicroseconds(1);
    // Set pins to input mode; Vcc/source become pullups.
    DDRC = 0;
    DDRB = (DDRB & 0xE0) | 0;

    //sink = 1 << (cycle+2);
	sink = 1 << cycle;
    uint16_t pins = sink;
    if (page < SHADES - 1)
        pins |= *displayPointer++;

    // Enable pullups on new output pins.
    PORTC = pins;
    PORTB = (PORTB & 0xE0) | (pins >> 8);
    //delayMicroseconds(1);
    // Set pins to output mode; pullups become Vcc/source.
    DDRC = pins;
    DDRB = (DDRB & 0xE0) | (pins >> 8);
    // Set sink pin to GND/sink, turning on current.
    PINC = sink;
    PINB = (PINB & 0xE0) | (sink >> 8);

    page++;

    if (page >= SHADES) {
        page = 0;
        cycle++;

        if (cycle >= 12) {
            cycle = 0;

#ifdef DOUBLE_BUFFER
            // If the page should be flipped, do it here.
            if (videoFlipPage)
            {
                videoFlipPage = false;
    
                struct videoPage* temp = displayBuffer;
                displayBuffer = workBuffer;
                workBuffer = temp;
            }
#endif

            if (videoFlipTimer) {
                videoFlipTimer = false;

				struct timerInfo* temp = frontTimer;
                frontTimer = backTimer;
                backTimer = temp;
            }

	    displayPointer = displayBuffer->pixels;
        }
    }
#ifdef MEASURE_ISR_TIME
    digitalWrite(statusPIN, LOW);
#endif
}
