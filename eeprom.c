// EEPROM functions
// Sean-Michael Woerner


//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include <stdbool.h>
#include "eeprom.h"
#include "wait.h"

#define MASK_CLEAR_1  (*((volatile uint32_t*)0x400FD0FC))
#define MASK_CLEAR_2  (*((volatile uint32_t*)0x400AE2C0))
#define MASK_CLEAR_3  (*((volatile uint32_t*)0x400FD0FC))

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
uint8_t currentStatus;

void initEeprom()
{
    SYSCTL_RCGCEEPROM_R = 1;
    _delay_cycles(3);
    while (EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING);
}

void writeEeprom(uint16_t add, uint32_t data)
{
    EEPROM_EEBLOCK_R = add >> 4;
    EEPROM_EEOFFSET_R = add & 0xF;
    EEPROM_EERDWR_R = data;

    while (EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING);
}

uint32_t readEeprom(uint16_t add)
{
    EEPROM_EEBLOCK_R = add >> 4;
    EEPROM_EEOFFSET_R = add & 0xF;
    return EEPROM_EERDWR_R;
}

void flashEeprom()
{
    writeEeprom(0, 0);
}

void learnInstruction(char* name, uint8_t add, uint8_t data)
{
    uint32_t listSize = readEeprom(0);
    uint8_t q;
    uint32_t temp[(NAME_LENGTH/4) + 1];
    uint16_t format = (listSize * ((NAME_LENGTH/4)+1)) + 1;

    putsUart0("Learning Instruction...\t\r\n");

    // Clear Eeprom
    for (q = 0; q < (NAME_LENGTH/4) + 1; q++)
    {
        temp[q] = 0;
    }
    // save name to temp
    for (q = 0; q < (NAME_LENGTH) && name[q] != '\0'; q++)
    {
        temp[q/4] |= name[q] << ((4-1) - (q%4)) * 8;
    }
    // tack on address and data to name
    temp[(NAME_LENGTH)/4] = (add << 8) | data;

    for (q=0; q < (NAME_LENGTH/4) + 1; q++)
    {
        writeEeprom(q + format, temp[q]);
    }

    // get to next spot in list for next entry in the Eeprom
    writeEeprom(0, listSize + 1);
}

// learn a new rule that does a specific function on a desired hardware device
void learnRule(char* name, char* dev, char* action)
{
    uint32_t listSize = readEeprom(0);
    uint8_t q;
    uint32_t temp[(NAME_LENGTH/4) + 1];
    uint16_t format = (listSize * ((NAME_LENGTH/4)+1)) + 1;

    putsUart0("Learning Rule...\t\r\n");

    // Clear Eeprom
    for (q = 0; q < (NAME_LENGTH/4) + 1; q++)
    {
        temp[q] = 0;
    }
    // save name
    for (q = 0; q < (NAME_LENGTH) && name[q] != '\0'; q++)
    {
        temp[q/4] |= name[q] << ((4-1) - (q%4)) * 8;
    }


    for (q = 0; q < (NAME_LENGTH) && dev[q] != '\0'; q++)
    {
        temp[q/4] |= dev[q] << ((4-1) - (q%4)) * 8;
    }

    for (q = 0; q < (NAME_LENGTH) && action[q] != '\0'; q++)
    {
        temp[q/4] |= action[q] << ((4-1) - (q%4)) * 8;
    }



    // get to next spot in list for next entry in the Eeprom
    writeEeprom(0, listSize + 1);


}

void infoIndex(uint16_t index)
{
    uint32_t size = readEeprom(0);
    uint8_t q = 0;
    char name[NAME_LENGTH];
    uint16_t start = (index * ((NAME_LENGTH / 4) + 1)) + 1;
    //NAME_LENGTH/4 + 1 -> 3+1 => 4
    //below should equal 4
    uint32_t temp[(NAME_LENGTH / 4) + 1];
    //clear the temp to zeros..
    for (q = 0; q < (NAME_LENGTH / 4) + 1; q++)
    {
        temp[q] = 0;
    }
    if (index < size)
    {
        for (q = 0; q < (NAME_LENGTH / 4) + 1; q++)
        {
            temp[q] = readEeprom(start + q);
        }
        for (q = 0; q < NAME_LENGTH; q++)
        {
            name[q] = (temp[q / 4] << ((q % 4)) * 8) >> 3 * 8;
        }
        putsUart0("cmd name: ");
        putsUart0(name);
        putsUart0("\t\r\n");

        //now retrieving the address and the data
        uint8_t add = (temp[((NAME_LENGTH / 4) + 1) - 1] << 16) >> 24;
        uint8_t data = (temp[((NAME_LENGTH / 4) + 1) - 1] << 24) >> 24;

        putsUart0("The Address is: ");
        convert2binary(add);
        putsUart0("\t\r\n");
        putsUart0("The Data is: ");
        convert2binary(data);
        putsUart0("\t\r\n");
    }
    else
    {
        putsUart0("Invalid Index\t\r\n");
    }
}


