//=========================================================================
//
// Module: coco3_512k_ram_board_test.ino
//
// Date: 13 August 2026
//
// Author: Pete Cervasio <cervasio@airmail.net>
//
// Copyright 2026, Pete Cervasio.  All rights reserved.
//
// Description: Coco 512k Static RAM Board Tester.  This is only for the
// 6809.org.uk board described on https://www.6809.org.uk/dragon/ because
// I just foofed around with RAS and CAS timing until the static RAM gave
// me good data.  It's probably WAY off from what would be optimal, and
// surely not good for a genuine TRS-80 Coco 512k board.  I'd love to be
// proven wrong on that, though.
//
// The 128 long uint32_t array used as a PRNG buffer gives 256 writes
// in each row, which should effect a refresh cycle, so once RAS/CAS/WE
// timing is fixed up this should work for the Tandy 512k upgrade board
// or another that uses dynamic RAM chips.  The inline asm xorshift32
// fills that array in less than 1 msec, so there should be plenty of
// time to read or write the row after generation.
//
// Usage: Connect as appropriate, then run this sketch.  In the list
// below, A0 through A15 are the analog pins (treated as their digital
// ports in the code).  A0-7 are PORTF, A8-15 are PORTK.  I have added
// the port A and C bits this uses, in case you're using something that
// has a different pinout from a regular Mega2560.  I have used this
// with both a real Mega2560 and a Robotdyn Mega2560 Pro Embed.  The RAM
// boards were ones I ordered from JLCPCB from the gerbers created in
// Kicad from the 6809.org.uk project.
//
// RAM          Mega 2560          RAM            Mega 2560
// ---------    ----------         ----------     ----------
//  1 - GND      GND                1 - GND       GND
//  2 - +5V      +5v                2 - /RAS      36 (PC1)
//  3 - D9       A9                 3 - Z0        22 (PA0)
//  4 - D8       A8                 4 - Z1        23 (PA1)
//  5 - D10      A10                5 - Z2        24 (PA2)
//  6 - D11      A11                6 - Z3        25 (PA3)
//  7 - D14      A14                7 - Z6        28 (PA6)
//  8 - D15      A15                8 - Z5        27 (PA5)
//  9 - D13      A13                9 - Z4        26 (PA4)
// 10 - D12      A12               10 - Z7        29 (PA7)
// 11 - /WE1     34 (PC3)          11 - Z8        30 (PC7)
// 12 - GND      GND               12 - GND       GND
// 13 - GND      GND
// 14 - D2       A2
// 15 - D3       A3
// 16 - D1       A1
// 17 - /WE0     35 (PC2)
// 18 - D0       A0
// 19 - /CAS     37 (PC0)
// 20 - D7       A7
// 21 - D5       A5
// 22 - D4       A4
// 23 - D6       A6
// 24 - GND      GND
//
//=========================================================================

#include <stdint.h>

#define SETB(port, b)  port |= _BV(b)
#define CLRB(port, b)  port &= ~_BV(b)
#define ISSET(port, b) ((port & _BV(b)) != 0)

// Bits for the 4 RAM control signals
#define nCE  0
#define nRAS 1
#define nWE0 2
#define nWE1 3

// This is set up to give us 256 bytes to write per port
// which should handle refreshing the row.
#define PRNG_DATA_SIZE 128
const uint32_t col_size = PRNG_DATA_SIZE * 2;
const uint32_t row_count = 262144 / col_size;
typedef union {
    uint32_t u32[PRNG_DATA_SIZE]; // 128 qwords
    uint16_t u16[col_size];       // 256 words
    uint8_t   u8[col_size * 2];   // 512 bytes
} prng_bytes;
prng_bytes prng_data;

// Truthful base prng seed (modified in setup())
uint32_t prng_state = 0xDecafBad;

//===============================================================
// Marsaglia xorshift32 in inline assembler.  About 4 x faster
// than the C version.  See the internal comments for the C.  I
// also contributed this to another project, so you might see it
// elsewhere.
//---------------------------------------------------------------
inline uint32_t prng(uint32_t x)
{
    uint32_t temp = prng_state;
    asm volatile(
        // ==========================================
        // 1. p ^= p << 13
        // ==========================================
        "movw %A1, %A0    \n\t"
        "movw %C1, %C0    \n\t"
        // Byte shift left by 8 bits
        "mov %D1, %C1     \n\t"
        "mov %C1, %B1     \n\t"
        "mov %B1, %A1     \n\t"
        "clr %A1          \n\t"
        // Unrolled 5-bit left shift (No counter loop!)
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        // XOR result
        "eor %A0, %A1     \n\t"
        "eor %B0, %B1     \n\t"
        "eor %C0, %C1     \n\t"
        "eor %D0, %D1     \n\t"

        // ==========================================
        // 2. p ^= p >> 17
        // ==========================================
        "movw %A1, %A0    \n\t"
        "movw %C1, %C0    \n\t"
        // Byte shift right by 16 bits
        "mov %A1, %C1     \n\t"
        "mov %B1, %D1     \n\t"
        "clr %C1          \n\t"
        "clr %D1          \n\t"
        // 1-bit right shift
        "lsr %B1          \n\t"
        "ror %A1          \n\t"
        // XOR result
        "eor %A0, %A1     \n\t"
        "eor %B0, %B1     \n\t"
        "eor %C0, %C1     \n\t"
        "eor %D0, %D1     \n\t"

        // ==========================================
        // 3. p ^= p << 5
        // ==========================================
        "movw %A1, %A0    \n\t"
        "movw %C1, %C0    \n\t"
        // Unrolled 5-bit left shift
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        "lsl %A1 \n\t rol %B1 \n\t rol %C1 \n\t rol %D1 \n\t"
        // XOR result
        "eor %A0, %A1     \n\t"
        "eor %B0, %B1     \n\t"
        "eor %C0, %C1     \n\t"
        "eor %D0, %D1     \n\t"

        : "+r" (x), "=&r" (temp)
        :
        : // No clobbers needed anymore! r26 is free.
    );
    prng_state = x;
    return x;
}

