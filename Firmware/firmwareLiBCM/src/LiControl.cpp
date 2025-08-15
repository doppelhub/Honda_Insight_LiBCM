//LiControl.cpp - Simplified SPI Communication between LiBCM (Master) and MuddersMIMA (Slave)
//
//Simple bidirectional data exchange:
//  - LiBCM sends: Motor power, OK-to-start flag, battery SoC, battery temperature
//  - LiControl responds: Heartbeat, motor RPM, vehicle speed, status flags
//  - 7-byte packets with simple checksum
//  - 100ms update rate

#include "libcm.h"
#include <SPI.h>

//==============================================================================
// Simplified Constants & Configuration
//==============================================================================

#define LICONTROL_PACKET_SIZE           7      // bytes per packet
#define LICONTROL_UPDATE_INTERVAL_MS   100    // Send data every 100ms
#define LICONTROL_SPI_CLOCK_HZ         250000  // 250kHz SPI clock

// Debug control - set to false to disable SPI debug output
#define LICONTROL_DEBUG_ENABLE         false   // Change to true to enable debug

// Packet header constants
#define LIBCM_PACKET_HEADER    0x55  // LiBCM packet identifier
#define LICONTROL_PACKET_HEADER 0xAA  // LiControl packet identifier

//==============================================================================
// Global Variables
//==============================================================================

static bool licontrol_connected = false;
static uint32_t last_update_time = 0;

// LiBCM data to send
static int16_t libcm_motor_power_watts = 0;
static uint8_t libcm_ok_to_start = 0;
static uint8_t libcm_battery_soc = 50;    // Start at 50%
static uint8_t libcm_battery_temp = 65;   // 25°C + 40 offset
static uint8_t libcm_mode_override = 0xFF; // 0xFF = no override, 0-2 = force specific mode

// LiControl data received  
static uint8_t licontrol_heartbeat = 0;
static uint16_t licontrol_motor_rpm = 0;
static uint8_t licontrol_vehicle_speed = 0;
static uint8_t licontrol_mode = 0;         // Current toggle switch position (0-2)
static uint8_t licontrol_status_flags = 0;

//==============================================================================
// Helper Functions
//==============================================================================

uint8_t LiControl_calculateChecksum(uint8_t* packet) {
    uint8_t sum = 0;
    for(int i = 0; i < LICONTROL_PACKET_SIZE - 1; i++) { // All bytes except checksum
        sum += packet[i];
    }
    return sum;
}

void LiControl_buildDataPacket(uint8_t* packet) {
    // Update LiBCM data from system
    libcm_motor_power_watts = LiControl_getInstantaneousPower();  // Use existing function
    libcm_ok_to_start = (SoC_getBatteryStateNow_percent() > 20) ? 1 : 0; // OK if SoC > 20%
    libcm_battery_soc = SoC_getBatteryStateNow_percent();
    libcm_battery_temp = temperature_battery_getLatest() + 40; // Add 40 offset
    
    // Mode override logic: only override if LiControl switch is in position 0
    // This allows LiBCM to take control when switch is in "auto" position
    // libcm_mode_override can be set by other LiBCM functions as needed
    
    // Build packet
    packet[0] = LIBCM_PACKET_HEADER;                           // 0x55
    packet[1] = (libcm_motor_power_watts >> 8) & 0xFF;        // Power high byte
    packet[2] = libcm_motor_power_watts & 0xFF;               // Power low byte  
    packet[3] = libcm_ok_to_start;                             // OK to start flag
    packet[4] = libcm_battery_soc;                             // SoC percentage
    packet[5] = libcm_battery_temp;                            // Temperature + 40
    packet[6] = LiControl_calculateChecksum(packet);          // Checksum
}

void LiControl_processResponsePacket(uint8_t* packet) {
    // Verify packet header
    if(packet[0] != LICONTROL_PACKET_HEADER) {
        licontrol_connected = false;
        return;
    }
    
    // Verify checksum
    uint8_t calculated_checksum = LiControl_calculateChecksum(packet);
    if(calculated_checksum != packet[6]) {
        licontrol_connected = false;
        return;
    }
    
    // Extract LiControl data
    licontrol_heartbeat = packet[1];
    licontrol_motor_rpm = (packet[2] << 8) | packet[3];  // Combine high and low bytes
    licontrol_vehicle_speed = packet[4];
    licontrol_mode = packet[5];                          // Toggle switch position (0-2)
    
    licontrol_connected = true; // Communication successful
}

