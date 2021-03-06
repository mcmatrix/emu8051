/* 8051 emulator 
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
 * logicboard.c
 * Logic board view-related stuff for the curses-based emulator front-end
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curses.h"
#include "emu8051.h"
#include "emulator.h"

static int position;
static int logicmode = 0;
static int portmode = 0;
static int oldports[7];
static unsigned char shiftregisters[7*4];
static int audiotick = 0;
static FILE *audioout = NULL;
static FILE *rawout = NULL;

// for the 2x16 character display
static unsigned char chardisplayram[0x80];
static unsigned char chardisplaycgram[0x40];
static int chardisplaycp = 0;
static int chardisplayofs = 0;
static int chardisplaydir = 1;
static int chardisplayshift = 0;
static int chardisplaydcb = 7;
static int chardisplaychargen = 0;
static int chardisplaydata = 0;
static int chardisplay4bmode = 0;
static int chardisplaytick = 0;
static int chardisplaybusy = 0;

static void closeaudio(void)
{
    int len = ftell(audioout);
    fseek(audioout, 4, SEEK_SET);
    len -= 8;
    fwrite(&len,1,4,audioout);
    fseek(audioout, 40, SEEK_SET);
    len -= 32;
    fwrite(&len,1,4,audioout);
    fclose(audioout);
}

static void closeraw(void)
{
    fclose(rawout);
}

void logicboard_tick(struct em8051 *aCPU)
{
    int i;
    if (logicmode == 2)
    {
        for (i = 0; i < 4; i++)
        {
            int clockmask = 2 << (i * 2);
            if ((oldports[0] & clockmask) == 0 && (aCPU->mSFR[REG_P0] & clockmask))
            {
                shiftregisters[i] <<= 1;
                shiftregisters[i] |= (aCPU->mSFR[REG_P0] & (clockmask >> 1)) != 0;
            }
            if ((oldports[1] & clockmask) == 0 && (aCPU->mSFR[REG_P1] & clockmask))
            {
                shiftregisters[i + 4] <<= 1;
                shiftregisters[i + 4] |= (aCPU->mSFR[REG_P1] & (clockmask >> 1)) != 0;
            }
            if ((oldports[2] & clockmask) == 0 && (aCPU->mSFR[REG_P2] & clockmask))
            {
                shiftregisters[i + 8] <<= 1;
                shiftregisters[i + 8] |= (aCPU->mSFR[REG_P2] & (clockmask >> 1)) != 0;
            }
            if ((oldports[3] & clockmask) == 0 && (aCPU->mSFR[REG_P3] & clockmask))
            {
                shiftregisters[i + 12] <<= 1;
                shiftregisters[i + 12] |= (aCPU->mSFR[REG_P3] & (clockmask >> 1)) != 0;
            }
            if ((oldports[4] & clockmask) == 0 && (aCPU->mSFR[REG_P4] & clockmask))
            {
                shiftregisters[i + 16] <<= 1;
                shiftregisters[i + 16] |= (aCPU->mSFR[REG_P4] & (clockmask >> 1)) != 0;
            }
            if ((oldports[5] & clockmask) == 0 && (aCPU->mSFR[REG_P5] & clockmask))
            {
                shiftregisters[i + 20] <<= 1;
                shiftregisters[i + 20] |= (aCPU->mSFR[REG_P5] & (clockmask >> 1)) != 0;
            }
            if ((oldports[6] & clockmask) == 0 && (aCPU->mSFR[REG_P6] & clockmask))
            {
                shiftregisters[i + 24] <<= 1;
                shiftregisters[i + 24] |= (aCPU->mSFR[REG_P6] & (clockmask >> 1)) != 0;
            }
            
        }
    }

	if (logicmode == 3)
	{
		// 44780 -style character display

		if (chardisplaybusy > 0)
			chardisplaybusy--;

		if (((aCPU->mSFR[REG_P4] & 0x01) == 0x01) && ((oldports[4] & 0x04) == 0) && ((aCPU->mSFR[REG_P4] & 0x04) != 0)) 
		{
			// Read op
			// - E level rises from low to high on read ops			


			if (aCPU->mSFR[REG_P4] & 0x02)
			{   // P4.1
				// memory IO mode

				if (!chardisplaybusy)
				{
					// memory IO mode
					if (chardisplaychargen == 0)
					{
						// read from display
						chardisplaydata = chardisplayram[chardisplaycp & 0x7f];
						if (!chardisplay4bmode || chardisplaytick)
						{
							chardisplaycp += chardisplaydir; 
							if (chardisplayshift)
								chardisplayofs += chardisplaydir;
							// busy for 250 microseconds
							chardisplaybusy = 250*opt_clock_hz / 12000000;
						}
					}
					else
					{
						// read from chargen ram
						chardisplaydata = chardisplaycgram[chardisplaycp & 0x3f];
						if (!chardisplay4bmode || chardisplaytick)
						{
							chardisplaycp++; // assumed; not clear from data sheet
							// busy for 250 microseconds
							chardisplaybusy = 250*opt_clock_hz / 12000000;
						}
					}
				}
			}
			else
			{
				// instruction mode				
				chardisplaydata = chardisplaycp & 0x7f;
				if (chardisplaybusy)
					chardisplaydata |= 0x80;
				// doesn't cause busy states
			}

			if (chardisplay4bmode == 0)
			{
				p1out = chardisplaydata;
			}
			else
			{	
				if (chardisplaytick)
					p1out = (chardisplaydata << 4) & 0xf0;
				else
					p1out = (chardisplaydata << 0) & 0xf0;
				chardisplaytick = !chardisplaytick;
			}
		}

		if (((aCPU->mSFR[REG_P4] & 0x01) != 0x01) && 
			 ((oldports[4] & 0x04) != 0) &&
			 ((aCPU->mSFR[REG_P4] & 0x04) == 0))
		{	// P4.2
			// Write op
			// - E level drops from high to low on write ops
			
			if (chardisplay4bmode == 0)
			{
				chardisplaydata = aCPU->mSFR[REG_P5];
			}
			else
			{
				if (!chardisplaytick)
				{
					chardisplaydata = (chardisplaydata & 0xf) | (aCPU->mSFR[REG_P5] & 0xf0);
				}
				else
				{
					chardisplaydata = (chardisplaydata & 0xf0) | ((aCPU->mSFR[REG_P5] & 0xf0) >> 4);
				}
				chardisplaytick = !chardisplaytick;
			}

			if (!chardisplaytick || !chardisplay4bmode)
			{
				if (aCPU->mSFR[REG_P4] & 0x02)
				{ // P4.1
					if (!chardisplaybusy)
					{
						// memory IO mode
						if (chardisplaychargen == 0)
						{
							// write to display
							chardisplayram[chardisplaycp & 0x7f] = chardisplaydata;
							chardisplaycp += chardisplaydir; 
							if (chardisplayshift)
								chardisplayofs += chardisplaydir;
							// busy for 250 microseconds
							chardisplaybusy = 250*opt_clock_hz / 12000000;
						}
						else
						{
							// write to chargen ram
							chardisplaycgram[chardisplaycp & 0x3f] = chardisplaydata;
							chardisplaycp++;  // assumed: not clear from data sheet
							// busy for 250 microseconds
							chardisplaybusy = 250*opt_clock_hz / 12000000;
						}
					}
				}
				else
				{
					// instruction mode				
					if (chardisplaybusy)
					{
						// if busy, only let the user read the busy state.
					}
					else
					if (chardisplaydata == 1)
					{
						// Clear display
						for (i = 0; i < 0x80; i++)
							chardisplayram[i] = 0x20;
						chardisplaycp = 0;
						chardisplayofs = 0;
						chardisplaydir = 1; // based on HD44780U data sheet
						// busy for 2 milliseconds
						chardisplaybusy = 2*opt_clock_hz / 12000;
					}
					else
					if ((chardisplaydata & (0xff & ~1)) == 2)
					{
						// return home
						chardisplaycp = 0;
						chardisplayofs = 0;
						// busy for 200 microseconds
						chardisplaybusy = 200*opt_clock_hz / 12000000;
					}
					else
					if ((chardisplaydata & (0xff & ~3)) == 4)
					{
						// entry mode set.
						if (chardisplaydata & 1)
							chardisplayshift = 1;
						else
							chardisplayshift = 0;
						if (chardisplaydata & 2)
							chardisplaydir = 1;
						else
							chardisplaydir = -1;
						// busy for 200 microseconds
						chardisplaybusy = 200*opt_clock_hz / 12000000;
					}
					else
					if ((chardisplaydata & (0xff & ~7)) == 8)
					{
						// display on/off setting.
						chardisplaydcb = chardisplaydata & 0x7;
						// busy for 200 microseconds
						chardisplaybusy = 200*opt_clock_hz / 12000000;
					}
					else
					if ((chardisplaydata & (0xff & ~0xf)) == 0x10)
					{
						// cursor or display shift.
						if (chardisplaydata & 8)
						{
							// move cursor
							if (chardisplaydata & 4)
								chardisplaycp++;
							else
								chardisplaycp--;
						}
						else
						{
							// shift display
							if (chardisplaydata & 4)
								chardisplayofs++;
							else
								chardisplayofs--;
						}
						// busy for 200 microseconds
						chardisplaybusy = 200*opt_clock_hz / 12000000;
					}
					else
					if ((chardisplaydata & (0xff & ~0x1f)) == 0x20)
					{
						// function set (4/8 bit interface, font size). 
						chardisplay4bmode = (chardisplaydata & 16) == 0;
						chardisplaytick = 0;
						// busy for 200 microseconds
						chardisplaybusy = 200*opt_clock_hz / 12000000;
					}
					else
					if ((chardisplaydata & (0xff & ~0x3f)) == 0x40)
					{
						// character gen address set. 
						chardisplaychargen = 1;
						// busy for 200 microseconds
						chardisplaybusy = 200*opt_clock_hz / 12000000;
					}
					else
					if ((chardisplaydata & (0xff & ~0x7f)) == 0x80)
					{
						// cursor position address set
						chardisplaycp = chardisplaydata & 0x7f;
						chardisplaychargen = 0;
						// busy for 200 microseconds
						chardisplaybusy = 200*opt_clock_hz / 12000000;
					}
				}
			}
		}
		if (chardisplaybusy < 0)
			chardisplaybusy = 0;
	}

    if (logicmode == 4)
    {
        if (audioout == NULL)
        {
            audioout = fopen("audioout.wav", "wb");
            // RIFF signature
            fputc('R', audioout);
            fputc('I', audioout);
            fputc('F', audioout);
            fputc('F', audioout);

            // file length - 8 bytes
            fputc(0, audioout);
            fputc(0, audioout);
            fputc(0, audioout);
            fputc(0, audioout);

            // file type
            fputc('W', audioout);
            fputc('A', audioout);
            fputc('V', audioout);
            fputc('E', audioout);

            // format chunk
            fputc('f', audioout);
            fputc('m', audioout);
            fputc('t', audioout);
            fputc(' ', audioout);

            // format size
            fputc(16, audioout);
            fputc(0, audioout);
            fputc(0, audioout);
            fputc(0, audioout);

            // PCM
            fputc(1, audioout);
            fputc(0, audioout);

            // mono
            fputc(1, audioout);
            fputc(0, audioout);

            // 44khz
            fputc(0x44, audioout);
            fputc(0xAC, audioout);
            fputc(0, audioout);
            fputc(0, audioout);

            // bytes / sec
            fputc(0x44, audioout);
            fputc(0xAC, audioout);
            fputc(0, audioout);
            fputc(0, audioout);

            // block align
            fputc(1, audioout);
            fputc(0, audioout);

            // bits per sample
            fputc(8, audioout);
            fputc(0, audioout);
/*
            // extra format bytes
            fputc(0, audioout);
            fputc(0, audioout);
*/
            // data chunk
            fputc('d', audioout);
            fputc('a', audioout);
            fputc('t', audioout);
            fputc('a', audioout);

            // chunk size
            fputc(0, audioout);
            fputc(0, audioout);
            fputc(0, audioout);
            fputc(0, audioout);

            // ..and we're ready to write data finally.
            audiotick = 0;

            atexit(closeaudio);
        }
        audiotick++;
        if (audiotick > opt_clock_hz / (44100 * 12))
        {
            fputc(aCPU->mSFR[REG_P3] & 0x80, audioout);
            audiotick -= opt_clock_hz / (44100 * 12);
        }
    }

    if (logicmode == 5)
    {
        if (rawout == NULL)
        {
            rawout = fopen("rawout.bin", "wb");
            fputc('B', rawout);
            fputc('I', rawout);
            fputc('N', rawout);
            atexit(closeraw);
        }
        fputc(aCPU->mSFR[REG_P5], rawout);
    }    
    
    oldports[0] = aCPU->mSFR[REG_P0];
    oldports[1] = aCPU->mSFR[REG_P1];
    oldports[2] = aCPU->mSFR[REG_P2];
    oldports[3] = aCPU->mSFR[REG_P3];
    oldports[4] = aCPU->mSFR[REG_P4];
    oldports[5] = aCPU->mSFR[REG_P5];
    oldports[6] = aCPU->mSFR[REG_P6];    
}

