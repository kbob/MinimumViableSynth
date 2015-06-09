#include "spi.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

// We use SPI buses 1, 3, 4, and 5.
// SPI bus 5 is also used for the LCD panel and the MEMS device.
// Pins PC1 and PC2 are SPI chip select pins for LCD and MEMS devices,
// so they must be held high.

// Pins
//
//  SPI 1
//    SCK   PA5   AF5
//    MISO  PB4   AF5
//    MOSI  PA7   AF5
//
//  SPI 3
//    SCK   PB3   AF6
//    MISO  PC11  AF6
//    MOSI  PC12  AF6
//
//  SPI 4
//    SCK   PE2   AF5
//    MISO  PE5   AF5
//    MOSI  PE6   AF5
//
//  SPI 5
//    SCK   PF7   AF5
//    MISO  PF8   AF5
//    MOSI  PF9   AF5

// There are three or four SPI groups.
// Each group uses all four SPI buses and is enabled by a single slave
// select line.  The nSS lines are controlled by software
//
//    Group A: PB7
//    Group B:  ?
//    Group C:  ?
//    Group D:  ?

typedef struct gpio_pin {
    uint32_t gp_port;
    uint16_t gp_pin;
    uint8_t  gp_af;
} gpio_pin;

typedef struct spi_config {
    uint32_t sc_reg_base;
    gpio_pin sc_sck;
    gpio_pin sc_miso;
    gpio_pin sc_mosi;
} spi_config;

static const spi_config spi1_config = {
    .sc_reg_base = SPI1_BASE,
    .sc_sck  = { GPIOA,  GPIO5, GPIO_AF5 },
    .sc_miso = { GPIOB,  GPIO4, GPIO_AF5 },
    .sc_mosi = { GPIOA,  GPIO7, GPIO_AF5 },
};

static const spi_config spi3_config = {
    .sc_reg_base = SPI3_BASE,
    .sc_sck  = { GPIOB,  GPIO3, GPIO_AF6 },
    .sc_miso = { GPIOC, GPIO11, GPIO_AF6 },
    .sc_mosi = { GPIOC, GPIO12, GPIO_AF6 },
};

static const spi_config spi4_config = {
    .sc_reg_base = SPI4_BASE,
    .sc_sck  = { GPIOE,  GPIO2, GPIO_AF5 },
    .sc_miso = { GPIOE,  GPIO5, GPIO_AF5 },
    .sc_mosi = { GPIOE,  GPIO6, GPIO_AF5 },
};

static const spi_config spi5_config = {
    .sc_reg_base = SPI5_BASE,
    .sc_sck  = { GPIOF,  GPIO7, GPIO_AF5 },
    .sc_miso = { GPIOF,  GPIO8, GPIO_AF5 },
    .sc_mosi = { GPIOF,  GPIO9, GPIO_AF5 },
};

static const spi_config *config_map[7] = {
    NULL,
    &spi1_config,
    NULL,
    &spi3_config,
    &spi4_config,
    &spi5_config,
    NULL
};

static const gpio_pin group_ss_pins[] = {
    { GPIOB, GPIO7, GPIO_AF0 },
};
static size_t group_count = sizeof group_ss_pins / sizeof group_ss_pins[0];

static void spi_setup_pin(const gpio_pin *pin, int pupd)
{
    // printf("gpio_mode_setup(port=%#lx, mode=%#x, updown=%#x, gpios=%#x)\n",
    //        pin->gp_port,
    //        GPIO_MODE_AF,
    //        GPIO_PUPD_NONE,
    //        pin->gp_pin);

    gpio_mode_setup(pin->gp_port,
                    GPIO_MODE_AF,
                    pupd,
                    pin->gp_pin);
    printf("gpio_set_af(%#lx, %#x, %#x)\n",
           pin->gp_port,
           pin->gp_af,
           pin->gp_pin);

    gpio_set_af(pin->gp_port,
                pin->gp_af,
                pin->gp_pin);
}

