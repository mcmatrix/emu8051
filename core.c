/* 8051 emulator core
 * Copyright 2006 Jari Komppa
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the 
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, subject 
 * to the following conditions: 
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE. 
 *
 * (i.e. the MIT License)
 *
 * core.c
 * General emulation functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emu8051.h"

static void timer_tick(struct em8051 *aCPU)
{
    int increment;
    int v;

    // TODO: External int 0 flag

    if ((aCPU->mSFR[REG_TMOD] & (TMOD_M0_0_MASK | TMOD_M1_0_MASK)) == (TMOD_M0_0_MASK | TMOD_M1_0_MASK))
    {
        // timer/counter 0 in mode 3

        increment = 0;
        
        // Check if we're run enabled
        // TODO: also run if GATE is one and INT is one (external interrupt)
        if (!(aCPU->mSFR[REG_TMOD] & TMOD_GATE_0_MASK) && 
            (aCPU->mSFR[REG_TCON] & TCON_TR0_MASK))
        {
            // check timer / counter mode
            if (aCPU->mSFR[REG_TMOD] & TMOD_CT_0_MASK)
            {
                // counter op;
                // counter works if T0 pin was 1 and is now 0 (P3.4 on AT89C2051)
                increment = 0; // TODO
            }
            else
            {
                increment = 1;
            }
        }
        if (increment)
        {
            v = aCPU->mSFR[REG_TL0];
            v++;
            aCPU->mSFR[REG_TL0] = v & 0xff;
            if (v > 0xff)
            {
                // TL0 overflowed
                aCPU->mSFR[REG_TCON] |= TCON_TF0_MASK;
            }
        }

        increment = 0;
        
        // Check if we're run enabled
        // TODO: also run if GATE is one and INT is one (external interrupt)
        if (!(aCPU->mSFR[REG_TMOD] & TMOD_GATE_1_MASK) && 
            (aCPU->mSFR[REG_TCON] & TCON_TR1_MASK))
        {
            // check timer / counter mode
            if (aCPU->mSFR[REG_TMOD] & TMOD_CT_1_MASK)
            {
                // counter op;
                // counter works if T1 pin was 1 and is now 0
                increment = 0; // TODO
            }
            else
            {
                increment = 1;
            }
        }

        if (increment)
        {
            v = aCPU->mSFR[REG_TH0];
            v++;
            aCPU->mSFR[REG_TH0] = v & 0xff;
            if (v > 0xff)
            {
                // TH0 overflowed
                aCPU->mSFR[REG_TCON] |= TCON_TF1_MASK;
            }
        }

    }

    {   // Timer/counter 0
        
        increment = 0;
        
        // Check if we're run enabled
        // TODO: also run if GATE is one and INT is one (external interrupt)
        if (!(aCPU->mSFR[REG_TMOD] & TMOD_GATE_0_MASK) && 
            (aCPU->mSFR[REG_TCON] & TCON_TR0_MASK))
        {
            // check timer / counter mode
            if (aCPU->mSFR[REG_TMOD] & TMOD_CT_0_MASK)
            {
                // counter op;
                // counter works if T0 pin was 1 and is now 0 (P3.4 on AT89C2051)
                increment = 0; // TODO
            }
            else
            {
                increment = 1;
            }
        }
        
        if (increment)
        {
            switch (aCPU->mSFR[REG_TMOD] & (TMOD_M0_0_MASK | TMOD_M1_0_MASK))
            {
            case 0: // 13-bit timer
                v = aCPU->mSFR[REG_TL0] & 0x1f; // lower 5 bits of TL0
                v++;
                aCPU->mSFR[REG_TL0] = (aCPU->mSFR[REG_TL0] & ~0x1f) | (v & 0x1f);
                if (v > 0x1f)
                {
                    // TL0 overflowed
                    v = aCPU->mSFR[REG_TH0];
                    v++;
                    aCPU->mSFR[REG_TH0] = v & 0xff;
                    if (v > 0xff)
                    {
                        // TH0 overflowed; set bit
                        aCPU->mSFR[REG_TCON] |= TCON_TF0_MASK;
                    }
                }
                break;
            case TMOD_M0_0_MASK: // 16-bit timer/counter
                v = aCPU->mSFR[REG_TL0];
                v++;
                aCPU->mSFR[REG_TL0] = v & 0xff;
                if (v > 0xff)
                {
                    // TL0 overflowed
                    v = aCPU->mSFR[REG_TH0];
                    v++;
                    aCPU->mSFR[REG_TH0] = v & 0xff;
                    if (v > 0xff)
                    {
                        // TH0 overflowed; set bit
                        aCPU->mSFR[REG_TCON] |= TCON_TF0_MASK;
                    }
                }
                break;
            case TMOD_M1_0_MASK: // 8-bit auto-reload timer
                v = aCPU->mSFR[REG_TL0];
                v++;
                aCPU->mSFR[REG_TL0] = v & 0xff;
                if (v > 0xff)
                {
                    // TL0 overflowed; reload
                    aCPU->mSFR[REG_TL0] = aCPU->mSFR[REG_TH0];
                    aCPU->mSFR[REG_TCON] |= TCON_TF0_MASK;
                }
                break;
            default: // two 8-bit timers
                // TODO
                break;
            }
        }
    }

    // TODO: External int 1 

    {   // Timer/counter 1 
        
        increment = 0;

        if (!(aCPU->mSFR[REG_TMOD] & TMOD_GATE_1_MASK) && 
            (aCPU->mSFR[REG_TCON] & TCON_TR1_MASK))
        {
            if (aCPU->mSFR[REG_TMOD] & TMOD_CT_1_MASK)
            {
                // counter op;
                // counter works if T1 pin was 1 and is now 0
                increment = 0; // TODO
            }
            else
            {
                increment = 1;
            }
        }

        if (increment)
        {
            switch (aCPU->mSFR[REG_TMOD] & (TMOD_M0_1_MASK | TMOD_M1_1_MASK))
            {
            case 0: // 13-bit timer
                v = aCPU->mSFR[REG_TL1] & 0x1f; // lower 5 bits of TL0
                v++;
                aCPU->mSFR[REG_TL1] = (aCPU->mSFR[REG_TL1] & ~0x1f) | (v & 0x1f);
                if (v > 0x1f)
                {
                    // TL1 overflowed
                    v = aCPU->mSFR[REG_TH1];
                    v++;
                    aCPU->mSFR[REG_TH1] = v & 0xff;
                    if (v > 0xff)
                    {
                        // TH1 overflowed; set bit
                        // Only update TF1 if timer 0 is not in "mode 3"
                        if (!(aCPU->mSFR[REG_TMOD] & (TMOD_M0_0_MASK | TMOD_M1_0_MASK)))
                            aCPU->mSFR[REG_TCON] |= TCON_TF1_MASK;
                    }
                }
                break;
            case TMOD_M0_1_MASK: // 16-bit timer/counter
                v = aCPU->mSFR[REG_TL1];
                v++;
                aCPU->mSFR[REG_TL1] = v & 0xff;
                if (v > 0xff)
                {
                    // TL1 overflowed
                    v = aCPU->mSFR[REG_TH1];
                    v++;
                    aCPU->mSFR[REG_TH1] = v & 0xff;
                    if (v > 0xff)
                    {
                        // TH1 overflowed; set bit
                        // Only update TF1 if timer 0 is not in "mode 3"
                        if (!(aCPU->mSFR[REG_TMOD] & (TMOD_M0_0_MASK | TMOD_M1_0_MASK)))
                            aCPU->mSFR[REG_TCON] |= TCON_TF1_MASK;
                    }
                }
                break;
            case TMOD_M1_1_MASK: // 8-bit auto-reload timer
                v = aCPU->mSFR[REG_TL1];
                v++;
                aCPU->mSFR[REG_TL1] = v & 0xff;
                if (v > 0xff)
                {
                    // TL0 overflowed; reload
                    aCPU->mSFR[REG_TL1] = aCPU->mSFR[REG_TH1];
                    // Only update TF1 if timer 0 is not in "mode 3"
                    if (!(aCPU->mSFR[REG_TMOD] & (TMOD_M0_0_MASK | TMOD_M1_0_MASK)))
                        aCPU->mSFR[REG_TCON] |= TCON_TF1_MASK;
                }
                break;
            default: // disabled
                break;
            }
        }
    }

    // TODO: serial port, timer2, other stuff
}

void handle_interrupts(struct em8051 *aCPU)
{
    int dest_ip = -1;
    int hi = 0;
    int lo = 0;

    // can't interrupt high level
    if (aCPU->mInterruptActive > 1) 
        return;    

    if (aCPU->mSFR[REG_IEN0] & IEN0_EA_MASK)
    {
        // Interrupts enabled
        if (aCPU->mSFR[REG_IEN0] & IEN0_EX0_MASK && aCPU->mSFR[REG_TCON] & TCON_IE0_MASK)
        {
            // External int 0 
            dest_ip = 0x3;
            if (aCPU->mSFR[REG_IP1] & IP1_IE0_MASK)
                hi = 1;
            lo = 1;
        }
        if (aCPU->mSFR[REG_IEN0] & IEN0_ET0_MASK && aCPU->mSFR[REG_TCON] & TCON_TF0_MASK && !hi)
        {
            // Timer/counter 0 
            if (!lo)
            {
                dest_ip = 0xb;
                lo = 1;
            }
            if (aCPU->mSFR[REG_IP1] & IP1_TF0_MASK)
            {
                hi = 1;
                dest_ip = 0xb;
            }
        }
        if (aCPU->mSFR[REG_IEN0] & IEN0_EX1_MASK && aCPU->mSFR[REG_TCON] & TCON_IE1_MASK && !hi)
        {
            // External int 1 
            if (!lo)
            {
                dest_ip = 0x13;
                lo = 1;
            }
            if (aCPU->mSFR[REG_IP1] & IP1_IE1_MASK)
            {
                hi = 1;
                dest_ip = 0x13;
            }
        }
        if (aCPU->mSFR[REG_IEN0] & IEN0_ET1_MASK && aCPU->mSFR[REG_TCON] & TCON_TF1_MASK && !hi)
        {
            // Timer/counter 1 enabled
            if (!lo)
            {
                dest_ip = 0x1b;
                lo = 1;
            }
            if (aCPU->mSFR[REG_IP1] & IP1_TF1_MASK)
            {
                hi = 1;
                dest_ip = 0x1b;
            }
        }
        if (aCPU->mSFR[REG_IEN0] & IEN0_ES_MASK && !hi)
        {
            // Serial port interrupt 
            if (!lo)
            {
                dest_ip = 0x23;
                lo = 1;
            }
            if (aCPU->mSFR[REG_IP1] & IP1_RI_TI_MASK)
            {
                hi = 1;
                dest_ip = 0x23;
            }
            // TODO
        }
        if (aCPU->mSFR[REG_IEN0] & IEN0_ET2_MASK && !hi)
        {
            // Timer 2 (8052 only)
            if (!lo)
            {
                dest_ip = 0x2b; // guessed
                lo = 1;
            }
            if (aCPU->mSFR[REG_IP1] & IP1_TF2_EXF2_MASK)
            {
                hi = 1;
                dest_ip = 0x2b; // guessed
            }
            // TODO
        }
    }
    
    // no interrupt
    if (dest_ip == -1)
        return;

    // can't interrupt same-level
    if (aCPU->mInterruptActive == 1 && !hi)
        return; 

    // some interrupt occurs; perform LCALL
    push_to_stack(aCPU, aCPU->mPC & 0xff);
    push_to_stack(aCPU, aCPU->mPC >> 8);
    aCPU->mPC = dest_ip;
    // wait for 2 ticks instead of one since we were not executing
    // this LCALL before.
    aCPU->mTickDelay = 2;
    switch (dest_ip)
    {
    case 0xb:
        aCPU->mSFR[REG_TCON] &= ~TCON_TF0_MASK; // clear overflow flag
        break;
    case 0x1b:
        aCPU->mSFR[REG_TCON] &= ~TCON_TF1_MASK; // clear overflow flag
        break;
    }

    if (hi)
    {
        aCPU->mInterruptActive |= 2;
    }
    else
    {
        aCPU->mInterruptActive = 1;
    }
    aCPU->int_a[hi] = aCPU->mSFR[REG_ACC];
    aCPU->int_psw[hi] = aCPU->mSFR[REG_PSW];
    aCPU->int_sp[hi] = aCPU->mSFR[REG_SP];
}

int tick(struct em8051 *aCPU)
{
    int v;
    int ticked = 0;

    if (aCPU->mTickDelay)
    {
        aCPU->mTickDelay--;
    }

    // Interrupts are sent if the following cases are not true:
    // 1. interrupt of equal or higher priority is in progress (tested inside function)
    // 2. current cycle is not the final cycle of instruction (tickdelay = 0)
    // 3. the instruction in progress is RETI or any write to the IE or IP regs (TODO)
    if (aCPU->mTickDelay == 0)
    {
        handle_interrupts(aCPU);
    }

    if (aCPU->mTickDelay == 0)
    {
        aCPU->mTickDelay = aCPU->op[aCPU->mCodeMem[aCPU->mPC & (aCPU->mCodeMemSize - 1)]](aCPU);
        ticked = 1;
        // update parity bit
        v = aCPU->mSFR[REG_ACC];
        v ^= v >> 4;
        v &= 0xf;
        v = (0x6996 >> v) & 1;
        aCPU->mSFR[REG_PSW] = (aCPU->mSFR[REG_PSW] & ~PSW_P_MASK) | (v * PSW_P_MASK);
    }

    timer_tick(aCPU);

    return ticked;
}

int decode(struct em8051 *aCPU, int aPosition, unsigned char *aBuffer)
{
    return aCPU->dec[aCPU->mCodeMem[aPosition & (aCPU->mCodeMemSize - 1)]](aCPU, aPosition, aBuffer);
}

void disasm_setptrs(struct em8051 *aCPU);
void op_setptrs(struct em8051 *aCPU);

void reset(struct em8051 *aCPU, int aWipe)
{
    // clear memory, set registers to bootup values, etc    
    if (aWipe)
    {
        memset(aCPU->mCodeMem, 0, aCPU->mCodeMemSize);
        memset(aCPU->mExtData, 0, aCPU->mExtDataSize);
        memset(aCPU->mLowerData, 0, 128);
        if (aCPU->mUpperData) 
            memset(aCPU->mUpperData, 0, 128);
    }

    memset(aCPU->mSFR, 0, 128);

    aCPU->mPC = 0;
    aCPU->mTickDelay = 0;
    aCPU->mSFR[REG_SP] = 7;
    aCPU->mSFR[REG_P0] = 0xff;
    aCPU->mSFR[REG_P1] = 0xff;
    aCPU->mSFR[REG_P2] = 0xff;
    aCPU->mSFR[REG_P3] = 0xff;
    aCPU->mSFR[REG_P4] = 0xff;
    aCPU->mSFR[REG_P5] = 0xff;

    // build function pointer lists

    disasm_setptrs(aCPU);
    op_setptrs(aCPU);

    // Clean internal variables
    aCPU->mInterruptActive = 0;
}


int readbyte(FILE * f)
{
    char data[3];
    data[0] = fgetc(f);
    data[1] = fgetc(f);
    data[2] = 0;
    return strtol(data, NULL, 16);
}

int load_obj(struct em8051 *aCPU, char *aFilename)
{
    FILE *f;    
    if (aFilename == 0 || aFilename[0] == 0)
        return -1;
    f = fopen(aFilename, "r");
    if (!f) return -1;
    if (fgetc(f) != ':')
    {
    	  fclose(f);
        return -2; // unsupported file format
    }
    while (!feof(f))
    {
        int recordlength;
        int address;
        int recordtype;
        int checksum;
        int i;
        recordlength = readbyte(f);
        address = readbyte(f);
        address <<= 8;
        address |= readbyte(f);
        recordtype = readbyte(f);
        if (recordtype == 1)
            return 0; // we're done
        if (recordtype != 0)
            return -3; // unsupported record type
        checksum = recordtype + recordlength + (address & 0xff) + (address >> 8); // final checksum = 1 + not(checksum)
        for (i = 0; i < recordlength; i++)
        {
            int data = readbyte(f);
            checksum += data;
            aCPU->mCodeMem[address + i] = data;
        }
        i = readbyte(f);
        checksum &= 0xff;
        checksum = 256 - checksum;
        if (i != (checksum & 0xff))
            return -4; // checksum failure
        while (fgetc(f) != ':' && !feof(f)) {} // skip newline        
    }
	  fclose(f);
    return -5;
}

int load_mem(struct em8051 *aCPU, char *aFilename)
{
    FILE *f;    
    int address = 0;
    int i;
    
    if (aFilename == 0 || aFilename[0] == 0)
        return -1;
    f = fopen(aFilename, "rb");
    if (!f) return -1;
    fread(aCPU->mExtData, aCPU->mExtDataSize, 1, f);
    fclose(f);
    return 0; // we're done
}