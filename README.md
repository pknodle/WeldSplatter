# Weldsplatter
A VCV Rack plugin focusing on 12-tone music.  And, you know,
bleepy-bloopy stuff...

See: [this video](https://youtu.be/tLkYu2iNsRs?si=sa8KAtsk2MNGBMT4) and [this](https://unitus.org/FULL/12tone.pdf) 
for background. 


## Usage

The general idea is that you hook up a volts/octave source and a gate to the "Midi In" ports.  The intention 
is that you use the MIDI->CV module, as shown in the example patch.  Other sources should work and could lead 
to interesting results!

Before playing a 12-tone row, pres the "Teach" button.  This will enter teaching mode where the 
module will remember which notes you play and in what order.  

After you have played 12 notes, the module will enter pad mode.  In this mode, it acts like a big input 
pad where each button is an entry in a cell in the 12-tone matrix. 

If you want to play boring tonal music, you can press the "Allow Rep" button to allow repetitions of 
notes.  In this mode, the module won't limit you to a strict 12-tone row.  

The default is to sort of break the rules of a 12-tone matrix to make a more ergonomic pad. 
The notes are ascending from top to bottom, and from left to right.  The upper left note is the lowest while 
the lower right is the highest.  If you want to only use one octave, press the "Single Octave" button.

You can hook up external inputs to the "Row" and "Col".  These will select the matrix cell when you 
are in external input mode (press the "Ext Input" button)

## Example
There is an example patch in the documentation folder.


