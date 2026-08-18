# coco3_512k_ram_board_test
![Mega 2560 set up for test](img/mega_2560.jpg)

Since my Color Computer 3 is non-functional at the moment due to a bad GIME chip, this project was created in order to test some 512k RAM boards created using the Kicad project files available at 6809.org.uk.  JLCPCB didn't have the RAM chip and single-flipflop chip, but Mouser Electronics did, so I populated those two myself.

Initial testing was done on a clone Mega 2560 board.  The second test was with a Mega 2560 Pro Embed from Robotdyn (and some clones)

![Mega2560 Pro Embed set up for test](img/mega_pro.jpg)

This particular RAM board uses a 16 bit x 256k static RAM chip which matches the Color Computer's memory expansion bus.  It was designed for two banks of eight 41256 RAM chips.  See the 6809.org.uk/dragon project information for particulars.

The memory test generates 1 row (128 32 bit values) using a Marsaglia xorshift32() PRNG.  The initial seed is 0xdecafbad which is modified at startup by reading all the analog inputs and adding values to it.  See the code.

It took approximately 35 hours to completely cycle through a write and verify of 256k words at a time (16384 loops).  You probably need far fewer than that.

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

TODO: Add more to this readme.