static void logicboard_render_7segs(struct em8051 *aCPU)
{
    int input1 = aCPU->mSFR[REG_P0];
    int input2 = aCPU->mSFR[REG_P1];
    int input3 = aCPU->mSFR[REG_P2];
    int input4 = aCPU->mSFR[REG_P3];
    mvprintw(2, 40, " %c   %c   %c   %c ", " -"[(input4 >> 0)&1], " -"[(input3 >> 0)&1], " -"[(input2 >> 0)&1], " -"[(input1 >> 0)&1]);
    mvprintw(3, 40, "%c %c %c %c %c %c %c %c", " |"[(input4 >> 5)&1], " |"[(input4 >> 1)&1], " |"[(input3 >> 5)&1], " |"[(input3 >> 1)&1], " |"[(input2 >> 5)&1], " |"[(input2 >> 1)&1], " |"[(input1 >> 5)&1], " |"[(input1 >> 1)&1]);
    mvprintw(4, 40, " %c   %c   %c   %c ", " -"[(input4 >> 6)&1], " -"[(input3 >> 6)&1], " -"[(input2 >> 6)&1], " -"[(input1 >> 6)&1]);
    mvprintw(5, 40, "%c %c %c %c %c %c %c %c", " |"[(input4 >> 4)&1], " |"[(input4 >> 2)&1], " |"[(input3 >> 4)&1], " |"[(input3 >> 2)&1], " |"[(input2 >> 4)&1], " |"[(input2 >> 2)&1], " |"[(input1 >> 4)&1], " |"[(input1 >> 2)&1]);
    mvprintw(6, 40, " %c%c  %c%c  %c%c  %c%c", " -"[(input4 >> 3)&1], " ."[(input4 >> 7)&1], " -"[(input3 >> 3)&1], " ."[(input3 >> 7)&1], " -"[(input2 >> 3)&1], " ."[(input2 >> 7)&1], " -"[(input1 >> 3)&1], " ."[(input1 >> 7)&1]);
}

