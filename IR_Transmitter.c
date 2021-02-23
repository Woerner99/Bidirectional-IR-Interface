//Lab 7
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
#include <stdint.h>
#include <stdbool.h>

// Bitband aliases
#define PORT_B 0x400053FC
#define PORT_E 0x400243FC

// IR LED pin PE4
#define IR_LED (*((volatile uint32_t *)(0x42000000 + (PORT_E-0x40000000)*32 + 4*4)))
#define IR_LED_MASK 16

// Speaker on pin PB7
#define SPEAKER (*((volatile uint32_t *)(0x42000000 + (PORT_B-0x40000000)*32 + 7*4)))
#define SPEAKER_MASK 128

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

uint8_t i = 0;                                      // indexer for signal output
bool output[100];                                   // high/low array for signal output


// bool's from main to determine whether or not to play sounds when receiving an IR cmd
extern bool alert_good;
extern bool alert_bad;


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initHw7()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;       // enable timer 0
    _delay_cycles(3); //Wait 3 clocks to set

    // Configure Timer 0
    TIMER0_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring, =1
    TIMER0_CFG_R = TIMER_CFG_32_BIT_TIMER;          // configure as 32-bit counter (A + B) -> CFG register, page 728, TIMER1_CFG_R = 0; is the same thing
    TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;         // Periodic mode
    TIMER0_IMR_R &= ~TIMER_IMR_TATOIM;                // turn-off interrupts
    //TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer (leave off)
    NVIC_EN0_R |= 1 << (INT_TIMER0A-16);             // turn-on interrupt 37 (TIMER1A)


}

void initSpeaker()
{
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;                          // enable PWM0 clock
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;                        // enable port B clock
    _delay_cycles(3);                                               //Wait 3 clocks to set

    SYSCTL_SRPWM_R = 0;                                             // leave reset state

    // Speaker (PB7)
    GPIO_PORTB_DIR_R |= SPEAKER_MASK;            // make bit 7 an output
    GPIO_PORTB_DR2R_R |= SPEAKER_MASK;           // set drive strength to 2mA
    GPIO_PORTB_DEN_R |= SPEAKER_MASK;            // enable digital
    GPIO_PORTB_AFSEL_R |= SPEAKER_MASK;          // select auxiliary function
    GPIO_PORTB_PCTL_R &= GPIO_PCTL_PB7_M;        // enable PWM
    GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB7_M0PWM1;   // pin PB7 uses PWM module M0PWM1

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                                // reset PWM0 module
    SYSCTL_SRPWM_R = 0;                                             // leave reset state

    // Configure PWM0 for Speaker
    PWM0_0_CTL_R = 0;                               // turn-off PWM0 generator 0
    PWM0_0_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;  // output 1 on PWM0, gen 0b, cmpb
    PWM0_0_LOAD_R = 10000;                          // Set period to
    PWM0_INVERT_R = PWM_INVERT_PWM1INV;             // invert output so duty cycle increases with increasing compare values
    PWM0_0_CMPB_R = 5000;                           // Speaker on
    PWM0_0_CTL_R = PWM_0_CTL_ENABLE;                // turn-on PWM0 generator 0
    PWM0_ENABLE_R = PWM_ENABLE_PWM1EN;              // enable output

    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;              // disable Speaker until we want to use it



}

void initIR_LED()
{
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;                          // enable PWM1 clock
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;                        // enable port E clock
    _delay_cycles(3);                                               //Wait 3 clocks to set

    SYSCTL_SRPWM_R = 0;                                             // leave reset state

    // IR_LED (PE4)
    GPIO_PORTE_DIR_R |= IR_LED_MASK;                                 // make bit 2 an output
    GPIO_PORTE_DR2R_R |= IR_LED_MASK;                                   // set drive strength to 2mA
    GPIO_PORTE_DEN_R |= IR_LED_MASK;                                   // enable digital
    GPIO_PORTE_AFSEL_R |= IR_LED_MASK;                                // select auxiliary function
    GPIO_PORTE_PCTL_R &= GPIO_PCTL_PE4_M;                                // enable PWM
    GPIO_PORTE_PCTL_R |= GPIO_PCTL_PE4_M1PWM2;                          // pin PE4 uses PWM module M1PWM2

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1;                               // reset PWM0 and PWM1 module
    SYSCTL_SRPWM_R = 0;                                             // leave reset state

    // Configure PWM1 for IR_LED
    PWM1_1_CTL_R = 0;                                            // turn-off PWM1 generator 1
    PWM1_1_GENA_R = PWM_1_GENA_ACTCMPBD_ZERO | PWM_1_GENA_ACTLOAD_ONE;  // output 2 on PWM1, gen 1a, cmpa
    PWM1_1_LOAD_R = 1050;                                                    // set period to 40 MHz sys clock /2/523 = 38.24092 kHz
    PWM1_INVERT_R = PWM_INVERT_PWM2INV;                         // invert output so duty cycle increases with increasing compare values
    PWM1_1_CMPB_R = 524;                                       // IR_LED on
    PWM1_1_CTL_R = PWM_1_CTL_ENABLE;                          // turn-on PWM1 generator 1
    PWM1_ENABLE_R = PWM_ENABLE_PWM2EN;                        // enable output

    GPIO_PORTE_DEN_R &= ~IR_LED_MASK;                          // disable IR_LED until we want to use it

}


