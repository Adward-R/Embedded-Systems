//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

// The raspberry pi firmware at the time this was written defaults
// loading at address 0x8000.  Although this bootloader could easily
// load at 0x0000, it loads at 0x8000 so that the same binaries built
// for the SD card work with this bootloader.  Change the ARMBASE
// below to use a different location.

#define ARMBASE 0x8000
#define true 1

#define LOAD    0x00
#define GO      0x01
#define PEEK    0x02
#define POKE    0x03
#define VERIFY  0x04

extern void PUT32 ( unsigned int, unsigned int );
extern void PUT16 ( unsigned int, unsigned int );
extern void PUT8 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
extern unsigned int GET8 ( unsigned int );
extern unsigned int GETPC ( void );
extern void BRANCHTO ( unsigned int );
extern void dummy ( unsigned int );

extern void uart_init ( void );
extern unsigned int uart_lcr ( void );
extern void uart_flush ( void );
extern void uart_send ( unsigned int );
extern unsigned int uart_recv ( void );
extern void hexstring ( unsigned int );
extern void hexstrings ( unsigned int );
extern void timer_init ( void );
extern unsigned int timer_tick ( void );

extern void timer_init ( void );
extern unsigned int timer_tick ( void );

void print_pi(char* s) {
    int i = 0;
    while(s[i] != '\0') {
        uart_send(s[i++]);
    }
    uart_send(0x0D);    //send carriage return
    uart_send(0x0A);    //send new line
}

//------------------------------------------------------------------------
unsigned char xstring[256];
//------------------------------------------------------------------------
int notmain ( void ) {
    unsigned int ra;
    //unsigned int rb;
    unsigned int rx;
    unsigned int addr;
    unsigned int block;
    unsigned int state;

    unsigned int crc;
    unsigned int error_addr;

    uart_init();            //init serial
    hexstring(0x12345678);  //translate to hex value and send
    hexstring(GETPC());
    hexstring(ARMBASE);
    print_pi("This is Raspberry Pi!");
    uart_send(0x04);
    timer_init();

//SOH 0x01
//ACK 0x06
//NAK 0x15
//EOT 0x04

//block numbers start with 1

//132 byte packet
//starts with SOH
//block number byte
//255-block number
//128 bytes of data
//checksum byte (whole packet)
//a single EOT instead of SOH when done, send an ACK on it too
    
    addr=ARMBASE;   //base RAM address
    error_addr = 0; //record the address of error
    block=1;        //the 
    state=0;        //initial state
    crc=0;          //check sum
    rx=timer_tick();

    while(true)
    {
        ra=timer_tick();
        if((ra-rx)>=4000000)
        {
            uart_send(0x15);        //send long time no-response signal
            rx+=4000000;
        }

        if((uart_lcr()&0x01)==0)
        {
            continue;               //test if input string begin with 0x01
        }

        xstring[state]=uart_recv();
        rx=timer_tick();

        switch(state)
        {
            case 0:     //initial state, decide which action to take
            {
                if(xstring[state]==0x01)
                {
                    crc=xstring[state];
                    state++;
                }
                else if (xstring[state] == 0x04)    //End Of transmission
                {                                   //decide the action
                    uart_send(0x06);
                    if (xstring[1] == LOAD)
                    {
                        print_pi("This is LOAD command!");
                        uart_send(0x04);
                        uart_flush();
                    }
                    else if (xstring[1] == VERIFY)
                    {
                        if (error_addr == 0)
                        {
                            print_pi("Verify successful!");
                            uart_send(0x04);
                        }
                        else
                        {
                            print_pi("Verify error");
                            print_pi("Error Adress:");
                            hexstring(error_addr);
                            print_pi("Error Value:");
                            hexstring(GET32(error_addr));
                            uart_send(0x04);
                        }
                        uart_flush();
                    }
                    addr = ARMBASE;
                    error_addr = 0;
                    block = 1;
                    state = 0;
                    crc = 0;
                }
                else
                {
                    state=0;
                    uart_send(0x15);
                    print_pi("Init Error!");
                    uart_send(0x04);
                    uart_flush();
                }
                break;
            }
            case 1:                             //decide the action
            {
                if (xstring[1] > VERIFY)
                {
                    state = 0;
                    uart_send(0x15);
                    print_pi("Command error!");
                    uart_send(0x04);
                    uart_flush();
                }
                else if (xstring[1] == GO)
                {
                    state = 0;
                    uart_send(0x06);
                    print_pi("Branch to the base address!");
                    uart_send(0x04);
                    uart_flush();
                    BRANCHTO(ARMBASE);
                }
                else if (xstring[1] == PEEK)
                {
                    state = 133;
                }
                else if (xstring[1] == POKE)
                {
                    state = 133;
                }
                else
                {
                    state++;
                }
                break;
            }
            case 2:
            {
                if(xstring[state] == block)//if the data has the right block number
                {
                    crc += xstring[state];
                    state++;
                }
                else
                {
                    state = 0;
                    uart_send(0x15);
                    print_pi("Data block error!");
                    uart_send(0x04);
                    uart_flush();
                }
                break;
            }
            
            case 132:   //receive and verify progress
            {
                crc &= 0xFF;
                if(xstring[state]==crc)
                {
                    if (xstring[1] == LOAD)
                    {
                        for(ra=0; ra<128; ra++)
                        {
                            PUT8(addr++,xstring[ra+4]);
                        }
                        uart_send(0x06);
                    }
                    else
                    {
                        //Verify progress
                        for (ra=0; ra<128; ra++,addr++)
                        {
                            if (xstring[ra + 4] != (GET8(addr) & 0xff))
                            {
                                error_addr = addr;  //get the error address
                                break;
                            }
                        }
                        uart_send(0x06);
                    }
                    block=(block+1) & 0xFF; //if the data flow has not stopped
                }
                else
                {
                    uart_send(0x15);
                    print_pi("Check sum error!");
                    uart_send(0x04);
                    uart_flush();
                }
                state=0;
                break;
            }
            case 136:
            {
                if (xstring[1] == PEEK)
                {
                    unsigned int peek_addr = 0;
                    for (ra = 0; ra < 4; ra++)
                    {   //generate the address
                        peek_addr = peek_addr << 8 | xstring[ra + 133];
                    }
                    uart_send(0x06);
                    print_pi("Peek command value:");
                    hexstring(GET32(peek_addr));
                    uart_send(0x04);
                    uart_flush();
                    state = 0;
                }
                else
                {
                    state++;
                }
                break;
            }
            case 140:
            {
                if (xstring[1] == POKE)
                {
                    unsigned int poke_addr = 0x00000000;
                    unsigned int poke_data = 0;
                    for (ra = 0; ra < 4; ra++)
                    {
                        poke_addr = poke_addr << 8 | xstring[ra + 133];
                        poke_data = poke_data << 8 | xstring[ra + 137];
                    }
                    uart_send(0x06);
                    print_pi("Poke command:");
                    PUT32(poke_addr, poke_data);
                    print_pi("Poke address:");
                    hexstring(poke_addr);           //get the Poke address
                    print_pi("Poke value:");
                    hexstring(GET32(poke_addr));   //get the value after edit action
                    uart_send(0x04);
                    uart_flush();
                    state = 0;
                }
                else
                {
                    state = 0;
                }
                break;
            }
            default:
            {
                crc+=xstring[state];
                state++;
                break;
            }
        }
    }
    return(0);
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Copyright (c) 2012 David Welch dwelch@dwelch.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------
