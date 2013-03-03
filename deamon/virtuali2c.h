/* Header of the virtuali2c.
 *
 */

//Pin defination for the I2C
#define SDA	      2
#define SCL	      3

//I2C_DELAY_TIME
#define DELAY_TIME   10

#ifdef __cplusplus
extern "C" {
#endif

//virtualI2C function delaration
void VirtualI2CInit(void);
void I2CStart(void);
void I2CStop(void);
int CheckAck(void);
uint8_t I2CReadByte(void);
void I2CWriteByte(uint8_t data);
void I2CWriteBytes(uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif


