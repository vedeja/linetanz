## DL16S address layout

The overall layout of the DL16S address space is:

* 1-1600: Inputs 1-16, 100 bytes each
* 1601-1776: Returns 1-2, 88 bytes each
* 1777-1824: FX 1-4 (outputs), 12 bytes each
* 1825-1948: FX 1-4 (parameters), 31 bytes each
* 1949-2212: FX 1-4 (inputs), 66 bytes each
* 2213-2518: Subgroups 1-6, 51 bytes each
* 2519-2605: LR, 87 bytes
* 2606-3145: Aux 1-6, 90 bytes each
* 3146-3205: VCA 1-6, 10 bytes each
* 3206-3215: Output mapping
* 3216-3231: USB mapping


## DL32R address layout

The overall layout of the DL32R address space is:

* 1-3200: Inputs 1-32, 100 bytes each
* 3201-3552: Returns 1-4, 88 bytes each
* 3553-3600: FX 1-4 (outputs), 12 bytes each
* 3601-2724: FX 1-4 (parameters), 31 bytes each
* 2725-2988: FX 1-4 (inputs), 66 bytes each

TODO:
* 2213-2518: Subgroups 1-6, 51 bytes each
* 2519-2605: LR, 87 bytes
* 2606-3145: Aux 1-6, 90 bytes each
* 3146-3205: VCA 1-6, 10 bytes each
* 3206-3215: Output mapping
* 3216-3231: USB mapping

FX1 mute = 0x1367 (4967 dec)