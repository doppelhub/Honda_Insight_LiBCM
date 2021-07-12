//This file contains compile-time configuration parameters for LiBCM's internal system.
#define YES 1
#define NO 0


//Define CPU pin map
#define CPU_MAP_MEGA2560
#define LIBCM_HW_REVB

//USB Serial baud rate
#define BAUD_RATE_USB 115200

//Define realtime commands that are immediately picked off from the serial stream.
//These characters are not passsed to the serial parser, and are executed immediately.
#define CMD_RESET '|'
#define CMD_STATUS_REPORT

//Define battery parameters
#define STACK_CELLS_IN_SERIES 48
#define STACK_mAh 5000 //nominal pack size.
#define STACK_SoC_MIN 20 //minimum state of charge
#define STACK_SoC_MAX 80 //maximum state of charge
#define STACK_CURRENT_MAX_ASSIST 200 //disable assist above this value
#define STACK_CURRENT_MAX_REGEN 100 //disable regen above this value

//Configure when LiBCM turns off when the key is not on.
//LiBCM will turn off when ANY condition below occurs
#define KEYOFF_TURNOFF_HOURS 4       //LiBCM turns off after this much time, -1 to disable
#define KEYOFF_TURNOFF_BELOW_mV 3500 //LiBCM turns off when any cell drops below this value
#define KEYOFF_TURNOFF_BELOW_SoC 50  //LiBCM turns off when SoC drops below this value

//Configure fan behavior when key is off
#define KEYOFF_FANS_ALLOWED YES //'NO' to prevent fan usage when key is off
#define KEYOFF_FANS_MIN_SoC 60 //Fans are disabled below this SoC

//Configure fan temperature setpoints
//All temperatures are in Celcius
#define TEMP_FAN_LOW 30  //enable OEM fan at  low speed above this value
#define TEMP_OEMFAN_HIGH 40 //enable OEM fan at high speed above this value
#define TEMP_FAN_MIN 30 //enable onboard fans at lowest speed
#define TEMP_FAN_MAX 40 //enable onboard fans at highest speed

//Define which parameters are reported over the USB serial bus
#define USB_REPORT_ALLOWED YES	//if disabled, no data is reported
#define USB_REPORT_CELL_VOLTAGE_MAX YES
#define USB_REPORT_CELL_VOLTAGE_MIN YES
#define USB_REPORT_CELL_VOLTAGE_ALL YES
#define USB_REPORT_TEMP_MAX YES
#define USB_REPORT_TEMP_MIN YES
#define USB_REPORT_TEMP_ALL YES
#define USB_REPORT_KEY_STATE YES
#define USB_REPORT_METSCI_DATA YES
#define USB_REPORT_BATTSCI_DATA NO
#define USB_REPORT_CURRENT YES
#define USB_REPORT_PACK_VOLTAGE YES
#define USB_REPORT_PACK_POWER YES
#define USB_REPORT_FAN_STATUS YES
#define USB_REPORT_FANOEM_STATUS YES
#define USB_REPORT_SoC YES

//Grid charger behavior
#define GRID_CHARGE_ALLOWED YES
#define GRID_CHARGE_MAX_SoC 80 //grid charger disabled when SoC exceeds this percentage
#define GRID_CHARGE_MAX_mV 3900 //grid charger disabled when any cell exceeds this value
#define GRID_CHARGE_TEMP_MIN //grid charging disabled below this temperature
#define GRID_CHARGE_TEMP_MAX //grid charging disabled below this temperature
#define GRID_CHARGE_CURRENT_MAX_mA //specifies grid charger's maximum current output in mA

#define SPOOF_VOLTAGE_PERCENT 1.0 //actual voltage multiplied by this value
#define SPOOF_CURRENT_PERCENT 0.7 //actual current multiplied by this value

#define SERIAL_H_LINE_CONNECTED NO //H-Line wire connected to OEM BCM connector pin B01
#define SERIAL_I2C_CONNECTED YES //Serial display connected to SDA/SDL lines
#define SERIAL_HMI_CONNECTED NO //Nextion touch screen connected to J14

