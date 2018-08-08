#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>

#include <asm/io.h>
#include <asm-arm/arch-at91/board.h>
#include <asm/gpio.h>
//#include <asm/cpu.h>
#include "ade7858.h"
#include "atmel_spi.h"

static struct timer_list hsdc_timeout;
extern struct ade7858_state *ade7858_dev;
static void hsdc_receive_timeout(unsigned long data)
{
	struct hsdc *hsdc = (struct hsdc *)data;
	if(hsdc)
	{
		spi_writel(hsdc, IDR, (SPI_BIT(RXBUFF) | SPI_BIT(ENDRX)
                                     | SPI_BIT(OVRES)));
        	spi_writel(hsdc, PTCR, SPI_BIT(RXTDIS) | SPI_BIT(TXTDIS));
        	spi_writel(hsdc, RCR, 0);
        	spi_writel(hsdc, TCR, 0);
        	while (spi_readl(hsdc, SR) & SPI_BIT(RDRF))
                        spi_readl(hsdc, RDR);
        	spi_readl(hsdc, SR);
        	spi_writel(hsdc, IDR, SPI_BIT(RXBUFF) | SPI_BIT(ENDRX) | SPI_BIT(OVRES));
	}
	printk("timeout\n");
	wake_up_interruptible(&(hsdc->hsdc_wait));
}
static int atmel_hsdc_receive(struct hsdc *hsdc,char *buf,int len)
{
	struct device   *dev = &hsdc->pdev->dev;
	u32	ieval;
	u16	val1;
	
	u32 ptcr_val;
	dma_addr_t rx_dma = dma_map_single(dev,buf,len,DMA_FROM_DEVICE);
	if(dma_mapping_error(rx_dma))
	{
		return -ENOMEM;
	}
	hsdc->len = len;
	hsdc->receive_bytes = 0;
	
	spi_writel(hsdc, CR, SPI_BIT(SWRST));
  spi_writel(hsdc, CR, SPI_BIT(SWRST));
	spi_writel(hsdc, MR, SPI_BIT(MODFDIS));
	spi_writel(hsdc, CR, SPI_BIT(SPIEN));
  spi_writel(hsdc, CSR0,SPI_BIT(CPOL));
	spi_writel(hsdc,RPR,rx_dma);
	spi_writel(hsdc,TPR,rx_dma);
	spi_writel(hsdc,RCR,len);	
	spi_writel(hsdc,TCR,0);
	
	ieval = SPI_BIT(RXBUFF) | SPI_BIT(ENDRX) | SPI_BIT(OVRES);
	spi_writel(hsdc, IER, ieval);
  spi_writel(hsdc, PTCR, SPI_BIT(RXTEN) | SPI_BIT(RXTEN));
  
  ptcr_val=spi_readl(hsdc, PTCR);
  
// printk("ptcr_val=0x%x\n",ptcr_val); 
  
	ade7858_dev->read_reg_16(ade7858_dev->dev,ADE7858_CONFIG,&val1);
        val1 |= 1<<6; /*HSDCEN*/
        ade7858_dev->write_reg_16(ade7858_dev->dev,ADE7858_CONFIG,val1);
//	printk("interrupt sleep\n");
	hsdc_timeout.expires = jiffies + 10; //1s
	add_timer(&hsdc_timeout);
	interruptible_sleep_on(&(hsdc->hsdc_wait));
	val1 &= ~(1<<6);
        ade7858_dev->write_reg_16(ade7858_dev->dev,ADE7858_CONFIG,val1);
	//printk("interrupt wakeup\n");
	dma_unmap_single(dev,rx_dma,len,DMA_FROM_DEVICE);
	return 0;
	
}


