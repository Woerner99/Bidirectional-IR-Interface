//Final Project Main.c
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
#include "project.h"
#include "tm4c123gh6pm.h"
#include "uart0.h"


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------



int main(void)
 {
   initHw5();
   initHw6();
   initHw7();
   initUart0();
   setUart0BaudRate(115200, 40e6);
   initIR_LED();
   initSpeaker();
   initEeprom();

   USER_DATA user_data;                              // Make an instance of USER_DATA struct
   playStartUpTone();                                // play startup sound when booting up
   //flashEeprom();                                    // clear eeprom on startup

   putsUart0("=======================================================\t\r\n");
   putsUart0("Programmable Bidirectional IR Remote-Control Interface\t\r\n");
   putsUart0("Author: Sean-Michael Woerner\t\r\n");
   putsUart0("=======================================================\t\r\n");
   putsUart0("for more information type 'help'\t\r\n");


   while(true)
   {
       valid2 = false;                               // make false to check for correct commands
       entered = false;                              // make false so that next cmd enter can trigger it
       putsUart0("\t\r\n\n>");                        // Clear line and new line for next cmd
       getsUart0(&user_data);                        // Get string from user
       parseFields(&user_data);                      // Parse the fields from user input



       // Here is where we do whatever we want after getting user input:
       //-----------------------------------------------------------------------------
       // COMMANDS FOR USER
       //-----------------------------------------------------------------------------

       // Toggle alert sounds for when we receive an IR cmd
       // alert good (on/off) = (play/dont't play) sound for good IR cmd
       // alert bad (on/off) = (play/don't play) sound for bad IR cmd
       if((isCommand(&user_data, "alert",2)))
       {
           char *cmd = getFieldString(&user_data, 1);
           if(strCompare(cmd,"good"))
           {
               char *val = getFieldString(&user_data, 2);
               if(strCompare(val,"on"))
               {
                   alert_good = true;
                   putsUart0("ALERT GOOD: ON\t\r\n");
               }
               if(strCompare(val,"off"))
               {
                   alert_good = false;
                   putsUart0("ALERT GOOD: OFF\t\r\n");
               }

           }

           if(strCompare(cmd,"bad"))
           {
               char *val = getFieldString(&user_data, 2);
               if(strCompare(val,"on"))
               {
                   alert_bad = true;
                   putsUart0("ALERT BAD: ON\t\r\n");
               }
               if(strCompare(val,"off"))
               {
                   alert_bad = false;
                   putsUart0("ALERT BAD: OFF\t\r\n");
               }

           }

           valid2 = true;
       }

       // Clear terminal screen
       if(isCommand(&user_data, "clear", 0))
       {
           clearScreen();
           valid2 = true;
        }

       // Step (1), decode add and data of user entered button #
       if(isCommand(&user_data, "decode", 1))
       {
           key = getFieldInteger(&user_data,1);
           if(key>=0 && key<=21)
           {
           putsUart0("Decoding... ");
           putsUart0("\t\r\n");
           playComment(0,buttons[key-1],1);
           valid2 = true;
           }
           else
           {
               putsUart0("Not an available button.\t\r\n");
               playBadTone();
           }
       }

       // Step (4) erase learned cmd
       if (isCommand(&user_data, "erase", 1))
             {
                 char *name = getFieldString(&user_data, 1);
                 eraseName(name);
                 valid2 = true;
             }


       if(isCommand(&user_data, "flash", 0))
       {
           putsUart0("Flashing Eeporm...\t\r\n");
           flashEeprom();
           valid2 = true;
       }

       if(isCommand(&user_data, "help",0))
       {
           putsUart0("Showing list of available terminal commands:\t\r\n");
           putsUart0("--------------------------------------------\t\r\n");
           putsUart0("(1)decode------------Receive IR packet and display Address and Data\t\r\n");
           putsUart0("(2)learn <NAME> <ADD> <DATA>------Receive IR command and store NAME with an associated Address and Data in the EEPROM\t\r\n");
           putsUart0("(3)erase <NAME>------Erases command NAME\t\r\n");
           putsUart0("(4)play <NAME1> - <NAME5>-------Plays a stored command (up to 5 cmd's in one enter)\t\r\n");
           putsUart0("(5)info <NAME>-------Displays the Address and Data of the command NAME\t\r\n");
           putsUart0("(6)list commands-----Lists the stored commands available to transmit\t\r\n");
           putsUart0("(7)alert good|bad on|off --- Turns on/off the control tone for received IR commands\t\r\n");
           putsUart0("(8)rule <NAME> <DEV> <ACTION> --- Creates a rule in EEPROM that performs \t\r\nan action on selected device if a command is received\t\r\n");
           putsUart0("(9)rule <NAME>-------Erases a stored rule\t\r\n");
           putsUart0("(10)list rules--------Lists the stored rules\t\r\n");
           putsUart0("(11)flash------------Completely erases the EEPROM\t\r\n");
           putsUart0("(12)clear------------Clears the terminal window\t\r\n");

           valid2 = true;

       }

       if (isCommand(&user_data, "info", 1))
             {
                 if(user_data.fieldType[1] == 'n')
                 {
                     putsUart0("\ninfo: Integer\t\r\n");
                     uint16_t position = getFieldInteger(&user_data, 1);
                     infoIndex(position);
                     valid2 = true;
                 }
                 if(user_data.fieldType[1] == 'a')
                 {
                     putsUart0("\ninfo: String\t\r\n");
                     char* name = getFieldString(&user_data, 1);
                     infoName(name);
                     valid2 = true;
                 }

             }



       // Step (2), learn (new button to send)
       if(isCommand(&user_data, "learn", 1))
       {
           // ex: [learn CHmin 0 162] or [learn VolUp 0 168]
           if(isCommand(&user_data, "learn", 3))
           {
               char* name = getFieldString(&user_data, 1);
               uint8_t add = getFieldInteger(&user_data, 2);
               uint8_t num = getFieldInteger(&user_data, 3);
               learnInstruction(name, add, num);
           }

           valid2 = true;
           //entered = true;
       }

       // Step (5), show list of available cmds or rules that have been learned
       if(isCommand(&user_data, "list", 0) && !entered)
       {
           char *option = getFieldString(&user_data, 1);

           if(strCompare(option, "commands"))
           {
               putsUart0("Printing list of available commands:\t\r\n");
               listCommands();
               valid2 = true;
           }
           else if(strCompare(option, "rules"))
           {
               putsUart0("Printing list of available rules:\t\r\n");
               valid2 = true;
           }
           else
           {
               putsUart0("type 'list commands|rules' to show either list\t\r\n");
           }



       }


       // Send IR command
       if (isCommand(&user_data, "play", 1))
       {
          uint8_t count = user_data.fieldCount;
          uint8_t num = 1;
          count--;

        for (num; num <= count; num++)
        {
             char * name = getFieldString(&user_data, num);
             uint16_t info = getInfo(name);
             uint8_t _address = info >> 8;
             uint8_t _data = ((info << 8) >> 8);
             putsUart0("-----------------------------------------\t\r\n");
             putsUart0("Playing: ");
             putsUart0(name);
             playComment(_address, _data, 0);
             valid2 = true;
             waitMicrosecond(125000);
         }

       }






      if(!valid2)
      {
          //putsUart0("Invalid command\t\r\n");
      }



      // clean string buffer for next run
      uint8_t i = 0;
         for (i=0; i < MAX_CHARS; i++)
         {
             user_data.buffer[i] = NULL;
         }
         for (i=0; i < MAX_FIELDS; i++)
         {
             user_data.fieldPosition[i] = 0;
             user_data.fieldType[i] = NULL;
         }
         user_data.fieldCount = 0;




   }

   /*
   playComment(0,162);
   waitMicrosecond(1000000);
    */




   while(true);
}