// convert decimal to binary and then print
void convert2binary(uint8_t x)
{
    uint8_t i;
    for (i = 8; i > 0; i--)
    {
        if ((1 << (i - 1)) & x)
        {
            putcUart0('1');
        }
        else
        {
            putcUart0('0');
        }
    }
}

// Find the index of the given "NAME" cmd
uint32_t findIndex(char *name)
{

    //this is the current size
    uint32_t size = readEeprom(0);
    uint16_t position = 0;

    //iterate through the eeprom size
    for (position = 0; position < size; position++)
    {
        //the start -> start
        //start is a variable that is changing through
        //each iteration
        int16_t start = (position * ((NAME_LENGTH / 4) + 1)) + 1;

        uint8_t q = 0;
        //init temp
        uint32_t temp[(NAME_LENGTH / 4) + 1];
        //clear temp
        for (q = 0; q < (NAME_LENGTH / 4) + 1; q++)
        {
            temp[q] = 0;
        }
        bool isMatching = true;
        //start reading from the eeprom
        //and start putting them in each position
        //question (NAME_LENGTH / 4) + 1
        for (q = 0; q < 4; q++)
        {
            temp[q] = readEeprom(start + q);
        }
        //iterate through the string size
        for (q = 0; q < NAME_LENGTH; q++)
        {
            if ((temp[q / 4] << ((q % 4)) * 8) >> 3 * 8 != name[q])
            {
                //they are not the same
                isMatching = false;
            }
            if (name[q] == '\0')
                break;
        }
        if (isMatching == 1)
        {
            currentStatus = Found;
            return position;
        }

    }
    currentStatus = notFound;
    return notFound;
}



// Function to execute when the user types: "info NAME"
void infoName(char *name)
{
    // first it is ideal that we get the index of said name
    currentStatus = notFound;
    uint8_t currentIndex = findIndex(name);
    if (currentStatus == Found)
    {
        infoIndex(currentIndex);
    }
    else if (currentStatus == notFound)
    {
        putsUart0("The name was not found.\t\r\n");
    }
    else
    {
        putsUart0("Error!\t\r\n");
    }

}

void eraseName(char *name)
{
    currentStatus = notFound;
    uint8_t currentIndex = findIndex(name);
    if (currentStatus == Found)
    {
        uint32_t size;
        uint16_t start;
        uint8_t it = 0;
        //read the size
        size = readEeprom(0);
        //find the start position using the index of
        //that name using the function
        start = (currentIndex * ((NAME_LENGTH / 4) + 1)) + 1;

        //iterate and find -> writeEeprom();
        for (it = 0; it < (NAME_LENGTH / 4) + 1; it++)
        {
            writeEeprom(start + it, 0);
        }
        putsUart0("The cmd: ");
        putsUart0(name);
        putsUart0(" has been removed.\t\r\n");
    }
    else if (currentStatus == notFound)
    {
        putsUart0("The cmd was not found.\t\r\n");
    }
    else
    {
        putsUart0("Error!\t\r\n");
    }
}

void listCommands()
{
    uint32_t size;
    uint8_t position = 0;
    uint16_t start;
    uint8_t i = 0;
    uint32_t temp[(NAME_LENGTH / 4) + 1];
    char name[NAME_LENGTH];
    size = readEeprom(0);
    for (position = 0; position < size; position = position + 1)
    {
        start = (position * ((NAME_LENGTH / 4) + 1)) + 1;

        //clear the temp array
        for (i = 0; i < (NAME_LENGTH / 4) + 1; i++)
        {
            temp[i] = 0;
        }
        //this temp holds the extraction of the name

        for (i = 0; i < (NAME_LENGTH / 4) + 1; i++)
        {
            temp[i] = readEeprom(start + i);
        }
        for (i = 0; i < NAME_LENGTH; i++)
        {
            name[i] = (temp[i / 4] << ((i % 4)) * 8) >> 3 * 8;

        }
        if (name[0] != '\0')
        {
            putsUart0("-----------------------------------------\t\r\n");
            putsUart0(name);
            putsUart0("\t\r\n");
            uint8_t _address = (temp[(NAME_LENGTH / 4)] << 16) >> 24;
            uint8_t _data = (temp[(NAME_LENGTH / 4)] << 24) >> 24;
            putsUart0("Address: ");
            convert2binary(_address);
            putsUart0("\t\n\r");
            putsUart0("Data: ");
            convert2binary(_data);
            putsUart0("\t\n\r");
        }

    }


}

uint16_t getInfo(char * name){
    uint8_t pos,st;
    uint32_t tempLength;
    pos = findIndex(name);
    st = (pos * 4);
    tempLength = readEeprom(st + 4);
    return ((tempLength << NAME_LENGTH) >> NAME_LENGTH);

}





