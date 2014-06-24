#include <avr/pgmspace.h>

/*
  Font drawing library

  Copyright 2009/2010 Benjamin Sonntag <benjamin@sonntag.fr> http://benjamin.sonntag.fr/

  History:
	2010-01-01 - V0.0 Initial code at Berlin after 26C3
	2014-01-01 - V0.1 add param to set other Font by Samuel Werder
	

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/

#include "FontHandler.h"
#include "Charliplexing.h"
#include <inttypes.h>
#include "FontTypes.h"

/* -----------------------------------------------------------------  */
/** Draws a figure (0-9). You should call it with c=1, 
 * wait a little them call it again with c=0
 * @param figure is the figure [0-9]
 * @param x,y coordinates, 
 * @param c is 1 or 0 to draw or clear it
 */
uint8_t FontHandler_Draw(int x, int y, const uint8_t* character, uint8_t c) {
  uint8_t maxx=0, data;
  if ((uint16_t)character < 0x000F){
    return (uint16_t) character;
  }
  if (character == NULL) return 0;

  //character = (const uint8_t *)pgm_read_word_near(&font[letter-fontMin]);
  data = pgm_read_byte_near(character++);

  while (data != END) {
    uint8_t charCol = data >> 4, charRow = data & 15;
    if (charCol>maxx) maxx=charCol;
    if (
     charCol + x <DISPLAY_COLS &&
     charCol + x >=0 &&
     charRow + y <DISPLAY_ROWS &&
     charRow + y >=0
    ) {
        LedSign_Set(charCol + x, charRow+y, c);
    } 

    data = pgm_read_byte_near(character++);
  }
  return maxx+2;
}


/* -----------------------------------------------------------------  */
/** Draw a figure in the other direction (rotated 90°)
 * @param figure is the figure [0-9]
 * @param x,y coordinates, 
 * @param c is 1 or 0 to draw or clear it
*/
uint8_t FontHandler_Draw90(int x, int y, const uint8_t* character, uint8_t c) {
  uint8_t maxx=0, data;

  if ((uint16_t)character < 0x000F){
    return (uint16_t) character;
  }
  if (character == NULL) return 0;

  //character = (const uint8_t *)pgm_read_word_near(&font[letter-fontMin]);
  data = pgm_read_byte_near(character++);

  while (data != END) {
    uint8_t charCol = data >> 4, charRow = data & 15;
    if (charCol>maxx) maxx=charCol;
    if (
     7 - charRow + x <DISPLAY_COLS &&
     7 - charRow + x >=0 &&
     charCol + y <DISPLAY_ROWS &&
     charCol + y >=0
    ) {
        LedSign_Set(7 - charRow + x, charCol + y, c);
    } 

    data = pgm_read_byte_near(character++);
  }
  return maxx+2;
}
