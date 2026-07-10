# HariFun\_166\_Morphing\_Clock modified by SnowHead and Jruys

----------

**Update: Don't use this code** - the vibe coding AI broke multiple bits, amongst others the night mode. Working on a fix.

Forked from SnowHeads version which stopped working in 2026 because HTTP time request was sunset, back to NTP
This fork only works for ESP8266
Gemini helped with the fixes, it's not clean at all but at least it works again

----------

Read what this code primary was all about on [Instructable](https://www.instructables.com/id/Morphing-Digital-Clock/).

**CHANGES BY JRUYS**

- Changed back to NTP
- Updated configuration webpage
- Updated code to play nicely with latest ESP8266 board definitions and libraries (June 2026)

Known issues
- Changing clock color messes with time (it jumps forward/back a few seconds) - quick fix is to reset clock
- Nigtmode does not work properly in 12hr mode (it evaluates based on 12hr clock, so night kicks in twice per day)
- DST logic is for Europe only - sorry for being selfish but main goal was to get my own clock running again

What I really would like to do but isn't in this version
- Use native time.h and do away with the odd manual NTP code
- Add tickbox to suppress leading zero on hour display
- Fix color change second drift bug

**CHANGES BY SNOWHEAD**

- Changed from NTP to HTTP time request (via ESP8266HTTPClient library)
- No further mess around with timezones and DST settings. The default timezone "ip" (case sensitive!) will force the server to determine your location and DST state by analyzing your public IP. Only if you're connected via a foreign proxy to the internet you will have to set your local timezone regarding the list on [WTA](http://worldtimeapi.org/api/timezones). DST settings will always be detected automatically.
- Digits will not further morphed digit by digit but all in parallel.
- Easier selection of digit color.
- Patch for weak ESP8266-12E included
- Night-Mode, if "Start" or "End" hour are different from zero, the display will switch off between this both times
- permanent accessible Web-Interface for live change of the settings (military, timezone, color, fadingspeed, nightmode - settings take only effect after clicking button "Save"

Click photo for a quick demo.

original: [![Morphing Clock](https://img.youtube.com/vi/i0M6F4wRxGc/0.jpg)](https://www.youtube.com/watch?v=i0M6F4wRxGc)

modified: [![Morphing Clock](https://img.youtube.com/vi/GLg5dzmM7W4/0.jpg)](https://youtu.be/GLg5dzmM7W4)

Because the ESP8266 is not able to dimm the brightness by PWM (trying this will produce an ugly flicker) the ESP8266 can set the display only to off in nightmode, can in normal state not reduce the brightness and must use all colors in full brightness.

ESP8266:![MorphingClock](images/IF_8266.jpg)
