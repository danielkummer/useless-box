# useless-box
A useless box is basically a machine which turns itself off. However, with a few additional servos, and a little programming, you're able to give the thing some "character" - and add a few surprises for the user...

## Features / surprises

My box can:

* Control the door and "hand" separately
* Detect the operator by using a distance sensor
* Wave a white flag
* Scoot away

I've used software debouncing of the switch because it's much easier to build and less to solder...
Although the MotorShield is clearly overkill for this project it significally reduces the complexity of the required electronics knowledge (and soldering).

## Hardware

### Electronics

The following electonics were used:

* Arduino UNO
* [MotorShield](https://www.arduino.cc/en/Main/ArduinoMotorShieldR3)
* 2 x Servo
* 1 x Micro Servo
* Geared DC Motor
* 2x Flip Switch
* 11.1v Battery ~ 1200 mAh
* 10k Resistor (I used software debouncing)
* Infrared based distance sensor

### Material

* Nice wooden box with enough space to fit everything into
* Rubber band to force-close the lid
* Some chipboard ~ 4mm to build the arm and mountings
* Some hinges
* Some cables

### Tools

* Solder
* Jigsaw
* Sanding paper
* Wood glue



## Operation

My useless box fullfills of course the basic functionality of turning itself off.
But using a few additional pieces of equipment it can:

* Drive away
* Detect the users distance (and react to it)
* Wave a white flag - but without giving up of course :)

# Software

The box has basically a fixed set of "moves" it can do to turn of the switch after it has been turned on. To make things a little more interesting, the moves are randomly used to surprise the operator.

The whole code is quite easy - there's nothing special about it.
I've created a few bases classes to control every periperal and another class "Moves" to implement the patterns. This is mainly because the basic arduino way of programming leads to lots of code duplication and unreadable code
