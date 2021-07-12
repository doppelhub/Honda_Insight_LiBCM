//main LiBCM include file

#ifndef grbl_h
#define grbl_h

// LiBCM versioning system
#define LiBCM_VERSION_FW "0.1a"
#define LiBCM_VERSION_DATE "20190825" //(YYYYMMDD)

//Standard libraries
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

//JTS2do-LiBCM: Rename per final names
//NOTE: Do not alter line order!
#include "config.h"
#include "nuts_bolts.h"
#include "settings.h"
#include "system.h"
#include "defaults.h"
#include "cpu_map.h"
#include "planner.h"
#include "eeprom.h"
#include "gcode.h"
#include "limits.h"
#include "motion_control.h"
#include "planner.h"
#include "print.h"
#include "probe.h"
#include "protocol.h"
#include "report.h"
#include "serial.h"
#include "spindle_control.h"
#include "stepper.h"
#include "jog.h"

// ---------------------------------------------------------------------------------------
// COMPILE-TIME ERROR CHECKING OF DEFINE VALUES:

#ifndef LIBCM_HW_REVB
  #error "LiBCM hardware revision not defined."
#endif

#endif
