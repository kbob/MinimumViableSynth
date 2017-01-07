# MinimumViableSynth

A virtual analog synthesizer with lots of knobs and buttons.

![front panel image](image/Synth_Panel_color.png?raw=true)

This is a polyphonic, monotimbral, virtual analog synth.  The audio
engine runs as an Apple Audio Unit on a Macintosh.  The control panel,
shown here, will provide dedicated controls for most of the synth's
functions.

![assembly animation](image/assembly.gif?raw=true)

The end goal is a self contained keyboard synth with knobs and buttons
with the classic connections: audio out, MIDI in/out, USB MIDI in/out.
I am getting there in three phases.

  * **Phase 1** (complete 2015-01-01): Softsynth.  The audio engine
    runs as an Audio Unit on a Mac.  It accepts MIDI messages and
    produces beautiful polyphonic analog sounds.  Or gritty,
    disturbing analog sounds.  It plugs in to DAW software (I use
    Logic Pro X).
    
  * **Phase 2** (in progress): Knobs and buttons.  I am designing and
    building a front panel (see above) with knobs, buttons, and an LCD
    touchscreen.  The goal is to make it as easy as possible to
    understand what the synth is doing at any time.

  * **Phase 3** (future): Stand-alone.  Build an enclosure that houses
    a keyboard, the knob and button panel, I/O jacks, and a processor
    (TBD) that can run the audio engine.  I'm looking at FPGAs...


## Why?

Learn stuff.  Music theory, DSP, PCB layout, CAD and machining, more.

    
## Status

Check out synth-notes.txt for my thoughts as I worked things out.


#### January 7, 2017:

I built 8 slave modules: the LFOs, the Controllers module, the
oscillators, the noise source, mixer, and filter.  I've since killed
the filter module, and one of the oscillators' LED has burnt out its
red component.  I have parts and tools to repair those; just haven't
done it yet.

I have working UI for the knobs and buttons.  Specifically, routing
between modulators and targets works, complete with lighting effects.

I made an enclosure from aluminum sheet and melamine end blocks, as
illustrated above.  It is flimsy, though, so that needs to be re-done.

I had the master, slaves, and LCD panel connected via a breadboard.
The breadboard wiring got flaky, so I stopped to design a PCB.  The
PCB snaps onto the back of the Discovery board, It routes video to the
LCD touch screen, SPI to the slaves, and serial MIDI to a future MIDI
I/O board.

I am currently designing the graphics for the LCD panel.

(Over a year since the last update.  Oops.)


#### December 17, 2015:

Three slave modules have been built.  (LFO1, LFO2, Controllers.)  All
the slave module PCBs are fabricated.

The master-slave communications are working over SPI.  The master
generates MIDI events.
[I can push a button on a slave and hear its effect on the audio
engine.](https://www.youtube.com/watch?v=dZsXk2Zf6Bg)

I blew up one of the LFO modules, probably with static discharge.  I
repaired it (swapped in a new Teensy LC), and it works now.  It was an
ugly repair.

Current tasks:

  + generating more MIDI events
  + experimenting with the LED animation
  + soldering up the rest of the slave modules
  + designing backing plates for the new slave modules


#### August 14, 2015:

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
