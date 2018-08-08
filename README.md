# NUC_JZQ_KERNEL
NUC972硬件平台
内核版本 Linux-3.10.107
arch文件夹：存放整个内核的一些参数配置
drivers文件夹：各个接口设备的驱动程序所在目录
.config：make menuconfig之后产生的最终配置文件

已确认可用的接口：
1、七个UART:UART0、UART2、UART3、UART6、UART7、UART8、UART9.
2、RTC:使用i2c接口和rx8025的RTC驱动
3、Nor Flash:spi0接口
4、按键，液晶，脉冲，计量芯片、ESAM全部在新创的char设备驱动中
5、NAND Flash:单独的驱动程序
6、网口：emac0驱动
7、USB：两个USB口驱动
8、定时器：新增加timer3（1毫秒）定时中断
9、pwm输出给红外发送时钟
10、gpio管脚控制驱动
