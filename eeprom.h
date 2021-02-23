// EEPROM functions
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    -


#ifndef EEPROM_H_
#define EEPROM_H_

#define NAME_LENGTH 12

enum STATUS
{
    Error = -1, Found = 1, notFound = 255
};

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initEeprom();
void writeEeprom(uint16_t add, uint32_t data);
uint32_t readEeprom(uint16_t add);

#endif
