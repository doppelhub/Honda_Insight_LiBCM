#define BATTSCI_DIR_PIN 2

VbattDiv2 = Vbatt>>1;

uint8_t BATTSCI_Packetx87[12] =
{
  0x87, /*Packet Type... never changes*/
  0x40, /*Never changes*/
 >0x00, /*Half the battery voltage (e.g. 0x40 = d64 = 128 V*/
  0x15, /*Battery SoC (upper byte)*/
  0x6F, /*Battery SoC (lower byte)*/
 >0x00, /*Battery Current (upper byte)*/
 >0x00, /*Battery Current (lower byte)*/
  0x32, /* */
  0x39, /* */
  0x39, /* */
 >0xmetsciState.b3, /* */
 >0xchecksum /* */
};

uint8_t BATTSCI_PacketxAA[12] =
{
  0xaa, /* */
  0x10, /* */
  0x00, /* */
  0x00, /* */
  0x00, /* */
  0x00, /* */
  0x40, /* */
  0x61, /* */
 >0x00, /*Battery Current (upper byte)*/
 >0x00, /*Battery Current (lower byte)*/
  0xmetsciState.b4, /* */
 >0xchecksum /* */
};
