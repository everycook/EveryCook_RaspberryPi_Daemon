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

/** \brief Handles communication with the AD7794 chip. 
 **/
int ad7794_communicate(struct adc_private *adc, int reg, int dir, int len, char *buffer)
{
	struct spi_ioc_transfer transfer;
	int r;
	uint8_t *tx_buf=NULL, *rx_buf=NULL;

	memset(&transfer, 0, sizeof(transfer));
	assert( (reg>=0) && (reg <= 7) );
	assert( (dir == 0) || (dir==1) );
	assert( len > 0 );

	/* The TX buffer contains at least one byte preable of the transmission 
	 * addressing the destination register and so on. See manual of AD7794 
	 * for definition of communication register.
	 */
	transfer.len = len+1;

	tx_buf = calloc(transfer.len, 1);
	if (tx_buf == NULL)
		return(-errno);

	if (dir == AD7794_DIRECTION_READ) 
	{
		rx_buf = calloc(transfer.len, 1);
		if (rx_buf == NULL) 
			return(-errno);
	} else
	{
		memcpy(&tx_buf[1], buffer, len);
	}
	transfer.tx_buf = (uint32_t)tx_buf;
	transfer.rx_buf = (uint32_t)rx_buf;

	/* Build communication register word.
	 * As a reminder:
	 * Bit 7 | Bit 6 | Bit 5| Bit 4| Bit 3| Bit 2 | Bit 1 | Bit 0
	 * _WEN    R/_W    RS2    RS1    RS0    CREAD     0       0
	 *   0      dir    \______reg_______/     0       0       0
	 **/
	tx_buf[0] = (dir << 6) | (reg<<3); 

	/* Initiate one SPI transfer via ioctl call */
	r = ioctl(adc->fd, SPI_IOC_MESSAGE(1), &transfer);

	if (rx_buf)
	{
		memcpy(buffer, &rx_buf[1], len); 
	}

cleanup:
	free(tx_buf);
	if (rx_buf)
		free(rx_buf);

}

/** \brief Resets the AD7794 chip via SPI.
 */
int ad7794_reset(struct adc_private *adc)
{
	char tmp[8];
	int len;
	/* The datasheet says on page 17:
	 * In situations where the interface sequence is lost, a write operation of 
	 * at least 32 serial clock cycles with DIN high returns the ADC to this 
	 * default state by resetting the entire part. 
	 *
	 * So write 4 bytes = 32 bit = 32 cycles to the device.
	 */
	memset(tmp, 0xFF, sizeof(tmp));
	len=write(adc->fd, tmp, sizeof(tmp));
	if (len!=sizeof(tmp)) 
		return(errno);
	else
	{
		usleep(500);
		return(0);
	}
}

/** \brief Initializes SPI based AD7794 A/D converter
 * \param spi_bus_nr Number of SPI bus the device is connected to.
 * \param spi_cs_nr Number of SPI chip select line the device is connected to.
 * \return Zero on success, otherwise the negated error number encountered, conforming to errno.h
 **/
