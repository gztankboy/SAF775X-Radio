/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
 */

#include <sfud.h>
#include <stdarg.h>
#include <string.h>

#include "gd32f30x.h"
#include "systick.h"

#define FLASH_CS(n)    n ? gpio_bit_set(GPIOD, GPIO_PIN_2):gpio_bit_reset(GPIOD, GPIO_PIN_2)

static char log_buf[256];

void sfud_log_debug(const char *file, const long line, const char *format, ...);

static void retry_delay_100us(void) {
  delay_us(100);
}

static void spi_lock(const sfud_spi *spi) {
  __disable_irq();
}

static void spi_unlock(const sfud_spi *spi) {
  __enable_irq();
}

/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
    size_t read_size) {
  sfud_err result = SFUD_SUCCESS;
  int i = 0;
  
  if (write_size) {
    SFUD_ASSERT(write_buf);
  }
  if (read_size) {
    SFUD_ASSERT(read_buf);
  }
  
  FLASH_CS(0);
  
  for(i=0; i<write_size; i++) {
    while(RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_TBE));        //等待发送区空
    spi_i2s_data_transmit(SPI2, write_buf[i]);                         //通过外设SPIx发送一个byte数据
    while(RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_RBNE));       //等待接收完一个byte
    spi_i2s_data_receive(SPI2);                              //通过SPIx最近接收的数据
  }
  
  if(read_size) {
    memset((uint8_t *)read_buf, 0xFF, read_size);
  }
  
  for(i=0; i<read_size; i++) {
    
    while(RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_TBE));        //等待发送区空
    spi_i2s_data_transmit(SPI2, 0);                         //通过外设SPIx发送一个byte数据
    while(RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_RBNE));       //等待接收完一个byte
    read_buf[i] = spi_i2s_data_receive(SPI2);                              //通过SPIx最近接收的数据
  }
  
  FLASH_CS(1);
  
  return result;
}

#ifdef SFUD_USING_QSPI
/**
 * read flash data by QSPI
 */
static sfud_err qspi_read(const struct __sfud_spi *spi, uint32_t addr, sfud_qspi_read_cmd_format *qspi_read_cmd_format,
        uint8_t *read_buf, size_t read_size) {
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your qspi read flash data code
     */

    return result;
}
#endif /* SFUD_USING_QSPI */

sfud_err sfud_spi_port_init(sfud_flash *flash) {
  sfud_err result = SFUD_SUCCESS;
  
  rcu_periph_clock_enable(RCU_GPIOB);
  rcu_periph_clock_enable(RCU_GPIOD);
  rcu_periph_clock_enable(RCU_AF);
  rcu_periph_clock_enable(RCU_SPI2);
  
  delay_us(10);
  
  //PD2 -> FLASH_CS
  GPIO_BOP(GPIOD) = GPIO_PIN_2;
  gpio_init(GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
  //PB3,5 -> SPI2_SCK,MOSI
  gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3 | GPIO_PIN_5);
  //PB4 -> SPI2_MISO
  gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
  
  spi_parameter_struct sspi;
  /* deinitilize SPI and the parameters */
  spi_struct_para_init(&sspi);
  
  spi_i2s_deinit(SPI2);
  /* SPI2 parameter config */
  sspi.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
  sspi.device_mode          = SPI_MASTER;
  sspi.frame_size           = SPI_FRAMESIZE_8BIT;
  sspi.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
  sspi.nss                  = SPI_NSS_SOFT;
  sspi.prescale             = SPI_PSC_2;  //F = 60MHz(APB1) / 2(PSC) = 30MHz (Max = 33MHz)
  sspi.endian               = SPI_ENDIAN_MSB;
  spi_init(SPI2, &sspi);
  
  spi_nss_output_disable(SPI2);
  
  spi_enable(SPI2);
  
  flash->spi.wr = spi_write_read;
  flash->spi.lock = spi_lock;
  flash->spi.unlock = spi_unlock;
  flash->spi.user_data = 0;
  flash->retry.delay = retry_delay_100us;
  flash->retry.times = 1000;
  
  return result;
}

/**
 * This function is print debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
}

/**
 * This function is print routine info.
 *
 * @param format output format
 * @param ... args
 */
void sfud_log_info(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD]");
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
}
