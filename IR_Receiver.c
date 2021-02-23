//Lab 6
//Sean-Michael Woerner
//1001229459
//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"




// Bitband aliases

// Using Port B pins
#define PORT 0x400053FC

// GPI IR Receiver input pin PB0
#define GPI (*((volatile uint32_t *)(0x42000000 + (PORT-0x40000000)*32 + 0*4)))
#define GPI_MASK 1

// GPO IR Transmitter output pin PB1
#define GPO (*((volatile uint32_t *)(0x42000000 + (PORT-0x40000000)*32 + 1*4)))
#define GPO_MASK 2

// Debug definitons
//#define DEBUG_DATA_SIGNAL
//#define DEBUG_ADD_SIGNAL


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw6()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1; //Enable Timer 1
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; //Enable Port-B Clock
    _delay_cycles(3); //Wait 3 clocks to set

    GPIO_PORTB_DIR_R &= ~GPI_MASK; //Set as Input PB0
    GPIO_PORTB_DEN_R |= GPI_MASK; //Enable Pin PB0
    GPIO_PORTB_DIR_R |= GPO_MASK;  // Set as Output PB1
    GPIO_PORTB_DEN_R |= GPO_MASK;  // Enable Pin PB1

    // Set Falling-Edge Interrupt
    // (edge mode, single edge, falling edge, clear any interrupts, turn on)
    GPIO_PORTB_IS_R &= ~GPI_MASK;
    GPIO_PORTB_IBE_R &= ~GPI_MASK;
    GPIO_PORTB_IEV_R &= ~GPI_MASK; // Make falling edge triggers interrupt
    GPIO_PORTB_ICR_R |= GPI_MASK;
    NVIC_EN0_R |= 1 << (INT_GPIOB-16); // Turn on interrupt 17 (GPIOB)
    GPIO_PORTB_IM_R |= GPI_MASK; // Send to interrupt controller

    // Configure Timer 1
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;                // turn-off interrupts
    //TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer (leave off)
    NVIC_EN0_R |= 1 << (INT_TIMER1A-16);             // turn-on interrupt 37 (TIMER1A)


}

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------




uint32_t samples [] = {0,0,154000, 60000, 74050, 22720};     // table of times to sample (40*us), last value is ~526.5us which
                                                    // will be used until the end of the signal for each iteration after x=5 (x>=6)


uint8_t x = 0;                                      // variable for incrementing through time samples array
uint8_t high_counter = 0;                           // variable to determine if data is 0(01) or 1(0111), when=3, record 1 in data array
uint8_t address[8];                                 // array to keep track of address bits
uint8_t data[8];                                    // array to keep track of data bits (which button on the remote was entered)
uint8_t data_counter = 0;                           // counter to keep track of data index
uint8_t address_counter = 0;                        // counter to keep track of address index

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
void onFallingEdge()
{
    GPIO_PORTB_ICR_R |= GPI_MASK;                    // clears the interrupt
    GPIO_PORTB_IM_R &= ~GPI_MASK;                    // turn off Interrupt
    TIMER1_TAILR_R = 90000;                          // start timer with 2.25 ms (2250us*40)
    TIMER1_IMR_R |= TIMER_IMR_TATOIM;                // turn-on interrupts
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    x = 0;                                           // clear sample index counter
    data_counter = 0;                                // clear DATA array index counter
    address_counter = 0;                             // clear ADD array index counter

}

void timerISR(void) {


    GPO ^= 1;                                        //toggle GPO


    if ( x == 103)                                  // once we get passed index of signal, turn off
   {
       TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;           // turn-off interrupts
       TIMER1_CTL_R &= ~TIMER_CTL_TAEN;             // turn-off timer
       GPIO_PORTB_ICR_R |= GPI_MASK;                // clear edge triggered interrupts
       GPIO_PORTB_IM_R |= GPI_MASK;                 // turn on falling edge interrupts

       // Convert DATA array from binary to integer and display button press to the screen
       getButton();
       /*
       putsUart0("\n\n");
       putcUart0('\t');
       putcUart0('\r');
        */
   }

    if(x >= 2 && x <= 5)
    {
        TIMER1_CTL_R &= ~TIMER_CTL_TAEN;             // Turn timer off
        TIMER1_TAILR_R = (samples[x]);           // Set Load Value from Time Samples array after 3 runs of 90000
        TIMER1_CTL_R |= TIMER_CTL_TAEN;              // turn-on timer
    }

    // For some reason this was messing up recording DATA bits
    /*
    if((x >= 5)&& (x <=52))                          // Record ADD bits
    {
        if((GPI==1))
        {
             high_counter ++;                        // Increment HIGH counter
             if (high_counter == 3)
             {
                  address[address_counter] = 1;             // Record a 1 in data array if there is a 0111 given
                  high_counter = 0;                   // Reset HIGH counter for next data bit
                  address_counter ++;                    // Increment index of DATA array
             }
         }

         if((GPI==0)&&(high_counter==1))
         {
               address[address_counter] = 0;             // Record a 0 in the data array if there is a 01 given
               high_counter = 0;                   // Reset HIGH counter for next data bit
               address_counter ++;                    // Increment index of DATA array
         }

    }
*/

    if((x >= 53)&& (x <=100))                       // Record DATA bits
    {
        if((GPI==1))
        {
            high_counter ++;                        // Increment HIGH counter
            if (high_counter == 3)
            {
                data[data_counter] = 1;             // Record a 1 in data array if there is a 0111 given
                high_counter = 0;                   // Reset HIGH counter for next data bit
                data_counter ++;                    // Increment index of DATA array
            }
        }

        if((GPI==0)&&(high_counter==1))
        {
            data[data_counter] = 0;             // Record a 0 in the data array if there is a 01 given
            high_counter = 0;                   // Reset HIGH counter for next data bit
            data_counter ++;                    // Increment index of DATA array
        }

    }

#ifdef DEBUG_ADD_SIGNAL
    // print ADD and ~ADD
        if(x==0)
        {
            putsUart0("Start Printing ADD and ~ADD: \n");
            putcUart0('\t');
            putcUart0('\r');
        }
        if((x>=5)&&(x<=52))
        {
        putcUart0(GPI+48);
        putcUart0(' ');
        }

#endif

#ifdef DEBUG_DATA_SIGNAL
    // print DATA and ~DATA
    if(x==52)
    {
        putsUart0("\t\r\nStart Printing DATA and ~DATA: \n");
        putcUart0('\t');
        putcUart0('\r');
    }
    if((x>=53)&&(x<=100))
    {
    putcUart0(GPI+48);
    putcUart0(' ');
    }
#endif


   x = x+1;                                        // Increment X for next sample
   TIMER1_ICR_R = TIMER_ICR_TATOCINT;               //clear timer interrupts

}


