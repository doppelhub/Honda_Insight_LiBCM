# Honda_Insight_LiBCM
Replacement BCM for G1 Honda Insight Lithium Conversion
Licensed under Creative Commons - Attribution - ShareAlike 3.0
Project created and maintained by insightcentral.net user 'mudder' (a.k.a. John Sullivan).

LiBCM is a replacement computer that allows the G1 Honda Insight (2000-2006) to safely use modern lithium traction batteries.

LiBCM replaces the OEM Battery Control Module (BCM).  LiBCM is presently offered as two different 'Kits':
-'5AhG3 LiBCM Kit' uses 12S & 18S lithium battery modules from various modern Honda hybrids, or;
-'47Ah LiBCM Kit' uses 12S Samsung SDI lithium battery modules

While other lithium battery types are theoretically supported, integrating them with LiBCM is beyond the design goals for this project.  A few customers are using non-supported modules, but it requires custom BMS harnesses and also cell discharge curve characterization to create a Voc->SoC lookup table.

LiBCM officially supports the following lithium configurations:
-'5AhG3' - 48S ONLY, or;
-'47Ah'- either 48S or 60S.

NO OTHER CONFIGURATION is officially supported.  While the PCB's onboard BMS supports anywhere from 6S to 60S, you're on your own (firmware-wise) if you want to run anything besides 48S.  Future firmware updates might allow 54S and/or 60S support. 

See linsight.org for more information.