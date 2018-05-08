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
#include <signal.h> 
#include <sys/time.h>


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	perror(s);
	abort();
}



int spi_read_write(int fd, const void* buf_tx, void* buf_rx, size_t count)
{
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)buf_tx,
		.rx_buf = (unsigned long)buf_rx,
		.len = count,
		.delay_usecs = 0,
		.speed_hz = 0,
		.bits_per_word = 0,  
	};

	return ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
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



volatile sig_atomic_t stop = 0;
void signal_int(int sig)
{ // can be called asynchronously
	stop = 1; // set flag
}


float get_time()
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (float)(tv.tv_sec) + 0.000001 * tv.tv_usec;
}


int main(int argc, char *argv[])
{
	signal(SIGINT, signal_int); 
	
	int ret = 0;
	int fd = open_spi_device();


	uint8_t tx[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };

	float start_time = get_time();
	size_t bytes_transferred = 0;

	while(!stop)
	{
		spi_read_write(fd, tx, rx, ARRAY_SIZE(tx));
		bytes_transferred += ARRAY_SIZE(tx);
		
		for(size_t i=0; i<ARRAY_SIZE(tx); ++i)
		{
			printf("%.2X ", rx[i]);
		}
		printf("\n");
	}

	float end_time = get_time();
	float time = (end_time - start_time);
	printf("time: %.2fs\n", time);
	printf("transferrer: %.2f KB\n", bytes_transferred / 1024.0f);

	close(fd);

	return ret;
}