static void spi_setup_config(const spi_config *config)
{
    // GPIO 
    spi_setup_pin(&config->sc_sck, GPIO_PUPD_PULLDOWN);
    spi_setup_pin(&config->sc_miso, GPIO_PUPD_PULLUP);
    spi_setup_pin(&config->sc_mosi, GPIO_PUPD_NONE);

    // Init SPI
    printf("%s:%d: %s: spi_init_master(%#lx, %#x, %#x, %#x, %#x, %#x)\n",
           __FILE__, __LINE__, __func__,
           config->sc_reg_base,
           SPI_CR1_BAUDRATE_FPCLK_DIV_256,
           SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
           SPI_CR1_CPHA_CLK_TRANSITION_2,
           SPI_CR1_DFF_8BIT,
           SPI_CR1_MSBFIRST);
    spi_init_master(config->sc_reg_base,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_256,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_2,
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    printf("%s:%d: %s: SPI_CR1 = %#lx\n",
           __FILE__, __LINE__, __func__,
           SPI_CR1(config->sc_reg_base));
    spi_enable_ss_output(config->sc_reg_base);
    printf("%s:%d: %s: SPI_CR1 = %#lx\n",
           __FILE__, __LINE__, __func__,
           SPI_CR1(config->sc_reg_base));
}

void spi_setup()
{
    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
    // Clocks.
    // rcc_periph_clock_enable(RCC_SPI1);
    // rcc_periph_clock_enable(RCC_SPI3);
    // rcc_periph_clock_enable(RCC_SPI4);
    rcc_periph_clock_enable(RCC_SPI5);

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOE);
    rcc_periph_clock_enable(RCC_GPIOF);

    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
    
    // spi_setup_config(&spi1_config);
    // spi_setup_config(&spi3_config);
    // spi_setup_config(&spi4_config);
    spi_setup_config(&spi5_config);
    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);

    // Configure nSS pins
    for (size_t i = 0; i < group_count; i++) {
        const gpio_pin *gp = &group_ss_pins[i];
        printf("%s:%d: %s: gpio_mode_setup(%#lx, %#x, %#x, %#x)\n",
               __FILE__, __LINE__, __func__,
               gp->gp_port,
               GPIO_MODE_OUTPUT,
               GPIO_PUPD_NONE,
               gp->gp_pin);
        gpio_mode_setup(gp->gp_port,
                        GPIO_MODE_OUTPUT,
                        GPIO_PUPD_NONE,
                        gp->gp_pin);
        printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
        gpio_set(gp->gp_port, gp->gp_pin);
    }
    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);

    // Hold pins PC1 and PC2 high.
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1 | GPIO2);
    gpio_set(GPIOC, GPIO1);
    gpio_set(GPIOC, GPIO2);
    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
}

void spi_select_group(int group)
{
    assert(group <= group_count);
    const gpio_pin *grp = &group_ss_pins[group];

    gpio_clear(grp->gp_port, grp->gp_pin);
}

void spi_deselect_group(int group)
{
    assert(group <= group_count);
    const gpio_pin *grp = &group_ss_pins[group];

    gpio_set(grp->gp_port, grp->gp_pin);
}

void spi_start_transfer(int            spi,
                        uint8_t const *tx_buf,
                        uint8_t       *rx_buf,
                        size_t         count)
{
    assert(spi <= 6);
    const spi_config *config = config_map[spi];
    assert(config);
    uint32_t base = config->sc_reg_base;
    
    // printf("spi_start_transfer: config = %p\n", config);
    // printf("                    base   = %#lx\n", base);
    spi_enable(config->sc_reg_base);
    // printf("%s:%d: %s: SPI_CR1 = %#lx\n",
    //        __FILE__, __LINE__, __func__,
    //        SPI_CR1(base));
    for (size_t i = 0; i < count; i++) {
	while (!(SPI_SR(base) & SPI_SR_TXE))
            continue;
	SPI_DR(base) = tx_buf[i];
        while (!(SPI_SR(base) & SPI_SR_RXNE))
            continue;
        rx_buf[i] = SPI_DR(base);
        // for (int i = 0; i < 200; i++)
        //     __asm__("nop");
        // printf("                 %u: SR = %#lx\n", i, SPI_SR(base));
    }
    spi_disable(base);
}


void spi_finish_transfer(int spi)
{
    // Nothing to do
}
