# Toorcamp LED Sign

## Installation

Each of the eight letters (`T`, `O1`, `O2`, `R`, `C`, `A`, `M`, `P`) has a corresponding wooden stand, which fits on the back side. Drive screws through the face of each letter, into the stands.

There are eight plastic Tupperware-style containers, marked for each letter. These are weatherproof enclosures for the electronics. Each container should be screwed onto the back of each stand, with the holes pointing downward.

Cables should be run through the Tupperware holes. The Tupperware lids should be sealed shut. Each Tupperware will contain a 12V power supply, a white Protoboard, and a green [OctoWS2811 adapter](https://www.pjrc.com/store/octo28_adaptor.html) (the last two are connected with solid AWG 22 wires).

### Connections

* **12V Power Supply cord:** Connect the three leads to the power supply terminals:
    * `L`: black wire
	* `N`: white wire
	* `⏚` (ground): greyish wire

* **Protoboard ground (black wire, 22 AWG stranded):** Connect to a `-V` terminal on the 12V power supply. (We are tying the grounds of the 5V and 12V power supplies together, so they have the same ground level.)

* **LEDs, white 16 AWG wire with 2 conductors:** Connect these to the 12V power supply terminals:
	* Black: `-V`
	* Red: `+V`

* **5V power supply ("wall wart"):** Plug this into the barrel connector on the white protoboard.

* **LEDs, CAT6 cable:** Connect to the RJ45 jack on the green OctoWS2811 adapter. (Note that the adapter has two jacks, an arrow points to the correct jack.)

* **120 VAC:** Plug both the 12V power supply cord, and 5V wall wart, into an extension cord (120 VAC). Please cover any gaps around these connections, and exposed outlets, with electrical tape, to protect against the elements.

When everything is connected properly, and power is applied, you should see a default "attract" animation displayed on the LEDs. Each letter has a different attract animation. The human-readable code for these animations is in `docs/lexer_notes.txt`, and the bytecode version is hard-coded in `LexerMicro/LexerMicro.ino`.

## CAT5e network

The eight letters communicate using a simple CAT5e network. Using the provided (yellow) 7-foot CAT5e cables, connect each letter to its neighbor, using the RJ45 jacks on the white protoboards. With the board oriented so the barrel jack is on the *right*:

* The RJ45 jack with pins on the *left* (MAX490 IC pins 5 and 6) goes downstream (transmit). `C` → `A`, `A` → `M`, `M` → `P`, etc.

* The RJ45 jack with pins on the *right* (MAX490 IC pins 7 and 8) goes upstream (receive). `C` → `R`, `A` → `C`, `M` → `A`, etc.

The network transmits using an [RS485 signal](https://en.wikipedia.org/wiki/RS-485), at 9600/8-N-1 baud.

Network messages are typically short, between 3-12 bytes. Each message begins with a "lifespan byte" between `8` and `1`, and ends with a newline `'\n'`. Messages are always passed downstream unaltered, except for the lifespan byte, which is decremented when a Teensy receives it. When the lifespan is exhausted (`1` is received, and decremented to `0`) the message is not passed.

### Ring network topology (not implemented yet)

*Known issue:* The `T` board is missing a 100-ohm terminating resistor connecting MAX490 pins 7 and 8. It cannot receive data until this resistor is added. (This is an easy fix.)

After the terminating resistor is added, then a 50-foot CAT5e cable can be connected from `P` all the way back to `T`, creating a ring network.

If the ring network is established, this enables every letter to send messages to all other letters. This allows sensor data to be shared, and the sign can become interactive. (See branch [`feature/ultrasonic`](https://bitbucket.org/zkarcher/toorcamp_sign/branch/feature/ultrasonic) on the code repository.)

## Code setup

* Clone this repository: [https://bitbucket.org/zkarcher/toorcamp_sign/](https://bitbucket.org/zkarcher/toorcamp_sign/)

### Flash a Teensy 3.2

* Download and install [Arduino IDE](https://www.arduino.cc/en/Main/Software), and then install [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html).
* Connect a USB micro cable from your laptop to the Teensy. Ensure the Teensy has power (via the 5V wall wart), and appears in the `Port` sub-menu. Select `"Teensy 3.1/3.2"` as the microcontroller.
* Open the `LexerMicro/LexerMicro.ino` file in the Arduino application.
* **Important:** At the top of the code, change `STATION_ID` to the correct index for the letter. `T`==(0), `O1`==(1), `O2`==(2), `R`==(3), etc.
* Click the `Upload` button.

### Dynamic code execution

The Teensys are executing dynamic code, which allows for animations to update immediately (no waiting for the compile-flash cycle to finish). This is a powerful creative tool, and makes live coding possible. To use this:

* In one terminal window, start the server script. This connects the web front-end (WebSocket) to the Teensy (USB serial):
	* `cd server`
	* `npm install`
	* `npm start`
	* If your computer cannot find a connected Teensy, the script will exit with a warning.

* In a second terminal window, start the web front-end:
	* `cd html`
	* `npm install`
	* `npm start`
	* Browse to: [http://localhost:9001/](http://localhost:9001/)
	* Try copy-pasting code samples from `docs/lexer_notes.txt`. Tweak these, or write your own.

## Bill of Materials

[https://docs.google.com/spreadsheets/d/1d07su_DdPGAXrdxyUl6WDVSRFD1-QwRbDe_fzCo3z0c/edit#gid=0](https://docs.google.com/spreadsheets/d/1d07su_DdPGAXrdxyUl6WDVSRFD1-QwRbDe_fzCo3z0c/edit#gid=0)

## Future upgrades

A list of potential upgrades:

* Edge materials: These are a kind of "strong cardboard". These have warped, due to environmental moisture. Tear these off, and replace with painted 1/4" plywood.

* Missing 100-ohm resistor on `T` MAX490 receive pins. (See *Ring network topology*, above.)
* Ultrasonic distance sensors. (See *Ring network topology*, above.)
* Increased baud rate, for faster network speeds. Can this work reliably at >9600 baud? How far (ShadyTel network)?
* Faster message sharing. Currently messages are only shared if the microcontroller believes messages are valid (begin with a lifespan byte, end with newline `'\n'`. Every byte can probably be shared immediately, without pre-validation.
* Security: Validate incoming messages. Ensure bytes are in the valid range. Check for potential bugs with using 2 message buffers (USB serial, and the CAT5e network).
* Simplified wiring: Use 12V→5V voltage regulators, which would allow the 5V wall warts to be omitted. (I purchased these regulators, but ran out of time, and didn't implement this.)
* Middle LED strands: Originally each "stroke" was intended to have 3 parallel strands of LEDs. Due to time constraints, we settled for 2 strands, which "outline" the letters. The wiring exists to add the missing 3rd strand: Use the unused CAT6 wire (colors are: orange, blue, green; see the [OctoWS2811 adapter docs](https://www.pjrc.com/store/octo28_adaptor.html)) and the extra 18 AWG 12V power wire (grey) that leads to the LEDs.
* Enhanced attract mode: The `T` can cycle through animations, and send them to the other letters. (Extra credit if the animations crossfade, somehow.)
* Time sync: The Teensys sometimes drift out of sync, due to factors like temperature differences. Consider adding a message that forces each Teensy to sync its clock (elapsed time in seconds). Or, would clocking the Teensy CPUs at a slower speed prevent this?
* Custom PCBs. (TODO: Learn KiCad)
* Live coding kiosk, so everyone can code animations. (Need: monitor, keyboard, burner laptop or SoC, wooden stand/enclosure.)
* Parser bugs: The parser occasionally behaves badly, especially long sequences without parens. Statements like "2 * X + Y * 4" are sometimes evaluated in an unexpected (wrong) order. Debug this?
* Custom gamma curves.
* More dynamic code functionality for artistic use: Ramps with variable slopes. ADSR envelopes and triggers. Particle buffers. *etc*
