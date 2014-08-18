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

#include "SmallFont.h"
#include <inttypes.h>
#include "FontTypes.h"


C(33) = { P(0,0), P(0,1), P(0,2), P(0,4), END };
C(39) = { P(0,0), P(0,1), END };
C(44) = { P(0,4), END };

C(48) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,4), P(2,4), P(2,3), P(2,2), P(2,1), P(2,0), P(1,0), END };
C(49) = { P(0,1), P(1,0), P(1,1), P(1,2), P(1,3), P(1,4), END };
C(50) = { P(0,0), P(1,0), P(2,0), P(2,1), P(2,2), P(1,2), P(0,2), P(0,3), P(0,4), P(1,4), P(2,4), END };
C(51) = { P(0,0), P(1,0), P(2,0), P(2,1), P(2,2), P(1,2), P(0,2), P(2,3), P(0,4), P(1,4), P(2,4), END };
C(52) = { P(0,0), P(0,1), P(0,2), P(1,2), P(2,0), P(2,1), P(2,2), P(2,3), P(2,4), END };
C(53) = { P(0,0), P(1,0), P(2,0), P(0,1), P(2,2), P(1,2), P(0,2), P(2,3), P(0,4), P(1,4), P(2,4), END };
C(54) = { P(0,0), P(1,0), P(2,0), P(0,1), P(2,2), P(1,2), P(0,2), P(2,3), P(0,4), P(1,4), P(2,4), P(0,3), END };
C(55) = { P(0,0), P(1,0), P(2,0), P(2,1), P(2,2), P(2,3), P(2,4), END };
C(56) = { P(0,1), P(0,0), P(1,0), P(2,0), P(2,1), P(2,2), P(1,2), P(0,2), P(0,3), P(0,4), P(1,4), P(2,4), P(2,3), END };
C(57) = { P(0,1), P(0,0), P(1,0), P(2,0), P(2,1), P(2,2), P(1,2), P(0,2), P(0,4), P(1,4), P(2,4), P(2,3), END };

C(65) = { P(1,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,2), P(2,1), P(2,2), P(2,3), P(2,4), END };
C(66) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,0), P(2,1), P(1,2), P(2,3), P(1,4), END };
C(67) = { P(2,0), P(1,0), P(0,1), P(0,2), P(0,3), P(1,4), P(2,4), END };
C(68) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,0), P(2,1), P(2,2), P(2,3), P(1,4), END };
C(69) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,0), P(2,0), P(1,2), P(2,2), P(1,4), P(2,4), END };
C(70) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,0), P(2,0), P(1,2), P(2,2), END };
C(71) = { P(3,0), P(2,0), P(1,0), P(0,1), P(0,2), P(0,3), P(1,4), P(2,4), P(3,4), P(3,3), P(3,2), P(2,2), END };
C(72) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,2), P(2,0), P(2,1), P(2,2), P(2,3), P(2,4), END };
C(73) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), END };
C(74) = { P(0,3), P(1,4), P(2,4), P(2,3), P(2,2), P(2,1), P(2,0), END };
C(75) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,2), P(2,0), P(2,1), P(2,3), P(2,4), END };
C(76) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,4), P(2,4), END };
C(77) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,1), P(2,2), P(3,1), P(4,0), P(4,1), P(4,2), P(4,3), P(4,4), END };
C(78) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,1), P(2,2), P(2,3), P(3,0), P(3,1), P(3,2), P(3,3), P(3,4), END };
C(79) = { P(1,0), P(0,1), P(0,2), P(0,3), P(1,4), P(2,3), P(2,2), P(2,1), END };
C(80) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,0), P(2,1), P(1,2), END };
C(81) = { P(0,1), P(0,2), P(0,3), P(1,4), P(2,4), P(3,4), P(2,3), P(3,3), P(3,2), P(3,1), P(2,0), P(1,0), END };
C(82) = { P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(1,0), P(2,1), P(1,2), P(2,3), P(2,4), END };
C(83) = { P(2,0), P(1,0), P(0,1), P(1,2), P(2,3), P(1,4), P(0,4), END };
C(84) = { P(0,0), P(1,0), P(2,0), P(1,1), P(1,2), P(1,3), P(1,4), END };
C(85) = { P(0,0), P(0,1), P(0,2), P(0,3), P(1,4), P(2,3), P(2,2), P(2,1), P(2,0), END };
C(86) = { P(0,0), P(0,1), P(0,2), P(1,3), P(1,4), P(2,2), P(2,1), P(2,0), END };
C(87) = { P(0,0), P(0,1), P(0,2), P(1,3), P(1,4), P(2,2), P(2,1), P(2,0), P(3,3), P(3,4), P(4,2), P(4,1), P(4,0), END };
C(88) = { P(0,0), P(0,1), P(1,2), P(0,3), P(0,4), P(2,0), P(2,1), P(2,3), P(2,4), END };
C(89) = { P(0,0), P(0,1), P(1,2), P(1,3), P(1,4), P(2,1), P(2,0), END };
C(90) = { P(0,0), P(1,0), P(2,0), P(2,1), P(1,2), P(0,3), P(0,4), P(1,4), P(2,4), END };

