#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/errno.h>
#include <assert.h>
#include <stdio.h>

struct adc_private {
	int fd;
	/* Currently selected input channel */
	int selected_input;
};

#define AD7794_DIRECTION_READ	1
#define AD7794_DIRECTION_WRITE	0

enum ad7794_registers {
#define AD7794_STATUS_NOTREADY_MASK	0x80
#define AD7794_STATUS_ERR_MASK		0x40
#define AD7794_STATUS_NOXREF_MASK	0x20
#define AD7794_STATUS_SR3_MASK		0x80
#define AD7794_STATUS_CHAN_MASK		0x03
	AD7794_STATUS  = 0,
	AD7794_MODE    = 1,
	AD7794_CONFIG  = 2,
	AD7794_DATA    = 3,
	AD7794_ID      = 4,
	AD7794_IO      = 5,
	AD7794_OFFSET  = 6,
	AD7794_FULLSC  = 7
};

#define AD7794_MODE_MD_CONT_CONVERSION	(0<<4)
#define AD7794_MODE_MD_SINGLE			(1<<4)
#define AD7794_MODE_MD_IDLE				(2<<4)
#define AD7794_MODE_MD_PWRDOWN			(3<<4)
#define AD7794_MODE_MD_INT_ZERO			(4<<4)
#define AD7794_MODE_MD_INT_FULLS		(5<<4)
#define AD7794_MODE_MD_EXT_ZERO			(6<<4)
#define AD7794_MODE_MD_EXT_FULLS		(7<<4)
#define AD7794_MODE_AMP_CM				(1<<1)

#define AD7794_MODE_CLK_INT				(0<<6)
#define AD7794_MODE_CLK_INT_CLKOUT		(1<<6)
#define AD7794_MODE_CLK_EXT				(2<<6)
#define AD7794_MODE_CLK_EXT_DIV_2		(3<<6)
#define AD7794_MODE_FILTER_MASK			(0x0F)


#define internalZeroScaleCalibration	0x8000;
#define internalFullScaleCalibration	0xA000;
#define systemZeroScaleCalibration		0xC000;
#define systemFullScaleCalibration		0xE000;

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/errno.h>
#include <assert.h>
#include <stdio.h>



int ad7794_communicate(struct adc_private *adc, int reg, int dir, int len, uint8_t *buffer);
int ad7794_reset(struct adc_private *adc);
int ad7794_init(struct adc_private *adc, int spi_bus_nr, int spi_cs_nr);
int ad7794_check_if_ready(struct adc_private *adc);
int ad7794_select_channel(struct adc_private *adc, int chan);
int ad7794_select_channel2(struct adc_private *adc, uint8_t chan, uint32_t config);
uint32_t ad7794_read_data(struct adc_private *adc);
uint32_t ad7794_read_data_reg(struct adc_private *adc, uint32_t reg);
uint32_t ad7794_write_data(struct adc_private *adc, uint32_t reg, uint32_t data);
int ad7794_run_conversion(struct adc_private * adc);