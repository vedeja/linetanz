# LineTanz

A hardware bridge for adding physical controllers to the Mackie DL series audio mixer.

## Working Principle

LineTanz acts as a regular mixer client, just like Master Fader app. This means establishing a TCP connection over wifi with the mixer and exchanging the correct messages. The footswitch triggers the toggling of FX input channel mute states, by sending the associated mixer messages.

![image](./LineTanzPrinciple.png)

## Background

I have a [Mackie DL32R](https://mackie.com/intl/products/mixers/dl-series) mixer, which I use with my band. The DL mixers are controlled purely via the [Master Fader](https://www.google.com/search?client=safari&rls=en&q=mackie+master+fader+app&ie=UTF-8&oe=UTF-8) app on a device, in my case an Apple iPad. This is great, so far. But as a musician with no sound engineer except myself, some simple things need to be taken care of. One is to mute the vocal effects (reverb, delay etc) in between songs. Many other mixers have a footswith jack where an external controller can be connected to solve this requirement, but the DL32R does not. This project solves that.

Credit goes out to [John Skeet](https://github.com/jskeet) who created [DigiMixer](https://github.com/jskeet/DemoCode/tree/main/DigiMixer) as part of his [DemoCode](https://github.com/jskeet/DemoCode/tree/main), and thereby reverse engineered huge parts of the Mackie DL communications [protocol](https://github.com/jskeet/DemoCode/blob/main/DigiMixer/Protocols/mackie.md). His solution is written in C# and has been of great help in creating LineTanz.