//===============================================================
// Fill up our row buffer with a hunk of random bits.
//---------------------------------------------------------------
void fill_prng_data()
{ 
  for (int i = 0; i < PRNG_DATA_SIZE; i++) {
    prng_data.u32[i] = prng(prng_state);
  }        
} 


//===============================================================
// Get this party ready to start
//---------------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  // Sign on.
  Serial.println("\n-------------------------------------");
  Serial.println("Coco Static RAM tester\n"); 
  Serial.flush();
  delay(100);

  // Read all the analog inputs and do mishmash garbage with them.
  uint32_t foo = abs(analogRead(A0))  + (abs(analogRead(A1)) << 1) + abs(analogRead(A2))  + (abs(analogRead(A3)) << 1)
               + abs(analogRead(A4))  + (abs(analogRead(A5)) << 1) + abs(analogRead(A6))  + (abs(analogRead(A7)) << 1)
               + abs(analogRead(A8))  + (abs(analogRead(A9)) << 1) + abs(analogRead(A10)) + (abs(analogRead(A11))<< 1)
               + abs(analogRead(A12)) + (abs(analogRead(A13))<< 1) + abs(analogRead(A14)) + (abs(analogRead(A15))<< 1);

  Serial.print ("Random start addition is 0x"); Serial.println(foo, HEX);
  Serial.flush();
  
  // Add in our bit of random noise.  Only doing this so that the
  // memory test doesn't start with the same sequence every time.
  prng_state += foo;

  // Also, grab the low byte of that thing, and frob the PRNG that many
  // times, just for good measure.
  uint8_t bar = foo & 0xff; 
  if (bar == 0) bar = 42;
  while(bar) {
    foo = prng(prng_state);
    bar--;
  }

  // Pin functions:

  // PA0..7 and PC7 = address generator (RAM A0..8/9..17 multiplexed)
  // PC0..3 = RAM control (CAS, RAS, WE0, WE1)
  // PF0..7 = Data byte low
  // PK0..7 = Data byte high

  // Pin direction: 0 = in/pullup, 1 = out
  DDRA     = 0xff;  DDRC     = 0x8f;
  PORTA    = 0;     PORTC    = 0x0f;

  DDRF     = 0xff;  DDRK     =  0xff; 
  PORTF    = 0x00;  PORTK    = 0x00;

  // Ensure that our analogRead() shenanigans above didn't mess with 
  // the digital input buffers.  Turn them back on.

  DIDR0 = 0x00; // Enables digital input buffers on PORTF (A0 - A7)
  DIDR2 = 0x00; // Enables digital input buffers on PORTK (A8 - A15)

  // Timer1 runs at 2 ticks/usec and does not require interrupts.

  TCCR1A = 0;
  TCCR1B = _BV(CS11);     // Prescaler 8: 0.5 usec/tick at 16 MHz
  TIMSK1 = 0;
  TIFR1 = _BV(TOV1);

  // And one full pass of filling the PRNG data buffer just because.
  uint8_t sreg = start_timer1();
  fill_prng_data();
  uint32_t elapsed = finish_timer1(sreg);

  // Just a data point for the serial observer
  Serial.print("Time to generate one row of PRNG data: ");
  Serial.print(elapsed);
  Serial.println(" usec");

  Serial.flush();
  delay(500);

}

uint8_t start_timer1()
{
  uint8_t sreg = SREG;
  TIFR1 = _BV(TOV1);
  TCNT1 = 0;
  return sreg;
}

uint32_t finish_timer1(uint8_t sreg)
{
  uint32_t ticks = TCNT1;
  if (TIFR1 & _BV(TOV1))
    ticks += 65536UL;
  SREG = sreg;
  return ticks / 2; // 0.5 usec/tick
}