void set_Timer(uint32_t us)
{
    TIMER0_CTL_R &= ~TIMER_CTL_TAEN;                // Turn timer off before reconfiguring
    TIMER0_TAILR_R = 40 * us;                       // 40 cycles per microsecond
    TIMER0_IMR_R |= TIMER_IMR_TATOIM;               // turn-on interrupts, = 1
    TIMER0_CTL_R |= TIMER_CTL_TAEN;                 // turn-on timer, = 1
}


//using IR_LED pin PE4
void IR_Interrupt(void)
{


    // First check if index out of bounds, turn off IR_TX
     if (i==100)
     {
         GPIO_PORTE_DEN_R &= ~IR_LED_MASK;           // make PE4 Low
         TIMER0_IMR_R &= ~TIMER_IMR_TATOIM;           // turn-off interrupts
         TIMER0_CTL_R &= ~TIMER_CTL_TAEN;            // Turn off timer

     }

     // We want to first send 9ms HIGH, 4.5ms LOW to match NEC IR format:
     // compared to the IR Receiver, the Transmitter is the inverse so high/low is inverted

     // 9ms HIGH
    if (i==0)
    {
        GPIO_PORTE_DEN_R |= IR_LED_MASK;             // make PE4 High
        set_Timer(9000);                             // load in 9000us (9ms)
        //putcUart0('0');                              // Output 0 on terminal (flip since IR_Reciever will be opposite of PE4)
    }

    // 4.5ms LOW
    if (i==1)
    {
        GPIO_PORTE_DEN_R &= ~IR_LED_MASK;           // make PE4 Low
        set_Timer(4500);                            // load in 4500us (4.5ms)
        //putcUart0('1');                             // Output 1 on terminal
    }

    // after initial 13.5ms, send ADD, ~ADD, DATA, ~DATA:
    if ( i>1 && i<100)
    {
        if (output[i])
        {
            GPIO_PORTE_DEN_R &= ~IR_LED_MASK;           // make PE4 Low
            //putcUart0('1');
        }
        else
        {
            GPIO_PORTE_DEN_R |= IR_LED_MASK;           // make PE4 High
            //putcUart0('0');
        }

        set_Timer(568);                                // set timer to 568us to send signal
    }



    i = i+1;                                         // increment i for next time value
    TIMER0_ICR_R = TIMER_ICR_TATOCINT;               //clear timer interrupts

}


void playComment(uint8_t address, uint8_t data, bool debug)
{
    uint8_t addressArray[8], dataArray[8];
    uint8_t i, test;
    test = address;
        // Convert address and data from integers to array of binary
        // before sending it to sendData:

        //putsUart0("Testing addressArray: ");
        if(debug){putsUart0("Address: ");}
        for (i = 8; i > 0; --i)
        {
            addressArray[8 - i] = (1 << (i - 1)) & test;
            if(debug)
            {

             if(addressArray[8-i]){
             putcUart0('1');
             }else
             putcUart0('0');

            }
        }
        test = data;
        if(debug)
        {

        putcUart0('\t');
        putcUart0('\r');
        putcUart0('\n');
        putsUart0("Data: ");

        }
        //putsUart0("Testing dataArray: ");

        for (i = 8; i > 0; --i)
        {
            dataArray[8 - i] = (1 << (i - 1)) & test;
            if(debug)
            {

             if(dataArray[8-i]){
             putcUart0('1');
             }else
             putcUart0('0');

            }
        }
        putcUart0('\t');
        putcUart0('\r');
        putcUart0('\n');
        sendData(addressArray, dataArray);

}

