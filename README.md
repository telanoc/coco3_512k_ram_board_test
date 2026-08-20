# coco3_512k_ram_board_test
![Mega 2560 set up for test](img/mega_2560.jpg)

My Color Computer 3 is non-functional at the moment due to a bad GIME chip.
That means that I didn't have a good way to test the five 512k RAM boards
(thanks, JLCPCB) that I built using the Kicad project at
www.6809.org.uk/dragon/ (see the "CoCo 3 512K SRAM" header).

Initial testing was done on a clone Mega 2560 board.  The second test was
with a Mega 2560 Pro Embed from Robotdyn (and some clones).

![Mega2560 Pro Embed set up for test](img/mega_pro.jpg)

Although this test was designed for a board using static RAM, it should
either work or be adaptable to a Coco 3 expansion that uses DRAM chips.  The
memory test generates 1 row (128 32 bit values) using a Marsaglia xorshift32
PRNG.  The array fill takes about 925 microseconds, and adding a row write
gets up to about 1.05 milliseconds so there should be no problem with
refresh timing on a DRAM chip.

The initial seed is 0xdecafbad which is modified at startup by reading all
the analog inputs and adding the absolute values to it.  It then cycles the
PRNG a few times.  See the code.  There's no reason for making it random
other than desire.  There are 16 analog inputs that read some noise, they
might as well be put to use.

It took approximately 35 hours to completely cycle through the PRNG doing a
write and verify of 256k words at a time (16384 loops).  You probably need
far fewer than that to consider a board good.

The connections between the RAM board and the Mega are as follows:

```
RAM          Mega 2560          RAM            Mega 2560
---------    ----------         ----------     ----------
 1 - GND      GND                1 - GND       GND
 2 - +5V      +5v                2 - /RAS      36 (PC1)
 3 - D9       A9                 3 - Z0        22 (PA0)
 4 - D8       A8                 4 - Z1        23 (PA1)
 5 - D10      A10                5 - Z2        24 (PA2)
 6 - D11      A11                6 - Z3        25 (PA3)
 7 - D14      A14                7 - Z6        28 (PA6)
 8 - D15      A15                8 - Z5        27 (PA5)
 9 - D13      A13                9 - Z4        26 (PA4)
10 - D12      A12               10 - Z7        29 (PA7)
11 - /WE1     34 (PC3)          11 - Z8        30 (PC7)
12 - GND      GND               12 - GND       GND
13 - GND      GND
14 - D2       A2
15 - D3       A3
16 - D1       A1
17 - /WE0     35 (PC2)
18 - D0       A0
19 - /CAS     37 (PC0)
20 - D7       A7
21 - D5       A5
22 - D4       A4
23 - D6       A6
24 - GND      GND
```

Sample Output

On the first pass, the time to generate and write one row of data
is displayed, just for information.

```-------------------------------------
Coco Static RAM tester

Random start addition is 0x2DD4
Time to generate one row of PRNG data: 927 usec
PRNG seed 0x9cbf715b : Writing... Verifying... Success!  Passes: 1
Time to generate and write one row: 1048 usec
PRNG seed 0x5a34a2ee : Writing... Verifying... Success!  Passes: 2
PRNG seed 0x7656b523 : Writing... Verifying... Success!  Passes: 3
PRNG seed 0xf2578867 : Writing... Verifying... Success!  Passes: 4
```

TODO: Add more to this readme.

