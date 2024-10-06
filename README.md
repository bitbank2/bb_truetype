bb_truetype (BitBank TrueType font renderer library)<br>
----------------------------------------------------
Original source by https://github.com/garretlab/truetype<br>
extended by https://github.com/k-omura/truetype_Arduino/<br>
bugfixes and improvements by Nic<br>
Major improvements and rewrite into C by Larry Bank<br>
bitbank@pobox.com<br>
<br>
<b>What is it?</b><br>
A reasonably functional TrueType rasterizer for constrained environments (e.g. embedded devices). It can draw nice looking characters from TTF files on any target display / CPU.<br>
<br>
<b>Why did you write it?</b><br>
My recent work with e-paper displays reminded me that good looking fonts are available in TrueType format and it would be useful to have a way to use them directly on microcontrollers. This code will also be used for my font compression tool to allow it to run on MCUs too. This is my first library derived from someone else's work. I'm better at code than math and TrueType fonts need math to function, so it was more practical to start from some working code. One of my improvements to the original code was to remove all of the unnecessary floating point conversions.<br>
<br>
<b>What's special about it?</b><br>
As with all libraries that I write for embedded systems, I have three main goals: speed (optimize the code as much as possible), memory usage (there's no malloc/free in case it's used on a setup with no OS), and it's written in C with a C++ wrapper class so that it can compile without problems on any target CPU. There doesn't seem to be a good TTF rasterizer for MCUs, so hopefully this project can grow into that role.<br>
<br>

Features:<br>
---------<br>
- C API and C++ wrapper class<br>
- No dynamic memory allocation; if there's a memory leak, it's not the library's fault :)
- Allows drawing of character outlines, filled interiors or both in two different colors<br>
- DrawLine() callback function allows your code to have complete control over the output. This also allows for running the code with no local framebuffer.
- Can draw characters of any size. Some internal limits may need to be raised to draw characters larger than 150pt.
- Only requires 8K of RAM (the default structure limits) to draw characters of almost any size.
<br>
See the Wiki for help getting started<br>
https://github.com/bitbank2/bb_truetype/wiki <br>
<br>

See the Arduino and Linux example code for how to use the library.<br> 
<br>

If you find this code useful, please consider sending a donation or becomming a Github sponsor.

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=SR4F44J2UR8S4)

