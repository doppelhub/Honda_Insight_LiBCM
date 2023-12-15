Starting condition:
Single 49.5 Ah cell charged to 4.15 volts at 5.5 amps.

Test:
Cell continuously discharged at 3.0 amps (C/16.5) down to 2.8 volts.
Cell voltage measured every second (the test data).

The results from this data are used to calculate SoC from resting Voc.

Note:
The raw data has several "0.00" volt readings, which occur when the received checksum doesn't match.  This data can be discarded.  There are way more data points than are required to draw useful conclusions.

I should rewrite the code to make it prevent writing errant data... but I'm not going to do that now.  Any non-zero value is accurate. Code is identical to 47Ah (use that code). Couldn't get programmable load to work in macOS, so used old Windows 7 image on 2013MBP.

...

Since the point of this test was to see if the so-called "49.5 Ah" and "47 Ah" Samsung modules were identical, see the 47 Ah test results for a comparative analysis.  In short, both modules are identical.