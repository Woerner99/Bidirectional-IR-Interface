/* Lab 5
 * Sean-Michael Woerner
 * CSE 3442 - 001
 * 10/13/2020
 */

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"

#define MAX_CHARS 80
#define MAX_FIELDS 6
#define DEBUG

#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define RED_LED_MASK 2

#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))  // debugging
#define GREEN_LED_MASK 8

// Command checking
bool valid = false;



//-----------------------------------------------------------------------------
// Structs
//-----------------------------------------------------------------------------

typedef struct USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];

} USER_DATA;


//-----------------------------------------------------------------------------
// External methods
//-----------------------------------------------------------------------------

// Blocking function that writes a serial character when the UART buffer is not full
extern void putcUart0(char c);

// Blocking function that writes a string when the UART buffer is not full
extern void putsUart0(char* str);

// Blocking function that returns with serial data once the buffer is not empty
extern char getcUart0();



//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw5()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable clocks
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure LED pins
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK;  // bits 1 and 3 are outputs
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK;  // enable LEDs

}


// Get characters from user interface, processes special chars (like <backspace>), and writes resultant string to buffer
void getsUart0(USER_DATA* data)
{
    int count = 0;
    char c;

    while(1)
    {
        while(1)
        {

        c = getcUart0();     // converts char into ascii value integer
        //GREEN_LED ^= 1;    // LED test to make sure it's taking input


        // convert all upper-case letters to lower-case
        /*
        if( (c >= 65) && (c<=90))
        {
            c = c + 32;
        }
        */

        // is c = to backspace?
        if (c == 127)
        {
            if (count > 0)
            {
                count -- ;          // decrement count if c is = to bkspc and count is greater than 0
            }

            break;                  // go back to get the next char
        }
        else
        {
            // is c = carriage return or line feed?
            if (c == 13)
            {
                putcUart0('\n');
                data->buffer[count] = 0;
                return;
            }
            else
            {
                // is c >= 32 ? (ASCII < 32 values are things we do not want to print, only > 32)
                if (c >= 32)
                {
                    data->buffer[count] = c;
                    count++;

                    // is count equal to max chars?
                    if(count == MAX_CHARS)
                    {
                        return;     // return since buffer is full
                    }

                }
            }
        }




        }
    }



}

 // Begin portion for Lab 5
// Take string buffer from function above and process string in-place and returns info about parsed fields in fieldCount, fieldPosition, and fieldType
void parseFields(USER_DATA* data)
{

    int count = 0;
    int fieldCount = 0;
    char temp;
    int offset_counter = 0;

    while(1)
    {
        temp = data->buffer[count];

        // check if a-z or A-Z, record type and also offset
        if ( (temp >= 65 && temp <= 90) || (temp >= 97 && temp <= 122) )
        {
            data->fieldType[fieldCount] = 'a';
            // get offset of string
            offset_counter = count;
            while(1)
            {
                temp = data->buffer[offset_counter];
                if ( (temp >= 65 && temp <= 90) || (temp >= 97 && temp <= 122) )
                {
                    offset_counter ++;
                }
                else
                {

                    // now we have a delimiter to create null in buffer
                    data->buffer[offset_counter] = NULL;
                    break;
                }
            }
            // record @ what index this string begins
            data->fieldPosition[fieldCount] = count;
            // increment count to get past string entered for next field
            count = offset_counter;
            // increment fieldCount
            fieldCount ++;


        }
        // check if number 1-9, record
        else if ( (temp >= 48 && temp <= 57)  )
        {
            data->fieldType[fieldCount] = 'n';
            offset_counter = count;
                       // check if number is more than 1 digit for offset
                       while(1)
                       {
                           temp = data->buffer[offset_counter];
                           if ( (temp >= 48 && temp <= 57) )
                           {
                               offset_counter ++;
                           }
                           else
                           {
                               // now we have a delimiter to create null in buffer
                               data->buffer[offset_counter] = NULL;
                               break;
                           }
                       }
            // record @ what index the number begins
            data->fieldPosition[fieldCount] = count;
            // increment count to skip past number for next field
            count = offset_counter;
            // increment fieldCount
            fieldCount ++;
        }
        // else (it's a delimiter), record a NULL (maybe record in data.buffer)
        else
        {

            data->buffer[count] = NULL;

            // check if next char is NULL, if it is, return. We've reached end of entry
            if (data->buffer[count+1] == NULL)
            {
                data->fieldCount = fieldCount;
                return;
            }

        }


        // increment count
        count ++;

        if ((fieldCount) == MAX_FIELDS)
        {
            // put local field count into struct
            data->fieldCount = fieldCount;
            return;
        }

    }


}



