WalgreenLights
==============

# Walgreens 15 C9 Multi-color Light Show LED Christmas light hack

Arduino code to control the Walgreen Christmas Light set.  See http://blog.hmpg.net/2014/01/walgreens-15-c9-multi-color-light-show.html for more details

## Overview
After Christmas I purchased a few strands of the Walgreen "15 C9 Multi-color Light Show" LED Christmas lights (WIC 276754; UPC 049022715905).  These have 15 individually addressable LED lights (3 sets of 5 LEDs arranged as red, green, blue, orange, white).  If you can find them on sale for a few dollars, they are a great deal.

The lights are apparently manufactured by BriteStar (www.britestar.com).  Unfortunately they have very little info on their website.  It appears that they intend to sell these and other similar light sets directly but they are not quite there yet.  Maybe by next Christmas season?

Seeing these lights in the package with the little "Try Me" button made it very obvious that the bulbs were individually addressable.  The brief demo in the package provides a few patterns.  I was immediately curious if they could be hacked to do other things.  Turns out that the protocol is very simple and easily implemented with an Arduino.

## Tear Down
A quick look at the string revealed several interesting things.  First, there was a transformer at the head of the string labeled 5V at 500 mA.  That seemed promising.  Second, there was a small box, near the transformer that had two lines (presumably power) going in and out of it, plus a connector for the "Try Me" button on the outside of the package.  The first bulb in the series had two wires going in and three wires going out.  That seemed interesting as well.

### Hardware
I started by pulling apart the small box next to the transformer.  My initial thought was that this might be the controller for the strand.  Alas, that was not the case.  In fact, this little box is only there to provide the connection point for the "Try Me" button and is otherwise useless.  However, it might provide a good place to slip in a controller (such as the Arduino Pro Mini) later!

A quick test with the VOM confirmed that the strand ran on 5V DC (as listed on the transformer at the head of the strand).

Next I pulled apart the first bulb in the strand.  Inside I found a gold mine.  There was a simple board with two wires (power) coming in and three wires going out.  Two wires were obviously the power and that left one for signal. In addition to the inputs and outputs there was a bonded processor and an LED.

I pulled apart the second and third bulb in the strand and found that the output of bulb one goes to the input pins of bulb two.  Likewise, the output of bulb two goes to the input of bulb three.

### Protocol
The next step was to reverse engineer the protocol between the boards.  First I hooked up the trusty DSO Nano to confirm that the third line was a data line and that it was a 5V signal.

Next, I hooked up the Saleae logic analyzer and took a look at the data signals.  After looking at the data for a bit it became obvious that there were basically two packets being sent; an "on" and an "off".  Below you can see the output of the first three bulbs.  It is clear that each board forwards it's previous state on to the next bulb after receiving a new packet.

It was also obvious that we had a very simple protocol going on.  The string of lights is basically a big shift register.  Each light stores whatever it is sent, and forwards on it's previous state to the next board.  Thus, we can keep pushing packets down the line to update the entire string.  The default firmware pushes out 15 packets each time.

Here are the specs for the packets:
* 3 start bits, each 6.25 us (microsecond) wide high followed by 6.67 us wide low pulse
* 4 data bits, each is either a 'wide' or a 'narrow' high followed by a 4.33 us low pulse
** A "one" is indicated by a 3.75 us wide high pulse
** A "zero" is indicated by a 1.24 us wide high pulse

An "on" packet consists of 4 "one" data bits while an "off" packet consists of 4 "zero" bits.  The astute reader may quickly think to themselves, "I wonder what would happen if I sent something other than all zeros or all ones."  Excellent question.

### PWM capability
The default firmware on the string only uses full on or full off.  However, the boards support 16 levels of brightness.  It turns out that sending any value between 0 and 15 (LSB first) will generate increasing intensity levels in the LED.  A packet of SSS (3 start bits) followed by bits 1000 will be the dim value of 1; a packet of SSS0100 is a dim value of 2; etc.

Packets (S = start bit, 1 = wide pulse, 0 = narrow pulse)
* SSS0000 = off
* SSS1000 = dimmer value of 1 (dimmest)
* SSS0100 = dimmer value of 2
* SSS1100 = dimmer value of 3
* SSS0010 = dimmer value of 4
* SSS1010 = dimmer value of 5
* SSS0110 = dimmer value of 6
* SSS1110 = dimmer value of 7
* SSS0001 = dimmer value of 8 (half brightness)
* SSS1001 = dimmer value of 9
* SSS0101 = dimmer value of 10
* SSS1101 = dimmer value of 11
* SSS0011 = dimmer value of 12
* SSS1011 = dimmer value of 13
* SSS0111 = dimmer value of 14
* SSS1111 = dimmer value of 15 (full brightness)

### Startup

You might be wondering how the first board knows it is the master.  The short answer is that if you don't see any packets on your input pin, you are the master.  A very simple startup protocol is to raise your output pin high for 150 ms (milliseconds) while looking at your input pin to see if you have an upstream neighbor (who would be doing the same thing on their output pin).  If you don't see anything on your input, then you must be the master.  

There is also a failsafe in the system that if you do not see any input for approximately 20 seconds, you should become the master.  Likewise, if you begin seeing valid packets on your input, you go into slave mode and start doing whatever you are told.

At startup, the master sends 10 sets of "off" packets.  It appears that if a board has no current state, it simply consumes a packet and does not send anything.  You can see this in the following capture.  Each white bar represents an entire packet.  I zoomed out so you can see the cascade of the first three bulbs as packets are being sent.


### Startup packet sequence

After discovering the basics of the protocol, I quickly created an Arduio program to simulate the protocol and started controlling the string myself.  Since the Arduino doesn't directly support this protocol I had to bit bang the line.  My ultimate goal is to be able to run multiple strings of lights from a single Arduino.  Since the times are so short (as small as a single microsecond) I had to disable interrupts during the packet transmission.

Source code for a class implementing the protocol and a quick test program is available in github.

## Interpacket gap
The default firmware waits about 1.13 ms between sending packets.  This gives more than enough time for a packet to propagate all the way to the end of the strand.  This seemed wasteful to me so I shortened the gap to 100 us.  It seems to work just fine.