void getButton() {

    /*
    putsUart0("\n");
    putcUart0('\t');
    putcUart0('\r');
    */

    if (binaryToInteger(address) == 0) {
        //putsUart0("Button: ");
        switch (binaryToInteger(data)) {
        case 162:
            //putsUart0("CH-\t\r\n");
            putsUart0("Address: 0 , Data: 162\t\r\n");
            playGoodTone();
           // writeEeprom(0,162);
            break;
        case 98:
            //putsUart0("CH\t\r\n");
            putsUart0("Address: 0 , Data: 98\t\r\n");
            playGoodTone();
            break;
        case 226:
           // putsUart0("CH+\t\r\n");
            putsUart0("Address: 0 , Data: 226\t\r\n");
            playGoodTone();
            break;
        case 34:
           // putsUart0("PREV\t\r\n");
            putsUart0("Address: 0 , Data: 34\t\r\n");
            playGoodTone();
            break;
        case 2:
           // putsUart0("NEXT\t\r\n");
            putsUart0("Address: 0 , Data: 2\t\r\n");
            playGoodTone();
            break;
        case 194:
          //  putsUart0("PLAY/PAUSE\t\r\n");
            putsUart0("Address: 0 , Data: 194\t\r\n");
            playGoodTone();
            break;
        case 224:
           // putsUart0("VOL-\t\r\n");
            putsUart0("Address: 0 , Data: 224\t\r\n");
            playGoodTone();
            break;
        case 168:
          //  putsUart0("VOL+\t\r\n");
            putsUart0("Address: 0 , Data: 168\t\r\n");
            playGoodTone();
            break;
        case 144:
           // putsUart0("EQ\t\r\n");
            putsUart0("Address: 0 , Data: 144\t\r\n");
            playGoodTone();
            break;
        case 104:
          //  putsUart0("0\t\r\n");
            putsUart0("Address: 0 , Data: 104\t\r\n");
            playGoodTone();
            break;
        case 152:
          //  putsUart0("100+\t\r\n");
            putsUart0("Address: 0 , Data: 152\t\r\n");
            playGoodTone();
            break;
        case 176:
          //  putsUart0("200+\t\r\n");
            putsUart0("Address: 0 , Data: 176\t\r\n");
            playGoodTone();
            break;
        case 48:
          //  putsUart0("1\t\r\n");
            putsUart0("Address: 0 , Data: 48\t\r\n");
            playGoodTone();
            break;
        case 24:
          //  putsUart0("2\t\r\n");
            putsUart0("Address: 0 , Data: 24\t\r\n");
            playGoodTone();
            break;
        case 122:
            //putsUart0("3\t\r\n");
            putsUart0("Address: 0 , Data: 122\t\r\n");
            playGoodTone();
            break;
        case 16:
         //   putsUart0("4\t\r\n");
            putsUart0("Address: 0 , Data: 16\t\r\n");
            playGoodTone();
            break;
        case 56:
           // putsUart0("5\t\r\n");
            putsUart0("Address: 0 , Data: 56\t\r\n");
            playGoodTone();
            break;
        case 90:
          //  putsUart0("6\t\r\n");
            putsUart0("Address: 0 , Data: 90\t\r\n");
            playGoodTone();
            break;
        case 66:
          //  putsUart0("7\t\r\n");
            putsUart0("Address: 0 , Data: 66\t\r\n");
            playGoodTone();
            break;
        case 74:
          //  putsUart0("8\t\r\n");
            putsUart0("Address: 0 , Data: 74\t\r\n");
            playGoodTone();
            break;
        case 82:
          //  putsUart0("9\t\r\n");
            putsUart0("Address: 0 , Data: 82\t\r\n");
            playGoodTone();
            break;
        default:
           // putsUart0("Invalid Button!");
            playBadTone();
            break;
        }
    }

   else
   {
       putsUart0("Invalid Remote!\n");
       playBadTone();
   }

}

uint8_t binaryToInteger(bool in[8]) {
    uint8_t y = 0;
    uint8_t mult = 1;
    uint8_t ans = 0;
    for (y = 0; y < 8; y++) {
        ans += in[7-y]*mult;
        mult *= 2;
    }
    return ans;
}


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
/*
int main(void)
{
    initHw();
    initUart0();
    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    // Display greeting
    putsUart0("IR Receiver Ready...\n");
    putcUart0('\t');
    putcUart0('\r');


    while(true);


}
*/
