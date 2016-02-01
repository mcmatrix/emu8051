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
 * emu8051.h
 * Emulator core header file
 */

struct em8051;

// Operation: returns number of ticks the operation should take
typedef int (*em8051operation)(struct em8051 *aCPU); 

// Decodes opcode at position, and fills the buffer with the assembler code. 
// Returns how many bytes the opcode takes.
typedef int (*em8051decoder)(struct em8051 *aCPU, int aPosition, char *aBuffer);

// Callback: some exceptional situation occurred. See EM8051_EXCEPTION enum, below
typedef void (*em8051exception)(struct em8051 *aCPU, int aCode);

// Callback: an SFR register is about to be read (not called for 'a' ops nor psw changes)
// Default is to return the value in the SFR register. Ports may act differently.
typedef int (*em8051sfrread)(struct em8051 *aCPU, int aRegister);

// Callback: an SFR register has changed (not called for 'a' ops)
// Default is to do nothing
typedef void (*em8051sfrwrite)(struct em8051 *aCPU, int aRegister);

// Callback: writing to external memory
// Default is to update external memory
// (can be used to control some peripherals)
typedef void (*em8051xwrite)(struct em8051 *aCPU, int aAddress, int aValue);

// Callback: reading from external memory
// Default is to return the value in external memory 
// (can be used to control some peripherals)
typedef int (*em8051xread)(struct em8051 *aCPU, int aAddress);


struct em8051
{
    unsigned char *mCodeMem; // 1k - 64k, must be power of 2
    int mCodeMemSize; 
    unsigned char *mExtData; // 0 - 64k, must be power of 2
    int mExtDataSize;
    unsigned char *mLowerData; // 128 bytes
    unsigned char *mUpperData; // 0 or 128 bytes; leave to NULL if none
    unsigned char *mSFR; // 128 bytes; (special function registers)
    int mPC; // Program Counter; outside memory area
    int mTickDelay; // How many ticks should we delay before continuing
    em8051operation op[256]; // function pointers to opcode handlers
    em8051decoder dec[256]; // opcode-to-string decoder handlers    
    em8051exception except; // callback: exceptional situation occurred
    em8051sfrread sfrread; // callback: SFR register being read
    em8051sfrwrite sfrwrite; // callback: SFR register written
    em8051xread xread; // callback: external memory being read
    em8051xwrite xwrite; // callback: external memory being written

    // Internal values for interrupt services etc.
    int mInterruptActive;
    // Stored register values for interrupts (exception checking)
    int int_a[2];
    int int_psw[2];
    int int_sp[2];
};

// set the emulator into reset state. Must be called before tick(), as
// it also initializes the function pointers. aWipe tells whether to reset
// all memory to zero.
void reset(struct em8051 *aCPU, int aWipe);

// run one emulator tick, or 12 hardware clock cycles.
// returns 1 if a new operation was executed.
int tick(struct em8051 *aCPU);

// decode the next operation as character string.
// buffer must be big enough (64 bytes is very safe). 
// Returns length of opcode.
int decode(struct em8051 *aCPU, int aPosition, unsigned char *aBuffer);

// Load an intel hex format object file. Returns negative for errors.
int load_obj(struct em8051 *aCPU, char *aFilename);

// Alternate way to execute an opcode (switch-structure instead of function pointers)
int do_op(struct em8051 *aCPU);

// Internal: Pushes a value into stack
void push_to_stack(struct em8051 *aCPU, int aValue);