int ad7794_init(struct adc_private *adc, int spi_bus_nr, int spi_cs_nr)
{
	char fn_tmp[128];
	int fd;
	int mode=3;
	//int mode=0;
	uint32_t speed=100000;

	assert(adc!=NULL);
	memset(adc, 0, sizeof(*adc));

	snprintf(fn_tmp, sizeof(fn_tmp)-1, "/dev/spidev%d.%d", spi_bus_nr, spi_cs_nr);
	adc->fd = open(fn_tmp, O_EXCL|O_RDWR);
	if (adc->fd<0) {
		return(errno);
	}

	ioctl(adc->fd, SPI_IOC_WR_MODE, &mode);
	ioctl(adc->fd, SPI_IOC_RD_MODE, &mode);
	ioctl(adc->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	ioctl(adc->fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);

	ad7794_reset(adc);
	return(0);
}

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
int ad7794_check_if_ready(struct adc_private *adc)
{
	char status=0;
	int r;
	r=ad7794_communicate(adc, AD7794_STATUS, AD7794_DIRECTION_READ, 1, &status);
	if (r>0)
	{ 
		printf("%s: status=%02x\n", __func__, status);
		if (status & AD7794_STATUS_NOTREADY_MASK) 
			return(0);
		else
			return(1);
	}
	printf("%s: r=%d status=%d\n", __func__, r, status);
	return(r);
}


/** Select a channel for A/D conversion 
  */
int ad7794_select_channel(struct adc_private *adc, int chan)
{
	char config[2];
	int r=0;
	
	r = ad7794_communicate(adc, AD7794_CONFIG, AD7794_DIRECTION_READ, 2, config);
	printf("%s: read { %02x, %02x } \n", __func__, config[0], config[1]);
//	config[0] = 0x00;
	config[1] |= config[1]&(~0x6) | (0x6 & chan);
	r = ad7794_communicate(adc, AD7794_CONFIG, AD7794_DIRECTION_WRITE, 2, config);
	printf("%s: wrote { %02x, %02x } \n", __func__, config[0], config[1]);
	r = ad7794_communicate(adc, AD7794_CONFIG, AD7794_DIRECTION_READ, 2, config);
	printf("%s: after writing, read back { %02x, %02x } \n", __func__, config[0], config[1]);
}

int ad7794_read_data(struct adc_private *adc)
{
	char data[3];
	int r;

	r=ad7794_communicate(adc, AD7794_DATA, AD7794_DIRECTION_READ, 3, data);
	if (r>0)
	{
		r  = data[0];
		r += data[1]<<8;
		r += data[2]<<16;
	}
	return(r);
}

/** Run A/D conversion and wait for result
  */
int ad7794_run_conversion(struct adc_private * adc)
{
	char mode[2];
#define AD7794_MODE_MD_CONT_CONVERSION	(0<<4)
#define AD7794_MODE_MD_SINGLE		(1<<4)
#define AD7794_MODE_MD_IDLE		(2<<4)
#define AD7794_MODE_MD_PWRDOWN		(3<<4)
#define AD7794_MODE_MD_INT_ZERO		(4<<4)
#define AD7794_MODE_MD_INT_FULLS	(5<<4)
#define AD7794_MODE_MD_EXT_ZERO		(6<<4)
#define AD7794_MODE_MD_EXT_FULLS	(7<<4)
#define AD7794_MODE_AMP_CM		(1<<1)

#define AD7794_MODE_CLK_INT		(0<<6)
#define AD7794_MODE_CLK_INT_CLKOUT	(1<<6)
#define AD7794_MODE_CLK_EXT		(2<<6)
#define AD7794_MODE_CLK_EXT_DIV_2	(3<<6)
#define AD7794_MODE_FILTER_MASK		(0x0F)
//	mode[0] = AD7794_MODE_MD_CONT_CONVERSION;
//	mode[1] = 0xA;
//	ad7794_communicate(adc, AD7794_STATUS, AD7794_DIRECTION_WRITE, 2, &mode);
	while (ad7794_check_if_ready(adc)>0)
	 	{
		}
	return(ad7794_read_data(adc));
}

int main()
{
	struct adc_private adc;
	int r,i;
	uint8_t value;
	if (r=ad7794_init(&adc, 0, 0))
	{
		printf("Could not init AD7794... errno=%d\n",r);
		exit(r);
	}
	ad7794_reset(&adc);
	while (!ad7794_check_if_ready(&adc))
	 {
	 }
	printf("selecting channel 6\n");
	ad7794_select_channel(&adc, 6);
	printf("Data = %06x\n",ad7794_read_data(&adc));
	printf("Data = %06x\n",ad7794_run_conversion(&adc));
	printf("selecting channel 2\n");
	ad7794_select_channel(&adc, 2);
	printf("Data = %06x\n",ad7794_run_conversion(&adc));

	exit(0);
	for (i=0;i<7;i++) 
	{ 
		ad7794_communicate(&adc, i, AD7794_DIRECTION_READ, 1, &value);
		printf("i=%d value=%d\n", i, &value);
	}
	ad7794_check_if_ready(&adc);
	
}
