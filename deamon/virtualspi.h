/* Header of the virtualspi.
 *
 */

//Pin defination for the SPI
#define MOSI	10
#define MISO	9
#define SCLK	11
#define CS	8

//AD774 Register defination
#define READ_STATUS_REG		0x40
#define READ_MODE_REG		0x48
#define WRITE_MODE_REG		0x08
#define READ_STRUCT_REG		0x50
#define WRITE_STRUCT_REG	0x10
#define READ_DATA_REG		0x58
#define READ_SERIAL_REG		0x60
#define READ_IO_REG 		0x68
#define WRITE_IO_REG		0x28
#define READ_SHIFT_REG		0x70

#ifdef __cplusplus
extern "C" {
#endif

/*
extern void VirtualSPIInit(void);
extern void SPIReset(void);
extern void SPIWrite(uint8_t data);	
extern uint8_t SPIRead(void);
extern uint8_t SPIReadByte(uint8_t register);
extern uint32_t SPIRead2Bytes(uint8_t register);
extern uint32_t SPIRead3Bytes(uint8_t register);
*/

//virtualSPI function delaration
void VirtualSPIInit(void);
void SPIReset(void);
void SPIWrite(uint8_t data);	
uint8_t SPIRead(void);
void SPIWriteByte(uint8_t reg, uint8_t data);
void SPIWrite2Bytes(uint8_t reg, uint32_t data);
uint8_t SPIReadByte(uint8_t register);
uint32_t SPIRead2Bytes(uint8_t register);
uint32_t SPIRead3Bytes(uint8_t register);

#ifdef __cplusplus
}
#endif