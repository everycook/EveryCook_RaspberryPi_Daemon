/*
This is the EveryCook Raspberry Pi daemon. It reads inputs from the EveryCook Raspberry Pi shield and controls the outputs.
EveryCook is an open source platform for collecting all data about food and make it available to all kinds of cooking devices.

This program is copyright (C) by EveryCook. Written by Samuel Werder, Peter Turczak and Alexis Wiasmitinow.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

See GPLv3.htm in the main folder for details.
*/
/**
 * @brief      This class make speak everycook  
 * @author     Arthur Bouché
 */
#ifndef SPEAKER_H_
#define SPEAKER_H_

#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "daemon.h"
#include "bool.h"

/** @brief creat a thread to speak
 *  @param * text : text that is spoken
*/
void speakerSpeak(char* text);

#endif /*----SPEAKER_H_-----*/
