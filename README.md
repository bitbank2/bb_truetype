bb_epaper (BitBank epaper library)<br>
----------------------------------
Project started 9/11/2024<br>
Copyright (c) 2024 BitBank Software, Inc.<br>
Written by Larry Bank<br>
bitbank@pobox.com<br>
<br>
<b>What is it?</b><br>
A frustration-free e-paper library suitable for Arduino, Linux, or random embedded systems with no OS.<br>
<br>
<b>Why did you write it?</b><br>
I've always had an interest in graphics and displays. After experimenting with epaper, I looked around for code to control them. All I found was half-implemented demos and frustration. I wrote my own support for epaper panels into my OneBitDisplay library, but it made it a bit unwieldy. E-paper is in a class by itself, so I decided to create a unique library for working with these panels. The main goal was to reduce frustration and make an efficient set of functions that work with a large collection of panels in a consistent way.<br>
<br>
<b>What's special about it?</b><br>
It's the first epaper library I've ever seen which can draw text and graphics without needing a local copy of the image data. There are some limitations to working that way, but it opens the possibility to control large displays on MCUs with nearly no memory. It also supports keeping a local copy of the graphics in RAM (the normal way to work with epaper). By offering both options, this library is unique.<br>
<br>

Features:<br>
---------<br>
- C API and C++ wrapper class with all of the usual graphics functions<br>
- Supports a large number of panels in a consistent way, without tons of repeated code<br>
- Supports any number of simultaneous displays of any type (mix and match)<br>
- Includes a unique compressed bitmap font format<br>
- Text cursor position with optional line wrap<br>
- A function to load a Windows BMP file<br>
- Optimized Bresenham line drawing<br>
- Optimized Bresenham outline and filled ellipse drawing<br>
- Optimized outline and filled rectangle drawing<br>
<br>
This code depends on my Group5 data compression library (included).<br>
See the Wiki for help getting started<br>
https://github.com/bitbank2/bb_epaper/wiki <br>
<br>

A few words about fonts<br>
-----------------------<br>

The library includes 3 fixed fonts (6x8, 8x8 and 12x16). The 12x16 is really a stretched+smoothed version of the 6x8 to save FLASH space. To use more elaborate fonts with more extensive character support, use my BB_FONT format compressed bitmap fonts. This functionality is part of my Group5 compression library.<br>

Black & White (& Red)<br>
------------------------<br>
The current code supports 1-bit black and white epaper displays as well as the B/W/R models. At the time of this writing, the BWR models can only do a full refresh (no fast nor partial). This may change in a future version.<br>
<br>
See WiKi and example code for how to use the library.<br> 
<br>

If you find this code useful, please consider sending a donation or becomming a Github sponsor.

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=SR4F44J2UR8S4)

