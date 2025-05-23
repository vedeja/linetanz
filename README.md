# LineTanz

A hardware bridge for adding physical controllers to the Mackie DL series audio mixer.

## Working Principle

LineTanz runs on an [Arduino](https://www.arduino.cc) and acts as a regular mixer client, just like Master Fader app. This means establishing a TCP connection over wifi with the mixer and exchanging the correct messages. The footswitch triggers the toggling of FX input channel mute states, then sends the associated mixer messages.

![image](./LineTanzPrinciple.png)

## Background

I have a [Mackie DL32R](https://mackie.com/intl/products/mixers/dl-series) mixer, which I use with my band. The DL mixers are controlled purely via the [Master Fader](https://www.google.com/search?client=safari&rls=en&q=mackie+master+fader+app&ie=UTF-8&oe=UTF-8) app on a device, in my case an Apple iPad. This is great, so far. But as a performing musician with no sound engineer on the gig (except myself), some simple things need to be taken care of. One is to mute the vocal effects (reverb, delay etc) in between songs. This can be done using Master Fader, but is fiddly on a small screen requiring at least four touch interactions. Many other mixers have a footswitch jack where an external controller can be connected an used to control FX mute, but the DL32R does not have such a feature. What a shame for an otherwise wonderful mixer. LineTanz solves that.

## Wireshark dissector

The file [mackie_dl32r.lua](/mackie_dl32r.lua) is a Wireshark dissector that is used together with Wireshark to help decode the Mackie DL TCP messages. I use it to reverse engineer the mixer protocol and re-implement its features in LineTanz. Install the dissector by placing it in Wireshark's plugin folder. On my Mac, that is `/Applications/Wireshark.app/Contents/PlugIns/wireshark`.

## Future ideas

- Playback control (select song, start/stop playback).
- Tap tempo control
- Full MIDI implementation and MIDI hardware jack, allowing for a mixer control surface to be connected via MIDI cable.
- LED indicators to show mute state etc.
- Configuration via inputs such as encoders and buttons and an LCD screen or similar.

## Credits

Thanks goes out to [John Skeet](https://github.com/jskeet) who created [DigiMixer](https://github.com/jskeet/DemoCode/tree/main/DigiMixer) as part of his [DemoCode](https://github.com/jskeet/DemoCode/tree/main), and thereby reverse engineered huge parts of the Mackie DL [communications protocol](https://github.com/jskeet/DemoCode/blob/main/DigiMixer/Protocols/mackie.md). His solution is written in C# and has been of great help in creating LineTanz.