static void logicboard_render_registers()
{
    mvprintw(2, 40, "P0.0/1: %02Xh     P3.0/1: %02Xh", shiftregisters[0], shiftregisters[12]);
    mvprintw(3, 40, "P0.2/3: %02Xh     P3.2/3: %02Xh", shiftregisters[1], shiftregisters[13]);
    mvprintw(4, 40, "P0.4/5: %02Xh     P3.4/5: %02Xh", shiftregisters[2], shiftregisters[14]);
    mvprintw(5, 40, "P0.6/7: %02Xh     P3.6/7: %02Xh", shiftregisters[3], shiftregisters[15]);    
    mvprintw(6, 40, "P1.0/1: %02Xh     P4.0/1: %02Xh", shiftregisters[4], shiftregisters[16]);
    mvprintw(7, 40, "P1.2/3: %02Xh     P4.2/3: %02Xh", shiftregisters[5], shiftregisters[17]);
    mvprintw(8, 40, "P1.4/5: %02Xh     P4.4/5: %02Xh", shiftregisters[6], shiftregisters[18]);
    mvprintw(9, 40, "P1.6/7: %02Xh     P4.6/7: %02Xh", shiftregisters[7], shiftregisters[19]);    
    mvprintw(10, 40, "P2.0/1: %02Xh     P5.0/1: %02Xh", shiftregisters[8], shiftregisters[20]);
    mvprintw(11, 40, "P2.2/3: %02Xh     P5.2/3: %02Xh", shiftregisters[9], shiftregisters[21]);
    mvprintw(12, 40, "P2.4/5: %02Xh     P5.4/5: %02Xh", shiftregisters[10], shiftregisters[22]);
    mvprintw(13, 40, "P2.6/7: %02Xh     P5.6/7: %02Xh", shiftregisters[11], shiftregisters[23]);   
}

