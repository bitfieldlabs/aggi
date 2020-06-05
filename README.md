![afterglow](https://github.com/bitfieldlabs/aggi/blob/master/doc/afterglow_logo.png "Afterglow GI")

AFTERGLOW GI is a WPC pinball machine extension board that aims at softening the hard light on/off transitions that result in replacing the original GI incandescent bulbs with LEDs.

The brain of the board is a Arduino nano. It monitors the triac signals on the WPC bus and adjusts the PWM of the GI lines accordingly in order to dim the LEDs. For this purpose the whole GI is powered on DC voltage as opposed to the original AC 6.3V.

There are two options to power the Afterglow GI board and all GI LEDs:

:open_file_folder: [afterglow_gi_nano](https://github.com/bitfieldlabs/aggi/tree/master/afterglow_gi_nano)<br>
Board revision 1.0 needs to be powered from an external power supply. Recommended is a 12V supply delivering at least 8A.

![afterglow GI](https://github.com/bitfieldlabs/aggi/raw/master/doc/images/pcb_v10_populated.jpg "Afterglow GI")

:open_file_folder: [afterglow_gi_nano_ps](https://github.com/bitfieldlabs/aggi/tree/master/afterglow_gi_nano_ps)<br>
Board revision 1.1 features a power supply and can be attached to the WPC power supply's 6.3V AC connector directly.
