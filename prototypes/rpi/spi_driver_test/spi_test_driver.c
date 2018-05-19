#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/dma-mapping.h>

#define DRVNAME "spi_dma"

static bool dma = false; /* use DMA? */

/* DMA buffer */
void *spi_tx_buf;
dma_addr_t spi_tx_dma;


static int spi_write_sync(struct spi_device *spi, const void *buf, size_t len)
{
	struct spi_transfer     t = {
					.tx_buf         = buf,
					.len            = len,
			};
	struct spi_message      m;
	int ret;

	printk("\n=> %s(len=%d), dma=%s\n", __func__, len, dma ? "true" : "false");

	spi_message_init(&m);

	if (dma) {
		t.tx_dma = spi_tx_dma;
		m.is_dma_mapped = 1;
	}

	spi_message_add_tail(&t, &m);
	ret = spi_sync(spi, &m);
	printk("<= %s() returned: %d\n", __func__, ret);
	return ret;
}

static int spi_dma_probe(struct spi_device *spi)
{
	dev_info(&spi->dev, "%s()\n", __func__);

	spi->dev.coherent_dma_mask = ~0;

	spi_tx_buf = dma_alloc_coherent(&spi->dev, 2*PAGE_SIZE, &spi_tx_dma, GFP_DMA);
	if (!spi_tx_buf) {
		return -ENOMEM;
	}

	dma = false;
	spi_write_sync(spi, spi_tx_buf, 1024);

	dma = true;
	spi_write_sync(spi, spi_tx_buf, 1024);

	return 0;
}

static int spi_dma_remove(struct spi_device *spi)
{
	dev_info(&spi->dev, "%s()\n", __func__);

	if (spi_tx_buf)
		dma_free_coherent(&spi->dev, PAGE_SIZE, spi_tx_buf, spi_tx_dma);

	return 0;
}


static struct spi_driver spi_dma_driver = {
	.driver = {
		.name	= DRVNAME,
		.owner	= THIS_MODULE,
	},
	.probe		= spi_dma_probe,
	.remove		= spi_dma_remove,
};

static void spidevices_delete(struct spi_master *master, unsigned cs)
{
	struct device *dev;
	char str[32];

	snprintf(str, sizeof(str), "%s.%u", dev_name(&master->dev), cs);

	dev = bus_find_device_by_name(&spi_bus_type, NULL, str);
	if (dev) {
		pr_err(DRVNAME": Deleting %s\n", str);
		device_del(dev);
	}
}

static struct spi_board_info spi_spi_dma = {
	.modalias = DRVNAME,
	.max_speed_hz = 10000,
	.bus_num = 0,
	.chip_select = 0,
	.mode = SPI_MODE_0,
};

struct spi_device *spi_dma_spi_device = NULL;

static int __init spi_dma_init(void)
{
	struct spi_master *master;

	pr_info("\n\n"DRVNAME": %s()\n", __func__);

	master = spi_busnum_to_master(spi_spi_dma.bus_num);
	if (!master) {
		pr_err(DRVNAME":  spi_busnum_to_master(%d) returned NULL\n", spi_spi_dma.bus_num);
		return -EINVAL;
	}

	spidevices_delete(master, spi_spi_dma.chip_select);      /* make sure it's available */

	spi_dma_spi_device = spi_new_device(master, &spi_spi_dma);
	put_device(&master->dev);
	if (!spi_dma_spi_device) {
		pr_err(DRVNAME":    spi_new_device() returned NULL\n");
		return -EPERM;
	}

	return spi_register_driver(&spi_dma_driver);
}

static void __exit spi_dma_exit(void)
{
	pr_info(DRVNAME": %s()\n", __func__);
	spi_unregister_driver(&spi_dma_driver);

	if (spi_dma_spi_device) {
		device_del(&spi_dma_spi_device->dev);
		kfree(spi_dma_spi_device);
	}
}

module_init(spi_dma_init);
module_exit(spi_dma_exit);


MODULE_LICENSE("GPL");