// takes in the add and data values that were converted into binary arrays
// and then send the bits to be formatted to output either "01" for 0 or "0111" for 1
void sendData(uint8_t address[8], uint8_t data[8])
{

   i = 0;
   output[i++] = 1;
   output[i++] = 0;
   formatBits(address, false);                  // send ADD for formatting
   formatBits(address, true);                   // send ~ADD for formatting
   formatBits(data, false);                     // send DATA for formatting
   formatBits(data, true);                      // send ~DATA for formatting
   output[i++] = 0;
   output[i++] = 1;
   i = 0;                                       // set i (global indexer) back to 0 for IR_interrupt()
   putcUart0('\n');
   IR_Interrupt();
}

// format the received bits so that 0 = "01" and 1 = "0111"
void formatBits(uint8_t byte[8], bool invert)
{
        uint8_t z;                              // Local counter variable 'z'
        //putsUart0("Testing formatBits: \n");
        //putcUart0('\t');
        //putcUart0('\r');
        for (z = 0; z < 8; z++)
        {

            // putcUart0(z+48);
            // putsUart0(": ");
            // if (byte[z])
             //putcUart0('1');
            // else
            // putcUart0('0');
            // putsUart0(": ");

            if (!invert)
            {
                if (byte[z])
                {
                    output[i] = 0;
                    output[i + 1] = 1;
                    output[i + 2] = 1;
                    output[i + 3] = 1;
                    i += 4;
                  //  putsUart0("0111");
                }
                else
                {
                    output[i] = 0; //0
                    output[i + 1] = 1;
                    i += 2;
                  //  putsUart0("01");
                }
            }
            else
            {
                if (!byte[z])
                {
                    output[i] = 0;
                    output[i + 1] = 1;
                    output[i + 2] = 1;
                    output[i + 3] = 1;
                    i += 4;
                   // putsUart0("0111");
                }
                else
                {
                    output[i] = 0; //0
                    output[i + 1] = 1;
                    i += 2;
                   // putsUart0("01");
                }
            }

        }
}



void playGoodTone()
{
    if(alert_good)
    {
    PWM0_0_LOAD_R = 10000;
    PWM0_0_CMPB_R = 5000;

    GPIO_PORTB_DEN_R |= SPEAKER_MASK;

    waitMicrosecond(50000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;

    waitMicrosecond(50000);
    GPIO_PORTB_DEN_R |= SPEAKER_MASK;

    waitMicrosecond(50000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;
    }
}

void playBadTone()
{
    if(alert_bad)
    {
    PWM0_0_LOAD_R = 50000;
    PWM0_0_CMPB_R = 25000;

    GPIO_PORTB_DEN_R |= SPEAKER_MASK;

    waitMicrosecond(50000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;

    waitMicrosecond(50000);
    GPIO_PORTB_DEN_R |= SPEAKER_MASK;

    waitMicrosecond(50000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;
    }

}


void playStartUpTone()
{
    PWM0_0_LOAD_R = 38222;
    PWM0_0_CMPB_R = 19112;
    GPIO_PORTB_DEN_R |= SPEAKER_MASK;
    waitMicrosecond(100000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;

    PWM0_0_LOAD_R = 51020;
    PWM0_0_CMPB_R = 22510;
    GPIO_PORTB_DEN_R |= SPEAKER_MASK;
    waitMicrosecond(100000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;

    PWM0_0_LOAD_R = 38222;
    PWM0_0_CMPB_R = 19112;
    GPIO_PORTB_DEN_R |= SPEAKER_MASK;
    waitMicrosecond(100000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;

    PWM0_0_LOAD_R = 25510;
    PWM0_0_CMPB_R = 12756;
    GPIO_PORTB_DEN_R |= SPEAKER_MASK;
    waitMicrosecond(100000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;

    PWM0_0_LOAD_R = 19112;
    PWM0_0_CMPB_R = 9556;
    GPIO_PORTB_DEN_R |= SPEAKER_MASK;
    waitMicrosecond(100000);
    GPIO_PORTB_DEN_R &= ~SPEAKER_MASK;

}

void test_IR_LED()
{
    GPIO_PORTE_DEN_R |= IR_LED_MASK;           // make PE4 High
    waitMicrosecond(1000000);
    GPIO_PORTE_DEN_R &= ~IR_LED_MASK;           // make PE4 High
    waitMicrosecond(1000000);
    GPIO_PORTE_DEN_R |= IR_LED_MASK;           // make PE4 High
    waitMicrosecond(1000000);
    GPIO_PORTE_DEN_R &= ~IR_LED_MASK;           // make PE4 High


}

/*
int main(void)
{
    initHw7();
    //initHwLab6();
    initUart0();
    setUart0BaudRate(115200, 40e6);
    initIR_LED();
    initSpeaker();

    //test_IR_LED();
    playComment(0,162);

    while(true);

}

*/




