/*
  Charliplexing.h - Library for controlling the charliplexed led board
  from JimmiePRodgers.com
  Created by Alex Wenger, December 30, 2009.
  Modified by Matt Mets, May 28, 2010.
  Released into the public domain.
*/

#ifndef Charliplexing_h
#define Charliplexing_h

#include <inttypes.h>
#include "mytypes.h"

#define SINGLE_BUFFER 0
#define DOUBLE_BUFFER 1	    // comment out to save memory
#define GRAYSCALE     2	    // comment out to save memory

#define FRAMERATE 80UL      // Desired number of frames per second

#define DISPLAY_COLS 14     // Number of columns in the display
#define DISPLAY_ROWS 9      // Number of rows in the display
#ifdef GRAYSCALE
#define SHADES 8 // Number of distinct shades to display, including black, i.e. OFF
#else
#define SHADES 2
#endif

//namespace LedSign
//{
    extern void LedSign_Init(uint8_t mode/* = SINGLE_BUFFER*/);
    extern void LedSign_Set(uint8_t x, uint8_t y, uint8_t c/* = 1*/);
    extern void LedSign_SetBrightness(uint8_t brightness);
#ifdef DOUBLE_BUFFER
    extern void LedSign_Flip(boolean blocking/* = false*/);
#endif
    extern void LedSign_Clear(uint8_t c/* = 0*/);
    extern void LedSign_Horizontal(uint8_t y, uint8_t c/* = 0*/);
    extern void LedSign_Vertical(uint8_t x, uint8_t c/* = 0*/);
//};

#endif