static void logicboard_render_chardisplay()
{
	int i;	
	mvprintw(2, 40, "[");
	for (i = 0; i < 16; i++)
	{
		int c = chardisplayram[(i + chardisplayofs) & 0x7f];
		if ((chardisplaydcb & 4) == 0) c = ' ';
		if (c == 0) c = ' ';		
		if (c < 32 || c > 126)
			c = '?';
		printw("%c", c);
	}
	printw("]");

	mvprintw(3, 40, "[");
	for (i = 0; i < 16; i++)
	{
		int c = chardisplayram[(i + chardisplayofs + 0x40) & 0x7f];
		if ((chardisplaydcb & 4) == 0) c = ' ';
		if (c == 0) c = ' ';
		if (c < 32 || c > 126)
			c = '?';
		printw("%c", c);
	}
	printw("]");

	
	mvprintw(4, 40, "Display %3s, Cursor %3s", (chardisplaydcb & 4)?"on":"off", (chardisplaydcb & 2)?"on":"off");
	mvprintw(5, 40, "Blinking %3s, 4bit %3s", (chardisplaydcb & 1)?"on":"off", (chardisplay4bmode & 1)?"on":"off");
	mvprintw(6, 40, "4b tick:%d Busy:%-7d", chardisplaytick, chardisplaybusy);

	mvprintw(10, 40, "P5.0-7 = DB0-7");
	mvprintw(11, 40, "P4.7   = EN");
	mvprintw(12, 40, "P4.6   = RS");
	mvprintw(13, 40, "P4.5   = RW");
}

