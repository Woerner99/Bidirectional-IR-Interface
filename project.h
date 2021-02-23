#ifndef __PROJECT__H
#define __PROJECT__H

//-----------------------------------------------------------------------------
// Device includes
//-----------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"


//-----------------------------------------------------------------------------
// PINS
//-----------------------------------------------------------------------------


#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define RED_LED_MASK 2

#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))  // debugging
#define GREEN_LED_MASK 8


//-----------------------------------------------------------------------------
// Lab5, Serial Communication defines and funcitons
//-----------------------------------------------------------------------------
#define MAX_CHARS 80
#define MAX_FIELDS 6


typedef struct USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];

} USER_DATA;

// Function that returns true if the entered strings are the same and false otherwise
bool strCompare(char *str1, char *str2)
{
    uint8_t count = 0;
    while (str1[count] != '\0' || str2[count] != '\0')
    {
        if (str1[count] != str2[count])
        {
            return false;
        }
        count++;
    }
    return true;
}

void clearScreen()
{
    uint8_t skip;
    for(skip = 0; skip<75; skip++)
    {
        // skip some lines on the terminal
        putsUart0("\t\r\n");
    }

}


// bool to check if a valid cmd
bool valid2 = false;

// bool to check for commands so we don't interfere and send multiple cmd's on
// one line entry from user. I'm only using this rn because I have interference
bool entered = false;

// bool for sending alert sound on good or bad IR cmd's, default is ON
bool alert_good = true;
bool alert_bad = true;



uint8_t buttons[] = {162,98,226,34,2,194,224,168,144,104,152,176,48,24,122,16,56,
                     90,66,74,82};

uint8_t key;





#endif