const unsigned char fontMin=33;
const unsigned char fontMax=90;


PROGMEM const uint8_t* const smallFont[fontMax-fontMin+1] = {
  L(33)  /*!*/, 0  /*"*/, 0  /*#*/, 0  /*$*/, 0  /*%*/, 0  /*&*/,
  L(39)  /*'*/, 0  /*(*/, 0  /*)*/, 0  /***/, 0  /*+*/,
  L(44)  /*,*/, 0  /*-*/, 0  /*.*/, 0  /*/*/,  
  L(48)  /*0*/, L(49)  /*1*/, L(50)  /*2*/, L(51)  /*3*/, L(52)  /*4*/,
  L(53)  /*5*/, L(54)  /*6*/, L(55)  /*7*/, L(56)  /*8*/, L(57)  /*9*/,
  0  /*:*/, 0  /*;*/, 0  /*<*/, 0  /*=*/, 0  /*>*/, 0  /*?*/, 0  /*@*/,
  L(65)  /*A*/, L(66)  /*B*/, L(67)  /*C*/, L(68)  /*D*/, L(69)  /*E*/,
  L(70)  /*F*/, L(71)  /*G*/, L(72)  /*H*/, L(73)  /*I*/, L(74)  /*J*/,
  L(75)  /*K*/, L(76)  /*L*/, L(77)  /*M*/, L(78)  /*N*/, L(79)  /*O*/,
  L(80)  /*P*/, L(81)  /*Q*/, L(82)  /*R*/, L(83)  /*S*/, L(84)  /*T*/,
  L(85)  /*U*/, L(86)  /*V*/, L(87)  /*W*/, L(88)  /*X*/, L(89)  /*Y*/,
  L(90)  /*Z*/,

//#ifdef LOWERCASE
//  0  /*[*/, 0  /*\*/, 0  /*]*/, 0  /*^*/, 0  /*_*/, 0  /*`*/,
//  L(97)  /*a*/, L(98)  /*b*/, L(99)  /*c*/, L(100) /*d*/, L(101) /*e*/,
//  L(102) /*f*/, L(103) /*g*/, L(104) /*h*/, L(105) /*i*/, L(106) /*j*/,
//  L(107) /*k*/, L(108) /*l*/, L(109) /*m*/, L(110) /*n*/, L(111) /*o*/,
//  L(112) /*p*/, L(113) /*q*/, L(114) /*r*/, L(115) /*s*/, L(116) /*t*/,
//  L(117) /*u*/, L(118) /*v*/, L(119) /*w*/, L(120) /*x*/, L(121) /*y*/,
//  L(122) /*z*/,
//#endif
};

const uint8_t* SmallFont_getChar(unsigned char letter) {
  const uint8_t* character;
  if (letter==' ') return (uint8_t*) 3;
//  #ifndef LOWERCASE
  if (letter>=97 && letter<=122){
    letter = letter-32;
  }
//  #endif
  if (letter<fontMin || letter>fontMax) {
    return NULL;
  }

  character = (const uint8_t *)pgm_read_word_near(&smallFont[letter-fontMin]);
  return character;
}
