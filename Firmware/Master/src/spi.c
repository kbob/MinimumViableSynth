#include "spi.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libopencm3/stm32/dma.h>
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

// Each active DMA requires a controller/stream/channel triple.
// Each controller/stream can only have a single channel active at a time.
//
// SPI 1 RX: 2/0/3, 2/2/3
// SPI 1 TX: 2/3/3, 2/5/3
//
// SPI 3 RX: 1/0/0, 1/2/0
// SPI 3 TX: 1/5/0, 1/7/0
//
// SPI 4 RX: 2/0/4, 2/3/5
// SPI 4 TX: 2/1/4, 2/4/5
//
// SPI 5 RX: 2/3/2, 2/5/7
// SPI 5 TX: 2/4/2, 2/6/7

#define RX_DMA 1
#define TX_DMA 1

typedef struct gpio_pin {
    uint32_t gp_port;
    uint16_t gp_pin;
    uint8_t  gp_af;
} gpio_pin;

typedef struct DMA_channel {
    uint32_t dc_dma;
    uint8_t  dc_stream;
    uint8_t  dc_channel;
} DMA_channel;

typedef struct spi_config {
    uint32_t sc_reg_base;
    gpio_pin sc_sck;
    gpio_pin sc_miso;
    gpio_pin sc_mosi;
    DMA_channel sc_rx_dma;
    DMA_channel sc_tx_dma;
} spi_config;

static const spi_config spi1_config = {
    .sc_reg_base = SPI1_BASE,
    .sc_sck      = { GPIOA, GPIO5, GPIO_AF5 },
    .sc_miso     = { GPIOB, GPIO4, GPIO_AF5 },
    .sc_mosi     = { GPIOA, GPIO7, GPIO_AF5 },
    .sc_rx_dma   = { DMA2,  DMA_STREAM2, 3 },
    .sc_tx_dma   = { DMA2,  DMA_STREAM3, 3 },
};

static const spi_config spi3_config = {
    .sc_reg_base = SPI3_BASE,
    .sc_sck      = { GPIOB, GPIO3, GPIO_AF6 },
    .sc_miso     = { GPIOC, GPIO11, GPIO_AF6 },
    .sc_mosi     = { GPIOC, GPIO12, GPIO_AF6 },
    .sc_rx_dma   = { DMA1,  DMA_STREAM0, 0 },
    .sc_tx_dma   = { DMA1,  DMA_STREAM5, 0 },
};

static const spi_config spi4_config = {
    .sc_reg_base = SPI4_BASE,
    .sc_sck      = { GPIOE, GPIO2, GPIO_AF5 },
    .sc_miso     = { GPIOE, GPIO5, GPIO_AF5 },
    .sc_mosi     = { GPIOE, GPIO6, GPIO_AF5 },
    .sc_rx_dma   = { DMA2,  DMA_STREAM0, 4 },
    .sc_tx_dma   = { DMA2,  DMA_STREAM1, 4 },
};

static const spi_config spi5_config = {
    .sc_reg_base = SPI5_BASE,
    .sc_sck      = { GPIOF, GPIO7, GPIO_AF5 },
    .sc_miso     = { GPIOF, GPIO8, GPIO_AF5 },
    .sc_mosi     = { GPIOF, GPIO9, GPIO_AF5 },
    .sc_rx_dma   = { DMA2,  DMA_STREAM5, 7 },
    .sc_tx_dma   = { DMA2,  DMA_STREAM6, 7 },
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
#if RX_DMA
    spi_enable_rx_dma(config->sc_reg_base);
#endif
#if TX_DMA
    spi_enable_tx_dma(config->sc_reg_base);
#endif
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

#if RX_DMA || TX_DMA
    // rcc_periph_clock_enable(RCC_DMA1);
    rcc_periph_clock_enable(RCC_DMA2);
#endif
    
    // spi_setup_config(&spi1_config);
    // spi_setup_config(&spi3_config);
    // spi_setup_config(&spi4_config);
    spi_setup_config(&spi5_config);

    // Configure nSS pins
    for (size_t i = 0; i < group_count; i++) {
        const gpio_pin *gp = &group_ss_pins[i];
        gpio_mode_setup(gp->gp_port,
                        GPIO_MODE_OUTPUT,
                        GPIO_PUPD_NONE,
                        gp->gp_pin);
        gpio_set(gp->gp_port, gp->gp_pin);
    }

    // Hold pins PC1 and PC2 high.
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1 | GPIO2);
    gpio_set(GPIOC, GPIO1);
    gpio_set(GPIOC, GPIO2);
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
#if RX_DMA
    uint32_t rx_dma = config->sc_rx_dma.dc_dma;
    uint8_t rx_stream = config->sc_rx_dma.dc_stream;
    uint32_t rx_channel = config->sc_rx_dma.dc_channel;
