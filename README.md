# Honda_Insight_LiBCM
 Replacement BCM for G1 Honda Insight Lithium Conversion

Licensed under Creative Commons - Attribution - ShareAlike 3.0

This is the central repository for the LiBCM Project, created by insightcentral.net user 'mudder' (a.k.a. John Sullivan).  

LiBCM replaces the OEM Battery Control Module (BCM) in G1 (2000-2006) Honda Insight vehicles.  LiBCM is designed specifically to allow the G1 Insight to use 12S & 18S lithium battery modules from 2018-2020 Honda Accord Hybrids and/or 2019+ Honda Insights.  While other lithium batteries are theoretically supported, that is beyond the design goals of this project (and requires manually wiring each series cell's sense lead to the LiBCM PCB).

LiBCM officially supports either 42S or 48S lithium configurations.  NO OTHER CONFIGURATION is officially supported.  While the PCB's onboard BMS supports anywhere from 6S to 60S - and also safely supports up to 84S using external modules (via an isoSPI interface), you're on your own (firmware-wise) if you want to run anything beyond 42S or 48S.