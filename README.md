harduino
========
hard. as in difficult. hardware is not intuitive to me!

This project groups a collection of Arduino sketches that run on a small collection of devices around my home:
Sketch | Description
-------- | -----
`access_manager` | Interacts with a pin pad and opens the gate if a valid pin is entered.
`alarm_monitor` | Watches the LED's on our home alarm system to understand the status of the home alarm.
`pool_temperature` | Detects and reports the pool temperature.
`rain_guage`|Tracks closures on a rain gauge. The maths is offloaded to an AWS lambda.
`switch`|Reacts to HTTP requests to switch on (and off) 24V current to activate irrigation solenoids.

## Code
The sketches all report an "alive" status via UDP.

All messages are relayed to an AWS Gateway endpoint using the code [here](https://github.com/renenw/relay).

None of the sketches are particularly complex.