static void logicboard_entermode()
{
	int i;
	for (i = 0; i < 0x80; i++)
		chardisplayram[i] = 0x20;
}

static void logicboard_leavemode()
{
    switch (logicmode)
    {
    case 1:
        mvprintw(2, 40, "                           ");
        mvprintw(3, 40, "               ");
        mvprintw(4, 40, "               ");
        mvprintw(5, 40, "               ");
        mvprintw(6, 40, "               ");
        break;
    case 2:
        mvprintw(2, 40, "                           ");
        mvprintw(3, 40, "                           ");
        mvprintw(4, 40, "                           ");
        mvprintw(5, 40, "                           ");
        mvprintw(6, 40, "                           ");
        mvprintw(7, 40, "                           ");
        mvprintw(8, 40, "                           ");
        mvprintw(9, 40, "                           ");
        mvprintw(10, 40, "                           ");
        mvprintw(11, 40, "                           ");
        mvprintw(12, 40, "                           ");
        mvprintw(13, 40, "                           ");
        break;
	case 3:
		mvprintw(2, 40, "                  ");
		mvprintw(3, 40, "                  ");
		mvprintw(4, 40, "                       ");
		mvprintw(5, 40, "                      ");
		mvprintw(6, 40, "                      ");
		mvprintw(10, 40, "              ");
		mvprintw(11, 40, "           ");
		mvprintw(12, 40, "           ");
		mvprintw(13, 40, "           ");
		break;
    }
}

void wipe_logicboard_view()
{
    logicboard_leavemode();
}

void build_logicboard_view(struct em8051 *aCPU)
{
    erase();
    logicboard_entermode();
}

void logicboard_editor_keys(struct em8051 *aCPU, int ch)
{
    int xorvalue = -1;
    switch (ch)
    {
    case KEY_RIGHT:
        if (position == 6)
        {
            logicboard_leavemode();
            logicmode++;
            if (logicmode > 5) logicmode = 5;
            logicboard_entermode();
        }
        break;
    case KEY_LEFT:
        if (position == 6)
        {
            logicboard_leavemode();
            logicmode--;
            if (logicmode < 0) logicmode = 0;
            logicboard_entermode();
        }
        break;
    case KEY_DOWN:
        position++;
        if (position > 6) position = 6;
        break;
    case KEY_UP:
        position--;
        if (position < 0) position = 0;
        break;
    case '1':
        xorvalue = 7;
        break;
    case '2':
        xorvalue = 6;
        break;
    case '3':
        xorvalue = 5;
        break;
    case '4':
        xorvalue = 4;
        break;
    case '5':
        xorvalue = 3;
        break;
    case '6':
        xorvalue = 2;
        break;
    case '7':
        xorvalue = 1;
        break;
    case '8':
        xorvalue = 0;
        break;
    }
    if (xorvalue != -1)
    {
        switch (position)
        {
        case 0:
            p0out ^= 1 << xorvalue;
            break;
        case 1:
            p1out ^= 1 << xorvalue;
            break;
        case 2:
            p2out ^= 1 << xorvalue;
            break;
        case 3:
            p3out ^= 1 << xorvalue;
            break;
        case 4:
            p4out ^= 1 << xorvalue;
            break;
        case 5:
            p5out ^= 1 << xorvalue;
            break;        
        }
    }
}