static irqreturn_t atmel_hsdc_interrupt(int irq, void *dev_id)
{
	struct hsdc *hsdc = dev_id;
	u32	status, pending, imr;
	int	ret = IRQ_NONE;
	del_timer(&hsdc_timeout);
	spin_lock(&hsdc->lock);
	imr = spi_readl(hsdc, IMR);
	status = spi_readl(hsdc, SR);
	pending = status & imr;
	
//	printk("atmel_hsdc_interrupt start imr %x status %x pending %x\n",imr,status,pending);
	spi_writel(hsdc, IDR, (SPI_BIT(RXBUFF) | SPI_BIT(ENDRX) | SPI_BIT(OVRES)));
  spi_writel(hsdc, PTCR, SPI_BIT(RXTDIS) | SPI_BIT(TXTDIS));
	spi_writel(hsdc, RCR, 0);
  spi_writel(hsdc, TCR, 0);
  
	
	while (spi_readl(hsdc, SR) & SPI_BIT(RDRF))
                        spi_readl(hsdc, RDR);
	
	spi_readl(hsdc, SR);
	if (pending & SPI_BIT(OVRES))
	{
		printk("pending %x SPI_BIT(OVRES) %x\n",pending,SPI_BIT(OVRES));
		hsdc->receive_bytes = 0;
	}
	if (pending & (SPI_BIT(RXBUFF) | SPI_BIT(ENDRX)))
	{
	 // printk("RX END\n");
		hsdc->receive_bytes = hsdc->len;
		
	}
	spin_unlock(&hsdc->lock);
	wake_up_interruptible(&(hsdc->hsdc_wait));
	return ret;
}
static int __init atmel_hsdc_probe(struct platform_device *pdev)
{
	struct resource         *regs;
  int                     irq;
	int                     ret;
	struct hsdc		          *slave;
	struct clk              *clk;
	
	clk = clk_get(&pdev->dev, "spi_clk");
        if (IS_ERR(clk))
                return PTR_ERR(clk);
 //#define AT91SAM9260_BASE_SPI0		0xfffc8000
//#define AT91SAM9260_BASE_SPI1		0xfffcc000    
           
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	
	if (!regs)
                return -ENXIO;
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
                return irq;
                
  printk("AT91SAM9260_BASE_SPI1==0xfffcc000,get val=0x%x,irq=%d\n",regs->start,irq);              
	slave = kzalloc(sizeof *slave, GFP_KERNEL);
	if(!slave)
	{
		ret =  -ENOMEM;
                return ret;
	}
	slave->transfer = atmel_hsdc_receive;
	
	spin_lock_init(&slave->lock);
	INIT_LIST_HEAD(&slave->queue);
	slave->pdev = pdev;
//	slave->regs = ioremap(regs->start, resource_size(regs));
	slave->regs = ioremap(regs->start, regs->end-regs->start+1);
	
	if (!slave->regs)
                goto out_free_buffer;
	init_waitqueue_head(&(slave->hsdc_wait));
	slave->irq = irq;
/*	ret = request_irq(irq, atmel_hsdc_interrupt, 0,
                        dev_name(&pdev->dev), slave);
*/
	ret = request_irq(irq, atmel_hsdc_interrupt, 0,
                             "hsdc", slave);
	clk_enable(clk);
	
	
	spi_writel(slave, CR, SPI_BIT(SWRST));
  spi_writel(slave, CR, SPI_BIT(SWRST));
  spi_writel(slave, MR, SPI_BIT(MODFDIS));
  
  spi_writel(slave, PTCR, SPI_BIT(RXTDIS) | SPI_BIT(TXTDIS));
  spi_writel(slave, CR, SPI_BIT(SPIEN));
  
  
	spi_writel(slave, CSR0,0);
	init_timer(&hsdc_timeout);
	hsdc_timeout.function = hsdc_receive_timeout;
	hsdc_timeout.data = slave;
	ret = ade7858_hsdcreg(slave);
	return 0;
out_free_buffer:
	kfree(slave);
	return ret;
}

static struct platform_driver atmel_hsdc_driver = {
        .driver         = {
                .name   = "atmel_hsdc",
                .owner  = THIS_MODULE,
        },
};

static int __init atmel_hsdc_init(void)
{
        return platform_driver_probe(&atmel_hsdc_driver, atmel_hsdc_probe);
}
module_init(atmel_hsdc_init);

static void __exit atmel_hsdc_exit(void)
{
        platform_driver_unregister(&atmel_hsdc_driver);
}
module_exit(atmel_hsdc_exit);
