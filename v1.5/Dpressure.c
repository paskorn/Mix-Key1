
void DPressure_init()
{
   //setup_adc_ports(NO_ANALOGS|VSS_VDD);
   setup_adc(ADC_OFF);
   //setup_psp(PSP_DISABLED);
   //setup_spi(SPI_SS_DISABLED);
   setup_wdt(WDT_OFF);
   setup_timer_0(RTCC_INTERNAL);
   setup_timer_1(T1_DISABLED);
   setup_timer_2(T2_DISABLED,0,1);
   //setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);

   //setup_adc_ports(ALL_ANALOG);
   //setup_adc(ADC_CLOCK_INTERNAL);
}

float oa_diff_water(){
   int16 value;
   int16 total = 0;
   int16 avg;
   
   for(int16 i=1;i<=100;i++){
      //set_adc_channel(0) ;// channel = 0-7
      delay_us(100); // wait for ADC to stabilize
      //value = read_adc();
      
         value = read_analog(7);
        
      // printf(usb_cdc_putc,"No. %ld ,Sensor= %ld\r\n",i,value);
      // delay_ms(50);
      total=total+value;
   }
      avg=total/100;
   
   float velocity;
   
      velocity=((-0.0024)*(avg*avg))+(0.2857*avg)-7.0963;
  
   return velocity;
}