// Return value of field requested if the field number is in range or NULL otherwise
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    char out[MAX_CHARS + 1];
    uint8_t count;
    for (count = data->fieldPosition[fieldNumber]; data->buffer[count] != '\0';
            count++)
    {
        out[count - data->fieldPosition[fieldNumber]] = data->buffer[count];
    }
    out[count - data->fieldPosition[fieldNumber]] = '\0';
    return out;

}



// Return a pointer to the field requested if the field number is in range and the field type is numeric or 0 otherwise
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    if (data->fieldType[fieldNumber] == 'n')
        {
            uint8_t count;
            int32_t res = 0;
            for (count = data->fieldPosition[fieldNumber];
                    data->buffer[count] != '\0'; count++)
            {
                res = res * 10 + data->buffer[count] - '0';
            }
            return res;
        }
        else
        {
            return 0;
        }

}



// Return True if the command matches the first field and the number of arguments (excluding the command field) is greater than or equal to the requested number of minimum arguments
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    //   command from structure compared to command passed--------field count - initial command >= min arguments passed

    if( (data->buffer[data->fieldPosition[0]] == *strCommand) && ((data->fieldCount)-1 >= minArguments) )
    {
        return true;
    }
    else
    {
        return false;
    }

}



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

/*
int main (void)
 {
    // Init HW
    initHw();
    // Init UART
    initUart0();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    // Create instance of struct
    USER_DATA data;



    while(true)
    {
    // Get the string from the user
    getsUart0(&data);

    // Echo back to the user of the TTY interface for testing
#ifdef DEBUG
    putsUart0(data.buffer);
    putcUart0('\r');
    putcUart0('\n');
#endif

    //Parse Fields
    parseFields(&data);

    // Echo the different fields'content and type of each field
#ifdef DEBUG
    uint8_t i;
    for (i = 0; i < data.fieldCount; i++)
    {
        putcUart0(data.fieldType[i]);
        putcUart0('\t');
        putsUart0(&data.buffer[data.fieldPosition[i]]);
        putcUart0('\r');
        putcUart0('\n');
    }



#endif

    // Command evaluationbool valid = false;
    // set add, data → add and data are integers
    if (isCommand(&data, "set", 2))
    {
    int32_t add = getFieldInteger(&data, 1);
    int32_t data = getFieldInteger(&data, 2);
    valid = true;
    // do something with this information
    //putsUart0("Command is 'set' \n");
    }
    // alert ON|OFF → alert ON or alert OFF are the expected commands
    if (isCommand(&data, "alert", 1))
    {
    char* str = getFieldString(&data, 1);
    valid = true;
    // process the string with your custom strcmp instruction, then do something
    // putsUart0("Alert ON\n");
    }
    // Process other commands here
    // Look for error
    if (!valid)
    {
       // putsUart0("Invalid command\n");
    }



    // clean string buffer for next run
    for (i=0; i < MAX_CHARS; i++)
    {
        data.buffer[i] = NULL;
    }
    for (i=0; i < MAX_FIELDS; i++)
    {
        data.fieldPosition[i] = 0;
        data.fieldType[i] = NULL;
    }
    data.fieldCount = 0;


    }


}
*/


