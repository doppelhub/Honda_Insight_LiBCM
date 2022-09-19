Starting condition:
Single 47 Ah cell charged to 4.2 volts at 5.5 amps.

Test:
Cell continuously discharged at 3.0 amps (C/15.7) down to 2.8 volts.
Cell voltage measured every second (the test data).

The results from this data are used to calculate SoC from resting Voc.

Note:
The raw data has several "0.00" volt readings, which occur when the received checksum doesn't match.  This data can be discarded.  There are way more data points than are required to draw useful conclusions.

I should rewrite the code to make it prevent writing errant data... but I'm not going to do that now.  Any non-zero value is accurate.