This custom MEGA2560 bootloader allows LiBCM to initially power up fast enough to not cause P1648 CEL.

A standard MEGA2560 will otherwise work with LiBCM, but will cause P1648 at the first key-on event after the IMA power switch is turned on.  After that, as long as LiBCM has not turned itself off (e.g. due to low battery), LiBCM will remain powered up, hence no P1648 error.

If you purchased an LiBCM PCB from me, then it already has the LiBCM bootloader on it.
If you're programming your own MEGA2560, then you can load the .hex file onto your MEGA using the "Arduino as SPI burn bootloader" method (do a google search).

To compile the file (e.g. if you don't want to use the pre-compiled hex file), you will need to install avr-gcc (e.g. via homebrew in MacOS), and then make the file ("make mega2560").  This is an advanced method, and if you don't know how to do it, just install the hex file instead.

The contents of this folder generate the "stk500boot_v2_mega2560.hex" file, which is then uploaded to the Arduino MEGA.