// SAB 80C515/80C535 Special Function Registers
// SFR register locations
enum SFR_REGS
{
    REG_P0  = 0x80 - 0x80, // Port 0
    REG_SP  = 0x81 - 0x80, // Stack Pointer
    REG_DPL = 0x82 - 0x80, // Data Pointer, Low Byte
    REG_DPH = 0x83 - 0x80, // Data Pointer, High Byte
    REG_PCON = 0x87 - 0x80, // Power Control Register
    REG_TCON = 0x88 - 0x80, // Timer Control Register
    REG_TMOD = 0x89 - 0x80, // Timer Mode Register
    REG_TL0 = 0x8A - 0x80, // Timer 0, Low Byte
    REG_TL1 = 0x8B - 0x80, // Timer 1, Low Byte
    REG_TH0 = 0x8C - 0x80, // Timer 0, High Byte
    REG_TH1 = 0x8D - 0x80, // Timer 1, High Byte
    REG_P1  = 0x90 - 0x80, // Port 1
    REG_SCON = 0x98 - 0x80, // Serial Channel Control Reg.
    REG_SBUF = 0x99 - 0x80, // Serial Channel Buffer Reg.
    REG_P2  = 0xA0 - 0x80, // Port 2
    REG_IEN0  = 0xA8 - 0x80, // Interrupt Enable Register 0
    REG_IP0  = 0xA9 - 0x80, // Interrupt Priority Register 0
    REG_P3  = 0xB0 - 0x80, // Port 3
    REG_IEN1  = 0xB8 - 0x80, // Interrupt Enable Register 1
    REG_IP1  = 0xB9 - 0x80, // Interrupt Priority Register 1
    REG_IRCON = 0xC0 - 0x80, // Interrupt Request Control Register
    REG_CCEN = 0xC1 - 0x80, // Comp./Capture Enable Reg
    REG_CCL1 = 0xC2 - 0x80, // Comp./Capture Reg. 1, Low Byte
    REG_CCH1 = 0xC3 - 0x80, // Comp./Capture Reg. 1, High Byte
    REG_CCL2 = 0xC4 - 0x80, // Comp./Capture Reg. 2, Low Byte
    REG_CCH2 = 0xC5 - 0x80, // Comp./Capture Reg. 2, High Byte
    REG_CCL3 = 0xC6 - 0x80, // Comp./Capture Reg. 3, Low Byte
    REG_CCH3 = 0xC7 - 0x80, // Comp./Capture Reg. 3, High Byte
    REG_T2CON = 0xC8 - 0x80, // Timer 2 Control Register
    REG_CRCL = 0xCA - 0x80, // Com./Rel./Capt. Reg. Low Byte
    REG_CRCH = 0xCB - 0x80, // Com./Rel./Capt. Reg. High Byte
    REG_TL2 = 0xCC - 0x80, // Timer 2, Low Byte
    REG_TH2 = 0xCD - 0x80, // Timer 2, High Byte
    REG_PSW = 0xD0 - 0x80, // Program Status Word Register
    REG_ADCON = 0xD8 - 0x80, // A/D Converter Control Register
    REG_ADDAT = 0xD9 - 0x80, // A/D Converter Data Register
    REG_DAPR = 0xDA - 0x80, // D/A Converter Program Register
    REG_P6 = 0xDB - 0x80, // Port 6, Analog/Digital Input
    REG_ACC = 0xE0 - 0x80, // Accumulator
    REG_P4 = 0xE8 - 0x80, // Port 4
    REG_B   = 0xF0 - 0x80, // B-Register
    REG_P5   = 0xF8 - 0x80 // Port 5
};

enum PSW_BITS
{
    PSW_P = 0, // Parity flag. Set/cleared by hardware each instruction cycle to indicate an odd/ even number of "one" bits in the accumulator, i.e. even parity.
    PSW_F1 = 1, // General purpose user flag
    PSW_OV = 2, // Overflow flag
    PSW_RS0 = 3, // Register bank select control bits
    PSW_RS1 = 4, // Register bank select control bits
    PSW_F0 = 5, // General purpose user flag 0
    PSW_AC = 6, // Auxiliary carry flag (for BCD operations)
    PSW_CY = 7 // Carry flag
};

enum PSW_MASKS
{
    PSW_P_MASK = (1<<PSW_P),
    PSW_F1_MASK = (1<<PSW_F1),
    PSW_OV_MASK = (1<<PSW_OV),
    PSW_RS0_MASK = (1<<PSW_RS0),
    PSW_RS1_MASK = (1<<PSW_RS1),
    PSW_F0_MASK = (1<<PSW_F0),
    PSW_AC_MASK = (1<<PSW_AC),
    PSW_CY_MASK = (1<<PSW_CY)
};

enum IEN0_MASKS
{
    IEN0_EX0_MASK = 0x01,
    IEN0_ET0_MASK = 0x02,
    IEN0_EX1_MASK = 0x04,
    IEN0_ET1_MASK = 0x08,
    IEN0_ES_MASK  = 0x10,
    IEN0_ET2_MASK = 0x20,
    IEN0_WDT_MASK = 0x40,
    IEN0_EA_MASK  = 0x80
};