//===============================================================
// Send the address out via the latch and /RAS signal.  This is
// highly ugly so don't look at it.
//---------------------------------------------------------------
static void inline set_address(uint32_t addr)
{
  uint8_t la, ha, lc, hc; // low and high byte, low and high address

  // Do the address bytes
  la = addr & 0xff;         // low address byte (0-7)
  lc = (addr & 0x100) != 0; // bit 8
  addr = (addr >> 9);
  ha = addr & 0xff;         // high address byte (9-16)
  hc = (addr & 0x100) != 0; // bit 17

  // Set high bits, then toggle nRAS
  PORTA = ha;
  if (hc) { SETB(PORTC, 7);}  else { CLRB(PORTC, 7); }
  CLRB (PORTC, nRAS);   // RAS low
  asm("nop");
  SETB (PORTC, nRAS);   // RAS high
  // Then set low bits
  PORTA = la;
  if (lc) { SETB(PORTC, 7); } else { CLRB(PORTC, 7); }
  asm("nop");

}


//===============================================================
// Write a 16 bit hunk of data to an address.  Not optimized to
// be usable on a dynamic RAM board.
//---------------------------------------------------------------
void write_col(uint16_t row, uint16_t col, uint16_t data )
{
  uint32_t addr = row * col_size + col;
  uint8_t lb, hb;

  // Set low and high data bytes
  PORTF = (data & 0xff); 
  PORTK = (data >> 8);

  set_address (addr);
  asm("nop");
  // Write low byte
  CLRB(PORTC, nWE0);
  asm("nop");
  CLRB(PORTC, nCE);    // -CAS low
  SETB(PORTC, nCE);    // -CAS high
  SETB(PORTC, nWE0);

  // 4). Write high byte
  asm("nop");
  CLRB(PORTC, nWE1);
  asm("nop");
  CLRB(PORTC, nCE);    // -CAS low
  asm("nop");
//  delayMicroseconds(2);
  SETB(PORTC, nWE1);
  SETB(PORTC, nRAS);
  SETB(PORTC, nCE);    // -CAS low
  asm("nop");

}

//===============================================================
// Read a 16 bit memory value from the RAM.  Not optimized for
// use with dynamic RAM.
//---------------------------------------------------------------
uint16_t read_col(uint16_t row, uint16_t col)
{
  uint32_t addr = row * col_size + col;
  uint16_t lb, hb;

  set_address (addr);
  asm("nop");
  CLRB (PORTC, nCE);    // -CAS low
  asm("nop");
  hb = PINK;
  lb = PINF;
  SETB (PORTC, nRAS);
  SETB (PORTC, nCE);    // -CAS high

  return (hb << 8) | lb;
}

uint32_t loop_count = 0;
//uint32_t error_count = 0;

//===============================================================
// Main testing loop.
//---------------------------------------------------------------
void loop()
{
  uint8_t  sreg;
  uint32_t elapsed;
  uint32_t timed = 0;
  uint32_t saveseed;
  uint16_t row = 0, col = 0;
  uint16_t value;
  char buffer[72];

  // Save PRNG seed for verification phase
  saveseed = prng_state;

  sprintf(buffer, "PRNG seed 0x%08lx : Writing..." , saveseed);
  Serial.print(buffer);

  // Set up ports F and K for output
  PORTF = 0x0;  PORTK = 0x0;
  DDRF  = 0xff; DDRK = 0xff;

  // Fill ram one row at a time.
  for (row = 0; row < row_count; row++) {
    if (timed == 0) {
      sreg = start_timer1();
    }
    fill_prng_data();
    for (col = 0; col < col_size; col++) {
      write_col(row, col, prng_data.u16[col] );
      if (timed == 0) {
        elapsed = finish_timer1(sreg);
        timed++;
      }
    }
  }

  // Make sure we're set for 16 bit read
  SETB(PORTC, nWE0);
  SETB(PORTC, nWE1);

  // Verify ram
  prng_state = saveseed;
  Serial.print(" Verifying... ");

  // Set up port F and K as input with pullups.
  DDRF  = 0x00; DDRK = 0x00;
  PORTF = 0xff; PORTK = 0xff;

  for (row = 0; row < row_count; row++) {
    fill_prng_data();
    for (col = 0; col < col_size; col++) {
      value = read_col(row, col);
      if (value != prng_data.u16[col]) {
        Serial.println(); Serial.println();
        sprintf(buffer, "Error! row %d, col %d.  Expected %04x, read %04x\n", row, col, prng_data.u16[col], value);
        Serial.println(buffer);
        Serial.flush();
        // At one time I was bumping error_count and continuing
        // but now I'm just stopping in place.
        while(1);
      }
    }
  }


  loop_count++;
  sprintf(buffer, "Success!  Passes: %d", loop_count); 
  Serial.println(buffer);

  // Show our generate/write time if this was the first row. Or we
  // somehow overflowed a uint32_t loop counter.
  if (loop_count == 1) {
    Serial.print("Time to generate and write one row: ");
    Serial.print(elapsed); Serial.println(" usec");
  }
}


