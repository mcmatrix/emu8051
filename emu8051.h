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
    PSWMASK_P = 0x01,
    PSWMASK_F1 = 0x02,
    PSWMASK_OV = 0x04,
    PSWMASK_RS0 = 0x08,
    PSWMASK_RS1 = 0x10,
    PSWMASK_F0 = 0x20,
    PSWMASK_AC = 0x40,
    PSWMASK_CY = 0x80
};

enum IEN0_MASKS
{
    IEN0MASK_EX0 = 0x01,
    IEN0MASK_ET0 = 0x02,
    IEN0MASK_EX1 = 0x04,
    IEN0MASK_ET1 = 0x08,
    IEN0MASK_ES  = 0x10,
    IEN0MASK_ET2 = 0x20,
    IEN0MASK_WDT = 0x40,
    IEN0MASK_EA  = 0x80
};

enum IEN1_MASKS
{
    IEN1MASK_EADC = 0x01,
    IEN1MASK_EX2 = 0x02,
    IEN1MASK_EX3 = 0x04,
    IEN1MASK_EX4 = 0x08,
    IEN1MASK_EX5 = 0x10,
    IEN1MASK_EX6 = 0x20,
    IEN1MASK_SWDT = 0x40,
    IEN1MASK_EXEN2 = 0x80
};

enum PT_MASKS
{
    PTMASK_PX0 = 0x01,
    PTMASK_PT0 = 0x02,
    PTMASK_PX1 = 0x04,
    PTMASK_PT1 = 0x08,
    PTMASK_PS  = 0x10,
    PTMASK_PT2 = 0x20,
    PTMASK_UNUSED1 = 0x40,
    PTMASK_UNUSED2 = 0x80
};

enum TCON_MASKS
{
    TCONMASK_IT0 = 0x01,
    TCONMASK_IE0 = 0x02,
    TCONMASK_IT1 = 0x04,
    TCONMASK_IE1 = 0x08,
    TCONMASK_TR0 = 0x10,
    TCONMASK_TF0 = 0x20,
    TCONMASK_TR1 = 0x40,
    TCONMASK_TF1 = 0x80
};

enum T2CON_MASKS
{
    T2CONMASK_T2I0 = 0x01,
    T2CONMASK_T2I1 = 0x02,
    T2CONMASK_T2CM = 0x04,
    T2CONMASK_T2R0 = 0x08,
    T2CONMASK_T2R1 = 0x10,
    T2CONMASK_I2FR = 0x20,
    T2CONMASK_I3FR = 0x40,
    T2CONMASK_T2PS = 0x80
};

enum IRCON_MASKS
{
    IRCONMASK_IADC = 0x01,
    IRCONMASK_IEX2 = 0x02,
    IRCONMASK_IEX3 = 0x04,
    IRCONMASK_IEX4 = 0x08,
    IRCONMASK_IEX5 = 0x10,
    IRCONMASK_IEX6 = 0x20,
    IRCONMASK_TF2 = 0x40,
    IRCONMASK_EXF2 = 0x80
};

enum TMOD_MASKS
{
    TMODMASK_M0_0 = 0x01,
    TMODMASK_M1_0 = 0x02,
    TMODMASK_CT_0 = 0x04,
    TMODMASK_GATE_0 = 0x08,
    TMODMASK_M0_1 = 0x10,
    TMODMASK_M1_1 = 0x20,
    TMODMASK_CT_1 = 0x40,
    TMODMASK_GATE_1 = 0x80
};

enum IP0_MASKS
{
    IP0MASK_IADC = 0x01,
    IP0MASK_IEX2 = 0x02,
    IP0MASK_IEX3 = 0x04,
    IP0MASK_IEX4 = 0x08,
    IP0MASK_IEX5 = 0x10,
    IP0MASK_IEX6 = 0x20
};

enum IP1_MASKS
{
    IP1MASK_IE0 = 0x01,
    IP1MASK_TF0 = 0x02,
    IP1MASK_IE1 = 0x04,
    IP1MASK_TF1 = 0x08,
    IP1MASK_RI_TI = 0x10,
    IP1MASK_TF2_EXF2 = 0x20
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
