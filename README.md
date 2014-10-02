cheali-charger-i1-DVF
DIRECT VOLTAGE FEEDBACK CONCEPT-CODE

FOR TESTING ONLY
----------------

SOME FUNCTIONS MAY NOT WORK (Resistance metering)


direct voltage feedback version of Pawel Staviki and Jozsef Nagy code

This fork is for experiments only! Voltage/current stabilization and e.t.c.
It is... hm.. BRUTALLY hacked cheali. Something is still alive and tries to work..
Output voltage corrects at every turn of SMPS_PID::update().  

By default cell's voltage controlled by slow getRealValue(). With balancer connected it can be jumpy. (Works smoother when current drops below ~110mA - why?)
In HardwareConfig.h you can turn on FAST_FEEDBACK (getADCValue), it is fast stable but inaccurate - it can stabilize the cell #2 (and m.b. #1) voltage at any value from 4 to 4.3 volts (depending of given hardware)

Charge/discharge current calibration did not work yet - you must use EEPROM generated with Jozsef fork cheali-charger-test1-master-0606...0726 or my https://github.com/Iggnus/cheali-charger-i1