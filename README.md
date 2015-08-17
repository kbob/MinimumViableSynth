# MinimumViableSynth

A virtual analog synthesizer with lots of knobs and buttons.

![front panel image](image/Synth Panel.png?raw=true)

This is a polyphonic, monotimbral, virtual analog synth.  The audio
engine runs as an Apple Audio Unit on a Macintosh.  The control panel,
shown here, will provide dedicated controls for most of the synth's
functions.

## Status

August 14, 2015:

The audio engine has been stable since January, though it does not
exactly match the front panel's feature set.

The front panel was fabricated in July.  I used a sheet of acrylic,
painted the underside black, then engraved the artwork through the
paint with a laser cutter.  Then the same laser cutter cut all the
holes for the controls.  It looks sharp, but not mass produced.
I got the work done at [InHaus Fabrication](http://inhausfabrication.com/).

There is a master microcontroller, an STM32F429, and each module has a
slave controller, a Teensy-LC.  Each slave directly drives a few
knobs, buttons, and LEDs.  The master will coordinate all the slaves
and provide the MIDI over USB interface.  I am currently writing SPI
drivers for master-slave communication.


