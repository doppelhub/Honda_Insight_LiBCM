/*
  Library created in February 2021
  by Arnaud Ouvrier (http://www.arnaudouvrier.fr)

  This is free and unencumbered software released into the public domain.
  For more information, see http://unlicense.org/ or
  the accompanying (un)LICENSE file

*/

#include "Arduino.h"
#include "Wire.h"
#include "lcd_I2C.h"
#include "libcm.h"

lcd_I2C_jts::lcd_I2C_jts(uint8_t address) {
  _i2cLcdAddress = address;
}

void lcd_I2C_jts::setRowOffsets(int row1, int row2, int row3, int row4) {
  _rowOffsets[0] = row1;
  _rowOffsets[1] = row2;
  _rowOffsets[2] = row3;
  _rowOffsets[3] = row4;
}

////////////////////////////////////////////////////////////////////////
//Core commands used to send data over I2C bus

//command to send one byte to the LCD display (containing DATA_PORTION nibble & CTRL_PORTION nibble)
void lcd_I2C_jts::send(uint8_t byte)
{
  Wire.beginTransmission(_i2cLcdAddress);                      //~t=5   microseconds
  Wire.write(byte);                                            //~t=8   microseconds
  Wire.endTransmission(false); //Wire transmits all bytes in buffer // t=240 microseconds //JTS2do: Send 'stop' (false) or 'restart' (true)
}

// Merge the command quartet with the control command (BL EN RW RS)
void lcd_I2C_jts::sendQuartet(uint8_t data)
{
  data |= _ctrlRegister;
  
  //send(data); //Not necessary, per HD44780U timing diagram (page 22)

  send(data | EN_BIT); // set EN ('E') line high.  Note: HD44780U latches nibble when 'E' line goes low.
  delayMicroseconds(1); //HD44780U requires 400 ns delay
  send(data); //set EN ('E') line back low.  This latches data nibble into HD44780U's buffer 
  
  delayMicroseconds(40); //This delay accounts for less than 1% of the overall display update time
}

// Take a command byte and split it in two quartets (LCD operates in 4 bit mode)
// This is the primary method used to send data to display
void lcd_I2C_jts::sendCmd(uint8_t data) //t=1 milliseconds
{
  digitalWrite(PIN_LED2,HIGH); //temp

  sendQuartet(data & DATA_PORTION);

  digitalWrite(PIN_LED2,LOW); //temp

  //JTS: Can delay as long as desired (i.e. split interrupt handler here if desired)

  sendQuartet((data << 4) & DATA_PORTION);
}

////////////////////////////////////////////////////////////////////////

// Set the state of a bit in the Control register
void lcd_I2C_jts::setCtrlRegisterBit(uint8_t bit, bool state) {
  if(state) _ctrlRegister |= bit;
  else _ctrlRegister &= ~bit;
}

void lcd_I2C_jts::setDsplRegisterBit(uint8_t bit, bool state) {
  if(state) _dsplRegister |= bit;
  else _dsplRegister &= ~bit;
}

void lcd_I2C_jts::setEntryModeBit(uint8_t bit, bool state) {
  if(state) _modeRegister |= bit;
  else _modeRegister &= ~bit;
}

// write a byte to the I2C bus
size_t lcd_I2C_jts::write(uint8_t byte) {
  _ctrlRegister |= RS_BIT; // Set register to DATA
  sendCmd(byte);
  _ctrlRegister &= ~RS_BIT; // Reset register to INSTRUCTION

  return 1;
}

// Initialization routine to set the LCD to 4 bit mode
void lcd_I2C_jts::initializationRoutine() {
  // Init in 8-bit
  // LiquidCrystal was outputing 0x30 4500µs - 0x30 4500µs 0x30 150µs
  // The datasheet for the HD44780  says 0x30 4100µs - 0x30 100µs 0x30 no delay...
  // (HD44780U datasheet, page 45)
  // It also may be optional, useful only when:
  //"the power supply conditions for correctly operating the internal reset circuit are not met"
  sendQuartet(LCD_FUNCTIONSET | LCD_FUNCTIONSET_DL_BIT);
  delayMicroseconds(4200);
  sendQuartet(LCD_FUNCTIONSET | LCD_FUNCTIONSET_DL_BIT); 
  delayMicroseconds(110);
  sendQuartet(LCD_FUNCTIONSET | LCD_FUNCTIONSET_DL_BIT); 

  // set in 4-bit mode (Function set)
  sendQuartet(LCD_FUNCTIONSET);
}

void lcd_I2C_jts::setBacklight(bool state) {
  setCtrlRegisterBit(BL_BIT, state);
  send(_ctrlRegister);
}

// set the function register
// bytemode = 0 -> 4-bit mode; twoLines = 1 -> 2 lines; font = 0 -> 5x8 dots, 1 = 5x10
void lcd_I2C_jts::setFctnRegister(uint8_t bytemode, uint8_t twoLines, uint8_t font) {
  _fctnRegister = 0 | (bytemode << 4) | (twoLines << 3) | (font << 2);
  sendCmd(LCD_FUNCTIONSET | _fctnRegister);
}

void lcd_I2C_jts::setDsplControl(uint8_t display, uint8_t cursor, uint8_t blink) {
  _dsplRegister = 0 | (display << 2) | (cursor << 1) | blink;
  sendCmd(LCD_DISPLAYCONTROL | _dsplRegister);
}

void lcd_I2C_jts::setEntryMode(uint8_t increment, uint8_t shift) {
  _modeRegister = 0 | (increment << 1) | shift;
  sendCmd(LCD_ENTRYMODESET | _modeRegister);
}