#endif
#if TX_DMA
    uint32_t tx_dma = config->sc_tx_dma.dc_dma;
    uint8_t tx_stream = config->sc_tx_dma.dc_stream;
    uint32_t tx_channel = config->sc_tx_dma.dc_channel;
#endif
    
#if RX_DMA
    //  1. Clear SxCR_EN.  Wait until SxCR_EN == 0.
    dma_stream_reset(rx_dma, rx_stream);
    while (DMA_SCR(rx_dma, rx_stream) & DMA_SxCR_EN)
        continue;

    //  2. Set peripheral port address register, SxPAR.
    dma_set_peripheral_address(rx_dma, rx_stream, (uint32_t)&SPI_DR(base));

    //  3. Set memory address register, SxMA0R.
    dma_set_memory_address(rx_dma, rx_stream, (uint32_t)rx_buf);

    //  4. Set number of data items, SxNDTR
    dma_set_number_of_data(rx_dma, rx_stream, count);

    //  5. Set the dma channel using SxCR.CHSEL[2:0].
    uint32_t rx_chsel = (uint32_t)rx_channel << DMA_SxCR_CHSEL_SHIFT;
    dma_channel_select(rx_dma, rx_stream, rx_chsel);

    //  6. Set SxCR.PFCTRL.
    // dma_set_peripheral_flow_control(rx_dma, rx_stream);

    //  7. Set the stream priority.
    dma_set_priority(rx_dma, rx_stream, DMA_SxCR_PL_LOW);

    //  8. Set FIFO mode: disabled.
    dma_enable_direct_mode(rx_dma, rx_stream);

    //  9. Set data transfer direction,
    //         peripheral and memory increment/fixed mode,
    //         single or burst transactions,
    //         peripheral and memory data widths,
    //         circular mode,
    //         double buffer mode,
    //         and interrupts
    //     in the SxCR register.
    dma_disable_peripheral_increment_mode(rx_dma, rx_stream);
    dma_enable_memory_increment_mode(rx_dma, rx_stream);
    dma_set_peripheral_burst(rx_dma, rx_stream, DMA_SxCR_PBURST_SINGLE);
    dma_set_memory_burst(rx_dma, rx_stream, DMA_SxCR_MBURST_SINGLE);
    dma_set_peripheral_size(rx_dma, rx_stream, DMA_SxCR_PSIZE_8BIT);
    dma_set_memory_size(rx_dma, rx_stream, DMA_SxCR_MSIZE_8BIT);
    dma_set_transfer_mode(rx_dma, rx_stream, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_disable_double_buffer_mode(rx_dma, rx_stream);
    dma_disable_transfer_error_interrupt(rx_dma, rx_stream);
    dma_disable_half_transfer_interrupt(rx_dma, rx_stream);
    dma_disable_transfer_complete_interrupt(rx_dma, rx_stream);
    dma_disable_direct_mode_error_interrupt(rx_dma, rx_stream);
    dma_disable_fifo_error_interrupt(rx_dma, rx_stream);
    printf("%s:%d: %s: DMA_S%dCR = %#lx\n",
           __FILE__, __LINE__, __func__,
           rx_stream, DMA_SCR(rx_dma, rx_stream));

    // 10. Set SxCR_EN.
    dma_enable_stream(rx_dma, rx_stream);
#endif
#if TX_DMA
    //  1. Clear SxCR_EN.  Wait until SxCR_EN == 0.
    dma_stream_reset(tx_dma, tx_stream);
    while (DMA_SCR(tx_dma, tx_stream) & DMA_SxCR_EN)
        continue;

    //  2. Set peripheral port address register, SxPAR.
    dma_set_peripheral_address(tx_dma, tx_stream, (uint32_t)&SPI_DR(base));

    //  3. Set memory address register, SxMA0R.
    dma_set_memory_address(tx_dma, tx_stream, (uint32_t)tx_buf);

    //  4. Set number of data items, SxNDTR
    dma_set_number_of_data(tx_dma, tx_stream, count);

    //  5. Set the dma channel using SxCR.CHSEL[2:0].
    uint32_t tx_chsel = (uint32_t)tx_channel << DMA_SxCR_CHSEL_SHIFT;
    dma_channel_select(tx_dma, tx_stream, tx_chsel);

    //  6. Set SxCR.PFCTRL.
    // dma_set_peripheral_flow_control(tx_dma, tx_stream);

    //  7. Set the stream priority.
    dma_set_priority(tx_dma, tx_stream, DMA_SxCR_PL_LOW);

    //  8. Set FIFO mode: disabled.
    dma_enable_direct_mode(tx_dma, tx_stream);

    //  9. Set data transfer direction,
    //         peripheral and memory increment/fixed mode,
    //         single or burst transactions,
    //         peripheral and memory data widths,
    //         circular mode,
    //         double buffer mode,
    //         and interrupts
    //     in the SxCR register.
    dma_disable_peripheral_increment_mode(tx_dma, tx_stream);
    dma_enable_memory_increment_mode(tx_dma, tx_stream);
    dma_set_peripheral_burst(tx_dma, tx_stream, DMA_SxCR_PBURST_SINGLE);
    dma_set_memory_burst(tx_dma, tx_stream, DMA_SxCR_MBURST_SINGLE);
    dma_set_peripheral_size(tx_dma, tx_stream, DMA_SxCR_PSIZE_8BIT);
    dma_set_memory_size(tx_dma, tx_stream, DMA_SxCR_MSIZE_8BIT);
    dma_set_transfer_mode(tx_dma, tx_stream, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    dma_disable_double_buffer_mode(tx_dma, tx_stream);
    dma_disable_transfer_error_interrupt(tx_dma, tx_stream);
    dma_disable_half_transfer_interrupt(tx_dma, tx_stream);
    dma_disable_transfer_complete_interrupt(tx_dma, tx_stream);
    dma_disable_direct_mode_error_interrupt(tx_dma, tx_stream);
    dma_disable_fifo_error_interrupt(tx_dma, tx_stream);
    printf("%s:%d: %s: DMA_S%dCR = %#lx\n",
           __FILE__, __LINE__, __func__,
           tx_stream, DMA_SCR(tx_dma, tx_stream));

    // 10. Set SxCR_EN.
    dma_enable_stream(tx_dma, tx_stream);
    // printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
#endif
    spi_enable(config->sc_reg_base);
    // printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
    for (size_t i = 0; i < count; i++) {
        // printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
#if !TX_DMA
        while (!(SPI_SR(base) & SPI_SR_TXE))
            continue;
        SPI_DR(base) = tx_buf[i];
#endif
#if !RX_DMA
        while (!(SPI_SR(base) & SPI_SR_RXNE))
            continue;
        rx_buf[i] = SPI_DR(base);
#endif
    }
    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
#if TX_DMA
    while (!(DMA_HISR(tx_dma) & DMA_HISR_TCIF6))
        continue;
    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
#endif
    // See Section 28.3.8, Disabling the SPI.
    while (!(SPI_SR(base) & SPI_SR_TXE))
        continue;
    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
    while (SPI_SR(base) & SPI_SR_BSY)
        continue;
    printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
#if TX_DMA
    DMA_SCR(tx_dma, tx_stream) &= ~DMA_SxCR_EN;
    while (DMA_SCR(tx_dma, tx_stream) & DMA_SxCR_EN)
        continue;
#endif
#if RX_DMA
    DMA_SCR(rx_dma, rx_stream) &= ~DMA_SxCR_EN;
    while (DMA_SCR(rx_dma, rx_stream) & DMA_SxCR_EN)
        continue;
#endif
    spi_disable(base);
}

void spi_finish_transfer(int spi)
{
    // Nothing to do
}
