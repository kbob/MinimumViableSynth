#include "console.h"

#include <stdio.h>
//#include <stdlib.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#define CONSOLE_UART USART1
#define CONSOLE_BAUD 115200

void console_setup(void)
{
    // Clock
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);

    // GPIO
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

    // USART
    usart_set_baudrate(CONSOLE_UART, CONSOLE_BAUD);
    usart_set_databits(CONSOLE_UART, 8);
    usart_set_stopbits(CONSOLE_UART, USART_STOPBITS_1);
    usart_set_mode(CONSOLE_UART, USART_MODE_TX_RX);
    usart_set_parity(CONSOLE_UART, USART_PARITY_NONE);
    usart_set_flow_control(CONSOLE_UART, USART_FLOWCONTROL_NONE);
    usart_enable(CONSOLE_UART);
}

static void console_putc(char c)
{
    while (!(USART_SR(CONSOLE_UART) & USART_SR_TXE))
        continue;
    USART_DR(CONSOLE_UART) = (uint8_t)c;
}

static ssize_t console_write(void *cookie, const char *buf, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        char c = buf[i];
        if (c == '\n')
            console_putc('\r');
        console_putc(c);
    }
    return size;
}

void console_stdio_setup()
{
    cookie_io_functions_t console_input_fns = {
        .read  = NULL,
        .write = NULL,
        .seek  = NULL,
        .close = NULL
    };
    cookie_io_functions_t console_output_fns = {
        .read  = NULL,
        .write = console_write,
        .seek  = NULL,
        .close = NULL
    };
    stdin  = fopencookie(NULL, "r", console_input_fns);
    stdout = fopencookie(NULL, "w", console_output_fns);
    stderr = fopencookie(NULL, "w", console_output_fns);
    setlinebuf(stdout);
    setbuf(stderr, NULL);
}
