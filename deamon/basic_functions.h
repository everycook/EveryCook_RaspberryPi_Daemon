/* Header of the basic_functions.
 *
 */

#define IND_KEY1	0 //14
#define IND_KEY2	1 //15
#define IND_KEY3	2 //18
#define IND_KEY4	3 //23

#define I2C_MOTOR				0
#define I2C_SERVO				1
#define I2C_7SEG_TOP			4	//SegAPin
#define I2C_7SEG_TOP_LEFT		3	//SegBPin
#define I2C_7SEG_TOP_RIGHT		5	//SegFPin
#define I2C_7SEG_CENTER			2	//SegGPin
#define I2C_7SEG_BOTTOM_LEFT	6	//SegEPin
#define I2C_7SEG_BOTTOM_RIGHT	8	//SegCPin
#define I2C_7SEG_BOTTOM			7	//SegDPin
#define I2C_7SEG_PERIOD			9	//SegDPPin

#define PI_PIN_BUZZER	4
#define BUZZER_PWM		1000

#define I2C_7SEG_ON				0
#define I2C_7SEG_OFF			4095

#define bool uint8_t
#define false 0
#define true 1
//#define arr_count(a) (sizeof(a) / sizeof(a[0]))

void setDebugEnabled(bool value);
void initHardware();
void setADCConfigReg(uint32_t newConfig[]);

//GPIO PCA9685 initialization
void GPIOInit(void);
void AD7794Init(void);
void PCA9685Init(void);

//read/write functions
uint32_t readADC(uint8_t i);
uint32_t readSignPin(uint8_t i);
uint32_t readRaspberryPin(uint8_t i);
void writeControllButtonPin(uint8_t i, uint8_t on);
void writeRaspberryPin(uint8_t i, uint8_t on);
void buzzer(uint8_t on, uint32_t pwm);
void writeI2CPin(uint8_t i, uint32_t value);