enum IEN1_MASKS
{
    IEN1_EADC_MASK = 0x01,
    IEN1_EX2_MASK = 0x02,
    IEN1_EX3_MASK = 0x04,
    IEN1_EX4_MASK = 0x08,
    IEN1_EX5_MASK = 0x10,
    IEN1_EX6_MASK = 0x20,
    IEN1_SWDT_MASK = 0x40,
    IEN1_EXEN2_MASK = 0x80
};

enum PT_MASKS
{
    PT_PX0_MASK = 0x01,
    PT_PT0_MASK = 0x02,
    PT_PX1_MASK = 0x04,
    PT_PT1_MASK = 0x08,
    PT_PS_MASK  = 0x10,
    PT_PT2_MASK = 0x20,
    PT_UNUSED1_MASK = 0x40,
    PT_UNUSED2_MASK = 0x80
};

enum TCON_MASKS
{
    TCON_IT0_MASK = 0x01,
    TCON_IE0_MASK = 0x02,
    TCON_IT1_MASK = 0x04,
    TCON_IE1_MASK = 0x08,
    TCON_TR0_MASK = 0x10,
    TCON_TF0_MASK = 0x20,
    TCON_TR1_MASK = 0x40,
    TCON_TF1_MASK = 0x80
};

enum T2CON_MASKS
{
    T2CON_T2I0_MASK = 0x01,
    T2CON_T2I1_MASK = 0x02,
    T2CON_T2CM_MASK = 0x04,
    T2CON_T2R0_MASK = 0x08,
    T2CON_T2R1_MASK = 0x10,
    T2CON_I2FR_MASK = 0x20,
    T2CON_I3FR_MASK = 0x40,
    T2CON_T2PS_MASK = 0x80
};

enum IRCON_MASKS
{
    IRCON_IADC_MASK = 0x01,
    IRCON_IEX2_MASK = 0x02,
    IRCON_IEX3_MASK = 0x04,
    IRCON_IEX4_MASK = 0x08,
    IRCON_IEX5_MASK = 0x10,
    IRCON_IEX6_MASK = 0x20,
    IRCON_TF2_MASK = 0x40,
    IRCON_EXF2_MASK = 0x80
};

enum TMOD_MASKS
{
    TMOD_M0_0_MASK = 0x01,
    TMOD_M1_0_MASK = 0x02,
    TMOD_CT_0_MASK = 0x04,
    TMOD_GATE_0_MASK = 0x08,
    TMOD_M0_1_MASK = 0x10,
    TMOD_M1_1_MASK = 0x20,
    TMOD_CT_1_MASK = 0x40,
    TMOD_GATE_1_MASK = 0x80
};

enum IP0_MASKS
{
    IP0_IADC_MASK = 0x01, // A/D converter
    IP0_IEX2_MASK = 0x02, // External interrupt 2
    IP0_IEX3_MASK = 0x04, // External interrupt 3
    IP0_IEX4_MASK = 0x08, // External interrupt 4
    IP0_IEX5_MASK = 0x10, // External interrupt 5
    IP0_IEX6_MASK = 0x20 // External interrupt 6
};

enum IP1_MASKS
{
    IP1_IE0_MASK = 0x01, // External interrupt 0
    IP1_TF0_MASK = 0x02, // Timer overflow
    IP1_IE1_MASK = 0x04, // External interrupt 1
    IP1_TF1_MASK = 0x08, // Timer 1 overflow
    IP1_RI_TI_MASK = 0x10, // Serial channel 
    IP1_TF2_EXF2_MASK = 0x20 // Timer 2 overflow/ext. reload
};

enum EM8051_EXCEPTION
{
    EXCEPTION_STACK,  // stack address > 127 with no upper memory, or roll over
    EXCEPTION_ACC_TO_A, // acc-to-a move operation; illegal (acc-to-acc is ok, a-to-acc is ok..)
    EXCEPTION_IRET_PSW_MISMATCH, // psw not preserved over interrupt call (doesn't care about P, F0 or UNUSED)
    EXCEPTION_IRET_SP_MISMATCH,  // sp not preserved over interrupt call
    EXCEPTION_IRET_ACC_MISMATCH, // acc not preserved over interrupt call
    EXCEPTION_ILLEGAL_OPCODE     // for the single 'reserved' opcode in the architecture
};
