# ofxBlackMagic is an addon for [openFrameworks](http://openframeworks.cc/)

This fork implements `ofxBlackmagicGrabber` as a subclass of `ofBaseVideoGrabber`, allowing it to be set as the `grabber` of an `ofVideoGrabber` instance (like `ofQuicktimeGrabber` and `ofQTKitGrabber`) making it more compatible with camera based code.

This work is adapted from Kyle McDonald's `ofxBlackMagic` class, which in turn was derived from studying an older addon, [ofxBlackMagicGrabber](https://github.com/arturoc/ofxBlackmagicGrabber), and the [DeckLink SDK](http://www.blackmagicdesign.com/support/sdks) sample code.

The goal is provide an interface with Blackmagic devices that integrate seamlessly with openFrameworks projects using `ofVideoGrabber`.

## Some advantages of this addon:

* Compatible with projects using `ofVideoGrabber` where previously using `ofQuickTimeGrabber` or `ofQTKitGrabber`
* All DeckLink specific functionality is placed in `DeckLinkController.h`, which can be extended if you're interested in getting minimum latency by overloading the `VideoInputFrameArrived()` callback.
* Raw data is triple buffered to provide minimum delay to the DeckLink device and no tearing on the display side.
* YUV to RGB conversion is only done when the user requests to use or display RGB data.
* YUV to RGB conversion algorithm is optimized to increase memory locality.
* YUV to Grayscale conversion is provided for situations where color conversion is unnecessary.
* Raw YUV data is provided as a texture for decoding or processing with a shader.

## Supported System

This addon has been checked with:

- OS X 10.9, MacBook Pro Retina, openFrameworks 0.8.3, Blackmagic UltraStudio Mini Recorder, GoPro Hero3 set to 1080i25

The original addon has been checked with:

- OS X 10.8, Mac Mini, openFrameworks 0.8.0, UltraStudio Mini Recorder, single 1080p30 camera.
- OS X 10.9, MacPro trashcan, openFrameworks 0.8.3, UltraStudio 4k Thunderbolt 2 device. 

## Installation

First, install the [Black Magic software](http://www.blackmagicdesign.com/support). If you are using an UltraStudio Mini Recorder, you should download [Desktop Video 10.1.2 for Macintosh](http://www.blackmagicdesign.com/support/detail?sid=3958&pid=31781&leg=false&os=mac). After installation, go to System Preferences, click "Black Magic Design" and make sure "Use 1080p not 1080PsF" is checked (this option is only available when the capture card is plugged in). If you don't check this option, nothing will work.

Then go to the [support](http://www.blackmagicdesign.com/support/sdks) page and download the DeckLink SDK (currently at version 10.1.1). After unzipping the SDK open the app `Mac/Samples/bin/CapturePreview` and select the video format of your device and hit "Start". If you have the right mode selected you should see the video streaming.

One you see the demo app working, try building and running the `example/` that comes with `ofxBlackMagic`.