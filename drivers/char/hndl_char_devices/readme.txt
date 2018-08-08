2011-08-09
国网集中器交采使用7858后：
1.增加文件：
   ade7858部分
2.修改：
  hnos_ttys2_mode.c
  
   at91_set_B_periph(AT91_PIN_PB1, 0)=>at91_set_B_periph(AT91_PIN_PB0, 0); 
   at91_tc_write(AT91_TC_CMR,  TC_CMR_TIOB)=> at91_tc_write(AT91_TC_CMR,  TC_CMR_TIOA);

  drivers\char\hndl_char_devices\hnos_input_hntt1800x.c
  
    pb0=>pb6
    pb3=>pb7
    pb2=>pc12
  

  

