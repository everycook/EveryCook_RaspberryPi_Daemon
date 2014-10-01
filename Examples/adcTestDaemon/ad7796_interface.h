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

#define AD7796_DIRECTION_READ	1
#define AD7796_DIRECTION_WRITE	0

enum ad7796_registers {
#define AD7796_STATUS_NOTREADY_MASK	0x80
#define AD7796_STATUS_ERR_MASK		0x40
#define AD7796_STATUS_CHAN_MASK		0x03
	AD7796_STATUS  = 0,
	AD7796_MODE    = 1,
	AD7796_CONFIG  = 2,
	AD7796_DATA    = 3,
	AD7796_ID      = 4,
	AD7796_Reserved= 5,
	AD7796_OFFSET  = 6,
	AD7796_FULLSC  = 7
};

#define AD7796_MODE_MD_CONT_CONVERSION	(0<<4)
#define AD7796_MODE_MD_SINGLE			(1<<4)
#define AD7796_MODE_MD_IDLE				(2<<4)
#define AD7796_MODE_MD_PWRDOWN			(3<<4)
#define AD7796_MODE_MD_INT_ZERO			(4<<4)
#define AD7796_MODE_MD_RESERVED			(5<<4)
#define AD7796_MODE_MD_EXT_ZERO			(6<<4)
#define AD7796_MODE_MD_EXT_FULLS		(7<<4)
#define AD7796_MODE_AMP_CM				(1<<1)

#define AD7796_MODE_CLK_INT				(0<<6)
#define AD7796_MODE_CLK_INT_CLKOUT		(1<<6)
#define AD7796_MODE_CLK_EXT				(2<<6)
#define AD7796_MODE_CLK_EXT_DIV_2		(3<<6)
#define AD7796_MODE_FILTER_MASK			(0x0F)


#define internalZeroScaleCalibration	0x8000;
//not avauilable #define internalFullScaleCalibration	0xA000;
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


/** \brief Handles communication with the ad7796 chip. 
 **/
int ad7796_communicate(struct adc_private *adc, int reg, int dir, int len, uint8_t *buffer);
int ad7796_reset(struct adc_private *adc);
/** \brief Initializes SPI based ad7796 A/D converter
 * \param spi_bus_nr Number of SPI bus the device is connected to.
 * \param spi_cs_nr Number of SPI chip select line the device is connected to.
 * \return Zero on success, otherwise the negated error number encountered, conforming to errno.h
 **/
int ad7796_init(struct adc_private *adc, int spi_bus_nr, int spi_cs_nr);
/** Test wether a conversion has successfully been completed.
 * This is done by querying the status register. 
 * The datasheet says: [..] continuously convert with the RDY 
 * pin in the status register going low each time a conversion is complete.
 * 
 * Furthermore the /RDY bit in the STATUS register also gives an indication
 * if a conversion was finished. It is cleared whenever data is ready to be
 * polled from the AD.
 * \return <0 on error. 0 when not ready, 1 when ready
 */
int ad7796_check_if_ready(struct adc_private *adc);
/** Select a channel for A/D conversion 
  */
int ad7796_select_channel(struct adc_private *adc, int chan);
/** Select a channel for A/D conversion 
  */
int ad7796_select_channel2(struct adc_private *adc, uint8_t chan, uint32_t config);
/** @brief use the fonction AD7796_read_data_reg
 *  @return data
*/
uint32_t ad7796_read_data(struct adc_private *adc);
/** @brief read data
 *  @return data
*/
uint32_t ad7796_read_data_reg(struct adc_private *adc, uint32_t reg);
uint32_t ad7796_write_data(struct adc_private *adc, uint32_t reg, uint32_t data);
int ad7796_run_conversion(struct adc_private * adc);