void logicboard_update(struct em8051 *aCPU)
{
    char ledstate[]="_*";
    char swstate[]="01";
    int data;
    mvprintw( 1, 1, "Logic board view");

    mvprintw( 3, 5, "1 2 3 4 5 6 7 8");
    data = aCPU->mSFR[REG_P0];
    mvprintw( 4, 2, "P0 %c %c %c %c %c %c %c %c",
        ledstate[(data>>7)&1],
        ledstate[(data>>6)&1],
        ledstate[(data>>5)&1],
        ledstate[(data>>4)&1],
        ledstate[(data>>3)&1],
        ledstate[(data>>2)&1],
        ledstate[(data>>1)&1],
        ledstate[(data>>0)&1]);

    data = p0out;
    mvprintw( 5, 2, "   %c %c %c %c %c %c %c %c",
        swstate[(data>>7)&1],
        swstate[(data>>6)&1],
        swstate[(data>>5)&1],
        swstate[(data>>4)&1],
        swstate[(data>>3)&1],
        swstate[(data>>2)&1],
        swstate[(data>>1)&1],
        swstate[(data>>0)&1]);

    data = aCPU->mSFR[REG_P1];
    mvprintw( 7, 2, "P1 %c %c %c %c %c %c %c %c",
        ledstate[(data>>7)&1],
        ledstate[(data>>6)&1],
        ledstate[(data>>5)&1],
        ledstate[(data>>4)&1],
        ledstate[(data>>3)&1],
        ledstate[(data>>2)&1],
        ledstate[(data>>1)&1],
        ledstate[(data>>0)&1]);

    data = p1out;
    mvprintw( 8, 2, "   %c %c %c %c %c %c %c %c",
        swstate[(data>>7)&1],
        swstate[(data>>6)&1],
        swstate[(data>>5)&1],
        swstate[(data>>4)&1],
        swstate[(data>>3)&1],
        swstate[(data>>2)&1],
        swstate[(data>>1)&1],
        swstate[(data>>0)&1]);

    data = aCPU->mSFR[REG_P2];
    mvprintw(10, 2, "P2 %c %c %c %c %c %c %c %c",
        ledstate[(data>>7)&1],
        ledstate[(data>>6)&1],
        ledstate[(data>>5)&1],
        ledstate[(data>>4)&1],
        ledstate[(data>>3)&1],
        ledstate[(data>>2)&1],
        ledstate[(data>>1)&1],
        ledstate[(data>>0)&1]);

    data = p2out;
    mvprintw(11, 2, "   %c %c %c %c %c %c %c %c",
        swstate[(data>>7)&1],
        swstate[(data>>6)&1],
        swstate[(data>>5)&1],
        swstate[(data>>4)&1],
        swstate[(data>>3)&1],
        swstate[(data>>2)&1],
        swstate[(data>>1)&1],
        swstate[(data>>0)&1]);

    data = aCPU->mSFR[REG_P3];
    mvprintw(13, 2, "P3 %c %c %c %c %c %c %c %c",
        ledstate[(data>>7)&1],
        ledstate[(data>>6)&1],
        ledstate[(data>>5)&1],
        ledstate[(data>>4)&1],
        ledstate[(data>>3)&1],
        ledstate[(data>>2)&1],
        ledstate[(data>>1)&1],
        ledstate[(data>>0)&1]);

    data = p3out;
    mvprintw(14, 2, "   %c %c %c %c %c %c %c %c",
        swstate[(data>>7)&1],
        swstate[(data>>6)&1],
        swstate[(data>>5)&1],
        swstate[(data>>4)&1],
        swstate[(data>>3)&1],
        swstate[(data>>2)&1],
        swstate[(data>>1)&1],
        swstate[(data>>0)&1]);
        
    data = aCPU->mSFR[REG_P4];
    mvprintw(16, 2, "P4 %c %c %c %c %c %c %c %c",
        ledstate[(data>>7)&1],
        ledstate[(data>>6)&1],
        ledstate[(data>>5)&1],
        ledstate[(data>>4)&1],
        ledstate[(data>>3)&1],
        ledstate[(data>>2)&1],
        ledstate[(data>>1)&1],
        ledstate[(data>>0)&1]);

    data = p4out;
    mvprintw(17, 2, "   %c %c %c %c %c %c %c %c",
        swstate[(data>>7)&1],
        swstate[(data>>6)&1],
        swstate[(data>>5)&1],
        swstate[(data>>4)&1],
        swstate[(data>>3)&1],
        swstate[(data>>2)&1],
        swstate[(data>>1)&1],
        swstate[(data>>0)&1]);

    data = aCPU->mSFR[REG_P5];
    mvprintw(19, 2, "P5 %c %c %c %c %c %c %c %c",
        ledstate[(data>>7)&1],
        ledstate[(data>>6)&1],
        ledstate[(data>>5)&1],
        ledstate[(data>>4)&1],
        ledstate[(data>>3)&1],
        ledstate[(data>>2)&1],
        ledstate[(data>>1)&1],
        ledstate[(data>>0)&1]);

    data = p5out;
    mvprintw(20, 2, "   %c %c %c %c %c %c %c %c",
        swstate[(data>>7)&1],
        swstate[(data>>6)&1],
        swstate[(data>>5)&1],
        swstate[(data>>4)&1],
        swstate[(data>>3)&1],
        swstate[(data>>2)&1],
        swstate[(data>>1)&1],
        swstate[(data>>0)&1]);
        

    mvprintw(23, 2, "  ");

    attron(A_REVERSE);
    switch (logicmode)
    {
    case 0:
        mvprintw(23, 4, "< No additional hw     >");
        break;
    case 1:
        mvprintw(23, 4, "< 7-seg displays       >");
        break;
    case 2:
        mvprintw(23, 4, "< 8bit shift registers >");
        break;
	case 3:
		mvprintw(23, 4, "< 16x2 44780 display   >");
		break;
	case 4:
        mvprintw(23, 4, "< 1bit audio out (P3.7)>");
        break;
	case 5:
        mvprintw(23, 4, "< debug raw output (P5)>");
        break;        
    }
    attroff(A_REVERSE);

    mvprintw(position*3+5,2,"->");

    switch (logicmode)
    {
    case 1:
        logicboard_render_7segs(aCPU);
        break;
    case 2:
        logicboard_render_registers();
        break;
	case 3:
		logicboard_render_chardisplay();
		break;
    }

    refresh();
}
