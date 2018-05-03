/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <string.h>



#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	perror(s);
	abort();
}



void spi_transfer(int fd, const void* send_buffer, void* receive_buffer, size_t size)
{
	
	uint8_t tx[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x40, 0x00, 0x00, 0x00, 0x00, 0x95,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xAD,
		0xF0, 0x0D,
	};
	
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };
	
	
	struct spi_ioc_transfer tr;
	memset(&tr, sizeof(tr), 0);
	tr.tx_buf = (unsigned long)send_buffer;
	tr.rx_buf = (unsigned long)receive_buffer;
	tr.len = (unsigned int)size;

	//printf("tr.tx_buf: 0x%p\n", send_buffer);
	//printf("tr.rx_buf: 0x%p\n", receive_buffer);
	//printf("tr.tx_buf: %llu\n", tr.tx_buf);
	//printf("tr.rx_buf: %llu\n", tr.rx_buf);
	//printf("tr.len: %d\n", tr.len);

	int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret == 1)
	{
		pabort("can't send spi message");
	}

	for(size_t i=0; i<size; ++i)
	{
		printf("%.2X ", ((const uint8_t*)receive_buffer)[i]);	
	}
	printf("\n");
}


int open_spi_device()
{
	static const char *device = "/dev/spidev0.0";
	static uint8_t mode;
	static uint8_t bits = 8;
	static uint32_t speed = 10000;
	static uint16_t delay;
	
	int ret;
	
	int fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");
		
	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	
	return fd;
}


int main(int argc, char *argv[])
{
	int ret = 0;
	int fd = open_spi_device();


	uint8_t tx[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };

	for(;;)
	{
		spi_transfer(fd, tx, rx, ARRAY_SIZE(tx));
	}

	close(fd);

	return ret;
}