bool LiControl_spiTransceive(uint8_t* tx_packet, uint8_t* rx_packet) {
    // Clear receive buffer
    memset(rx_packet, 0, LICONTROL_PACKET_SIZE);
    
    // Assert chip select with setup time
    digitalWrite(PIN_GPIO0_CS_MIMA, LOW);
    delayMicroseconds(200);
    
    // Configure SPI
    SPI.beginTransaction(SPISettings(LICONTROL_SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0));
    
    // Exchange data byte by byte
    for(int i = 0; i < LICONTROL_PACKET_SIZE; i++) {
        rx_packet[i] = SPI.transfer(tx_packet[i]);
        if(i < LICONTROL_PACKET_SIZE - 1) {
            delayMicroseconds(50); // Inter-byte delay
        }
    }
    
    SPI.endTransaction();
    
    // Deassert chip select
    delayMicroseconds(50);
    digitalWrite(PIN_GPIO0_CS_MIMA, HIGH);
    delayMicroseconds(200);
    
    return true;
}

//==============================================================================
// Public Interface Functions
//==============================================================================

void LiControl_begin(void)
{
    // Initialize CS pin
    pinMode(PIN_GPIO0_CS_MIMA, OUTPUT);
    digitalWrite(PIN_GPIO0_CS_MIMA, HIGH);
    
    // Initialize SPI
    SPI.begin();
    
    // Reset state
    licontrol_connected = false;
    last_update_time = 0;
    
    Serial.println(F("\nLiControl: Data stream initialized"));
}

void LiControl_handler(void)
{
    uint32_t current_time = millis();
    
    // Only update every 100ms
    if(current_time - last_update_time < LICONTROL_UPDATE_INTERVAL_MS) {
        return;
    }
    last_update_time = current_time;
    
    uint8_t tx_packet[LICONTROL_PACKET_SIZE];
    uint8_t rx_packet[LICONTROL_PACKET_SIZE];
    
    // Build data packet to send
    LiControl_buildDataPacket(tx_packet);
    
    // Exchange data
    if(LiControl_spiTransceive(tx_packet, rx_packet)) {
        // Process response first
        LiControl_processResponsePacket(rx_packet);
        
        // Only show debug if enabled
        if(LICONTROL_DEBUG_ENABLE) {
            Serial.println();
            Serial.print(F("LiControl TX: "));
            for(int i = 0; i < LICONTROL_PACKET_SIZE; i++) {
                Serial.print(tx_packet[i], HEX);
                Serial.print(F(" "));
            }
            Serial.print(F("RX: "));
            for(int i = 0; i < LICONTROL_PACKET_SIZE; i++) {
                Serial.print(rx_packet[i], HEX);
                Serial.print(F(" "));
            }
            
            if(licontrol_connected) {
                Serial.print(F("✓ HB:"));
                Serial.print(licontrol_heartbeat);
                Serial.print(F(" RPM:"));
                Serial.print(licontrol_motor_rpm);
                Serial.print(F(" Speed:"));
                Serial.print(licontrol_vehicle_speed);
                Serial.print(F("mph Mode:"));
                Serial.println(licontrol_mode);
            } else {
                Serial.println(F("✗ Bad response"));
            }
        }
    }
}

//==============================================================================
// Data Access Functions for LiBCM system to read LiControl data
//==============================================================================

bool LiControl_isConnected(void) {
    return licontrol_connected;
}

bool LiControl_isHealthy(void) {
    return licontrol_connected; // Simple health check
}

uint8_t LiControl_getHeartbeat(void) {
    return licontrol_heartbeat;
}

uint16_t LiControl_getEngineRPM(void) {
    return licontrol_motor_rpm;
}

uint8_t LiControl_getVehicleSpeed_mph(void) {
    return licontrol_vehicle_speed;
}

uint8_t LiControl_getMode(void) {
    return licontrol_mode; // Return current toggle switch position (0-2)
}

// Mode override functions - allows LiBCM to control LiControl's mode
void LiControl_setModeOverride(uint8_t mode) {
    if(mode <= 9) {
        libcm_mode_override = mode; // Set specific mode (0-9)
    } else {
        libcm_mode_override = 0xFF; // Clear override if invalid mode
    }
}

void LiControl_clearModeOverride(void) {
    libcm_mode_override = 0xFF; // Clear any mode override
}

uint8_t LiControl_getModeOverride(void) {
    return libcm_mode_override; // Return current mode override (0xFF = none)
}

void LiControl_disable(void) {
    licontrol_connected = false;
    Serial.println(F("LiControl: Disabled"));
}