void lcd_I2C_jts::setCursor(uint8_t col, uint8_t row) {
  if ((row >= _rows) | (row >= 4)) {
    row = _rows - 1; 
  }
	sendCmd(LCD_SETDDRAMADDR | (col + _rowOffsets[row]));
}

// Clear the display
void lcd_I2C_jts::clear() {
  sendCmd(LCD_CLEARDISPLAY);
  delay(2);
}

// Set cursor to 0;0 position
void lcd_I2C_jts::home() {
  sendCmd(LCD_RETURNHOME);
  delay(2);
}

// Setting up and initializig the LCD 
void lcd_I2C_jts::begin(uint8_t cols, uint8_t rows, uint8_t font) {
  Wire.begin();
  Wire.setWireTimeout(1000, true); //waits up to 1000 microseconds
  _cols = cols;
  _rows = rows;
  _font = font;

  // set rows start address
  // On a 16x2:
  //  line 1, screen 1 [0x00 ; 0x0F]
  //  line 2, screen 1 [0x40 ; 0x4F]
  //  line 1, screen 2 [0x10 ; 0x1F]
  //  line 2, screen 2 [0x50 ; 0x5F]
  // On a 20x4 (only one screen, HD44780 can only store 80 characters):
  // (Remark: on a 20x4, incrementing the cursor col give this result:
  //   Line 1 -> Line 3 -> Line 2 -> Line 4)
  //  line 1, screen 1 [0x00 ; 0x13]
  //  line 2, screen 1 [0x40 ; 0x53]
  //  line 3, screen 1 [0x14 ; 0x27]
  //  line 4, screen 1 [0x54 ; 0x67]
  setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);
  
  delay(1000); // LCD power up time //JTS2do: Reduce

  send(0x00); // clear data line
  initializationRoutine();

  // first params(byte) = 0 -> Always 4bit
  setFctnRegister(0, (rows != 1), font);

  clear();
  setDsplControl(1, 0, 0);
  setEntryMode(1, 0);
  home();
}

void lcd_I2C_jts::createChar(uint8_t index, uint8_t character[]) {
    index &= 0x7; // 7 editable characters

    sendCmd(LCD_SETCGRAMADDR | (index << 3));
    _ctrlRegister |= RS_BIT; // Set register to DATA
    for (uint8_t i=0; i<8; i++) {
      sendCmd(character[i]);
    }
    _ctrlRegister &= ~RS_BIT; // Reset register to INSTRUCTION
}

void lcd_I2C_jts::selectScreen(uint8_t index) {
  home();

  for (uint8_t i = 0; i < index * _cols; i++) {
    sendCmd(LCD_CURSORSHIFT | LCD_CURSORSHIFT_SC_BIT);
  }
}

void lcd_I2C_jts::backlight() {
  setBacklight(true);
}

void lcd_I2C_jts::noBacklight() {
  setBacklight(false);
}

void lcd_I2C_jts::display() {
  setDsplRegisterBit(LCD_DISPLAYCONTROL_D_BIT, true);
  sendCmd(LCD_DISPLAYCONTROL | _dsplRegister);
}

void lcd_I2C_jts::noDisplay() {
  setDsplRegisterBit(LCD_DISPLAYCONTROL_D_BIT, false);
  sendCmd(LCD_DISPLAYCONTROL | _dsplRegister);
}

void lcd_I2C_jts::blink(){
  setDsplRegisterBit(LCD_DISPLAYCONTROL_B_BIT, true);
  sendCmd(LCD_DISPLAYCONTROL | _dsplRegister);
}
    
void lcd_I2C_jts::noBlink() {
  setDsplRegisterBit(LCD_DISPLAYCONTROL_B_BIT, false);
  sendCmd(LCD_DISPLAYCONTROL | _dsplRegister);
}

void lcd_I2C_jts::cursor() {
  setDsplRegisterBit(LCD_DISPLAYCONTROL_C_BIT, true);
  sendCmd(LCD_DISPLAYCONTROL | _dsplRegister);
}

void lcd_I2C_jts::noCursor() {
  setDsplRegisterBit(LCD_DISPLAYCONTROL_C_BIT, false);
  sendCmd(LCD_DISPLAYCONTROL | _dsplRegister);
}

void lcd_I2C_jts::leftToRight() {
  setEntryModeBit(LCD_ENTRYMODESET_ID_BIT, true);
  sendCmd(LCD_ENTRYMODESET | _modeRegister);
}

// This is for text that flows Right to Left
void lcd_I2C_jts::rightToLeft() {
  setEntryModeBit(LCD_ENTRYMODESET_ID_BIT, false);
  sendCmd(LCD_ENTRYMODESET | _modeRegister);
}

void lcd_I2C_jts::autoscroll(void) {
  setEntryModeBit(LCD_ENTRYMODESET_S_BIT, true);
  sendCmd(LCD_ENTRYMODESET | _modeRegister);
}

// This will 'left justify' text from the cursor
void lcd_I2C_jts::noAutoscroll(void) {
  setEntryModeBit(LCD_ENTRYMODESET_S_BIT, false);
  sendCmd(LCD_ENTRYMODESET | _modeRegister);
}

void lcd_I2C_jts::scrollDisplayLeft(void) {
  sendCmd(LCD_CURSORSHIFT | LCD_CURSORSHIFT_SC_BIT);
}

void lcd_I2C_jts::scrollDisplayRight(void) {
  sendCmd(LCD_CURSORSHIFT | LCD_CURSORSHIFT_RL_BIT | LCD_CURSORSHIFT_SC_BIT);
}

void lcd_I2C_jts::command(uint8_t value) {
  sendCmd(value);
}