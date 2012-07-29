axerig2
=======

A simple program to be able to control the FractalAudio
[Axe-Fx II](http://www.fractalaudio.com/) guitar processor
with a Native Instruments
[Rig Kontrol 2](http://www.native-instruments.com/#/en/products/guitar/guitar-rig-kontrol/).
Both devices are connected to a linux box via USB.

First, make sure the Axe-Fx is recognized by linux. See the linux and fxload
[notes](http://wiki.fractalaudio.com/axefx2/index.php?title=USB_connectivity)
on the Axe-Fx [wiki](http://wiki.fractalaudio.com/axefx2).

Then, we're lucky, necessary ALSA drivers for the Rig Kontrol 2 are included in
kernel versions 2.6.22 and above. axerig2 translates the Rig input events to midi
CCs and send them to the Axe-Fx.

The Rig's seven foot switches and the pedal are configurable via a simple
config.json file.

``` javascript
{
  // Rig Kontrol 2 input device
  "rig_device": "/dev/input/by-path/pci-0000:00:1a.0-usb-0:1.1-event",

  // Axe-Fx midi port
  "axe_midi": {
    "port": "AXE-FX II",
    "channel": 0
  },

  // foot switch
  "switch": {
    "1": { "cc": 121 },// looper undo
    "2": { "cc": 32 }, // looper reverse
    "3": { "cc": 30, "toggle": true }, // looper once
    "4": { "cc": 28 }, // looper record
    "5": { "cc": 29, "toggle": true }, // looper play
    "6": { "cc": 31 }, // looper dub
    "7": { "cc": 122, "toggle": true }, // metro // 120 looper half
    "pedal": { "cc": 17 } // external controller 2
  }
}

```

### Building ###

Dependencies: [ALSA](http://www.alsa-project.org), [boost](http://boost.org)
header only for the property_tree library, GCC 4.4 or more with -std=c++0x.

```
$ INCLUDES="-I/path/to/boost/include" make
```

### Using axerig2 ###

Plug the Axe-Fx and the Rig Kontrol to your computer via USB.

Prior launching axerig2 you probably want to prevent the Rig input events to be
handled as keyboard events. One solution is to disable the device with xinput
as shown bellow.

Edit config.json to match your Rig device. The Axe-Fx midi specs should remain
unchanged. Configure the switches CCs.

```
$ xinput --set-prop "RigKontrol2" "Device Enabled" 0
$ ./axerig2
```

#### Useful link ####

[Axe-Fx MIDI CCs list](http://wiki.fractalaudio.com/axefx2/index.php?title=MIDI_CCs_list)

[Axe-Fx forum](http://forum.fractalaudio.com/forum.php)
