//#define ARGB4444
//#define ARGB1555
#define RGB565

#include "lcd-dma.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/ltdc.h>

#include "systick.h"
#include "console.h"
#include "gpio.h"
#include "sdram.h"

#define LCD_WIDTH  800
#define LCD_HEIGHT 480
#define LCD_DOTCLK 30 /* MHz */
#define REFRESH_RATE 70 /* Hz */

#define HSYNC       48
#define HBP         88
#define HFP         40

#define VSYNC        3
#define VBP         32
#define VFP         13

/* Layer 1 (bottom layer) is ARGB8888 format, full screen. */

#ifdef ARGB4444
  typedef uint16_t layer1_pixel;
  #define LCD_LAYER1_PIXFORMAT LTDC_LxPFCR_ARGB4444
#elif defined(ARGB1555)
  typedef uint16_t layer1_pixel;
  #define LCD_LAYER1_PIXFORMAT LTDC_LxPFCR_ARGB1555
#elif defined(RGB565)
  typedef uint16_t layer1_pixel;
  #define LCD_LAYER1_PIXFORMAT LTDC_LxPFCR_RGB565
  #define L1_COLOR_KEY 0xFFFFF7
#else
  typedef uint32_t layer1_pixel;
  #define LCD_LAYER1_PIXFORMAT LTDC_LxPFCR_ARGB8888
#endif

#define LCD_LAYER1_PIXEL_SIZE (sizeof(layer1_pixel))
#define LCD_LAYER1_WIDTH  LCD_WIDTH
#define LCD_LAYER1_HEIGHT LCD_HEIGHT
#define LCD_LAYER1_PIXELS (LCD_LAYER1_WIDTH * LCD_LAYER1_HEIGHT)
#define LCD_LAYER1_BYTES  (LCD_LAYER1_PIXELS * LCD_LAYER1_PIXEL_SIZE)
layer1_pixel lcd_layer1_frame_buffer[LCD_LAYER1_PIXELS] SDRAM_SECTION;

/* Layer 2 (top layer) is ARGB4444, a 128x128 square. */

typedef uint16_t layer2_pixel;
#define LCD_LAYER2_PIXFORMAT LTDC_LxPFCR_ARGB4444
#define LCD_LAYER2_PIXEL_SIZE (sizeof(layer2_pixel))
#define LCD_LAYER2_WIDTH  128
#define LCD_LAYER2_HEIGHT 128
#define LCD_LAYER2_PIXELS (LCD_LAYER2_WIDTH * LCD_LAYER2_HEIGHT)
#define LCD_LAYER2_BYTES (LCD_LAYER2_PIXELS * LCD_LAYER2_PIXEL_SIZE)
layer2_pixel lcd_layer2_frame_buffer[LCD_LAYER2_PIXELS] SDRAM_SECTION;

/*
 * Pin assignments
 *     R2      = PC10, AF14
 *     R3      = PB0,  AF09
 *     R4      = PA11, AF14
 *     R5      = PA12, AF14
 *     R6      = PB1,  AF09
 *     R7      = PG6,  AF14
 *
 *     G2      = PA6,  AF14
 *     G3      = PG10, AF09
 *     G4      = PB10, AF14
 *     G5      = PB11, AF14
 *     G6      = PC7,  AF14
 *     G7      = PD3,  AF14
 *
 *     B2      = PD6,  AF14
 *     B3      = PG11, AF11
 *     B4      = PG12, AF09
 *     B5      = PA3,  AF14
 *     B6      = PB8,  AF14
 *     B7      = PB9,  AF14
 *
 * More pins...
 *     ENABLE  = PF10, AF14
 *     DOTCLK  = PG7,  AF14
 *     HSYNC   = PC6,  AF14
 *     VSYNC   = PA4,  AF14
 */

static const gpio_pin lcd_tft_pins[] = {
    {                           // PG7 = LCD_CLK
        .gp_port   = GPIOG,
        .gp_pin    = GPIO7,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PC6 = LCD_HSYNC
        .gp_port   = GPIOC,
        .gp_pin    = GPIO6,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PA4 = LCD_VSYNC
        .gp_port   = GPIOA,
        .gp_pin    = GPIO4,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PF10 = LCD_DE
        .gp_port   = GPIOF,
        .gp_pin    = GPIO10,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },


    {                           // PC10 = LCD_R[2]
        .gp_port   = GPIOC,
        .gp_pin    = GPIO10,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PB0 = LCD_R[3]
        .gp_port   = GPIOB,
        .gp_pin    = GPIO0,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF9,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PA11 = LCD_R[4]
        .gp_port   = GPIOA,
        .gp_pin    = GPIO11,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PA12 = LCD_R[5]
        .gp_port   = GPIOA,
        .gp_pin    = GPIO12,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PB1 = LCD_R[6]
        .gp_port   = GPIOB,
        .gp_pin    = GPIO1,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF9,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PG6 = LCD_R[7]
        .gp_port   = GPIOG,
        .gp_pin    = GPIO6,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },


    {                           // PA6 = LCD_G[2]
        .gp_port   = GPIOA,
        .gp_pin    = GPIO6,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PG10 = LCD_G[3]
        .gp_port   = GPIOG,
        .gp_pin    = GPIO10,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF9,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PB10 = LCD_G[4]
        .gp_port   = GPIOB,
        .gp_pin    = GPIO10,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PB11 = LCD_G[5]
        .gp_port   = GPIOB,
        .gp_pin    = GPIO11,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PC7 = LCD_G[6]
        .gp_port   = GPIOC,
        .gp_pin    = GPIO7,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PD3 = LCD_G[7]
        .gp_port   = GPIOD,
        .gp_pin    = GPIO3,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },


    {                           // PD6 = LCD_B[2]
        .gp_port   = GPIOD,
        .gp_pin    = GPIO6,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PG11 = LCD_B[3]
        .gp_port   = GPIOG,
        .gp_pin    = GPIO11,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF11,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PG12 = LCD_B[4]
        .gp_port   = GPIOG,
        .gp_pin    = GPIO12,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF9,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PA3 = LCD_B[5]
        .gp_port   = GPIOA,
        .gp_pin    = GPIO3,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PB8 = LCD_B[6]
        .gp_port   = GPIOB,
        .gp_pin    = GPIO8,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {                           // PB9 = LCD_B[7]
        .gp_port   = GPIOB,
        .gp_pin    = GPIO9,
        .gp_mode   = GPIO_MODE_AF,
        .gp_pupd   = GPIO_PUPD_NONE,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
};
static const size_t lcd_tft_pin_count = (&lcd_tft_pins)[1] - lcd_tft_pins;

/*
 * Checkerboard pattern.  Odd squares are transparent; even squares are
 * all different colors.
 */

static void draw_layer_1(void)
{
    int row, col;
    int cel_count = (LCD_LAYER1_WIDTH >> 5) + (LCD_LAYER1_HEIGHT >> 5);

    for (row = 0; row < LCD_LAYER1_HEIGHT; row++) {
        for (col = 0; col < LCD_LAYER1_WIDTH; col++) {
            size_t i = row * LCD_LAYER1_WIDTH + col;
            uint32_t cel = (row >> 5) + (col >> 5);
            uint8_t a = cel & 1 ? 0 : 0xFF;
            uint8_t r = row * 0xFF / LCD_LAYER1_HEIGHT;
            uint8_t g = col * 0xFF / LCD_LAYER1_WIDTH;
            uint8_t b = 0xFF * (cel_count - cel - 1) / cel_count;
            if (!(cel & 3))
                b = 0;

            /* Put black and white borders around the squares. */
            if (row % 32 == 0 || col % 32 == 0) {
                r = g = b = a ? 0xFF : 0;
                a = 0xFF;
            }

            layer1_pixel pix;
#ifdef ARGB4444
            pix = a >> 4 << 12 | r >> 4 << 8 | g >> 4 << 4 | b >> 4 << 0;
#elif defined(ARGB1555)
            pix = a >> 7 << 15 | r >> 3 << 10 | g >> 3 << 5 | b >> 3 << 0;
#elif defined(RGB565)
            if (a == 0) {
                r = (uint8_t)(L1_COLOR_KEY >> 16);
                g = (uint8_t)(L1_COLOR_KEY >>  8);
                b = (uint8_t)(L1_COLOR_KEY >>  0);
            }
            pix = r >> 3 << 11 | g >> 2 << 5 | b >> 3 << 0;
#else
            pix = a << 24 | r << 16 | g << 8 | b << 0;
#endif

            /*
             * Outline the screen in white.  Put a black
             * dot at the origin.
             *
             * (The origin is in the lower left!)
             */
            if (row == 0 || col == 0 ||
                row == LCD_LAYER1_HEIGHT - 1 || col == LCD_LAYER1_WIDTH - 1)
#if defined(ARGB4444) || defined(ARGB1555) || defined(RGB565)
                pix = 0xFFFF;
#else
                pix = 0xFFFFFFFF;
#endif
            else if (row < 20 && col < 20) {
#ifdef ARGB4444
                pix = 0xF000;
#elif defined(ARGB1555)
                pix = 0x8000;
#elif defined(RGB565)
                pix = 0x0000;
#else
                pix = 0xFF000000;
#endif
            }
            // pix = (row ^ col) & 1 ? 0xFFFFFFFF : 0xff000000;
            lcd_layer1_frame_buffer[i] = pix;
        }
    }
}

/*
 * Layer 2 holds the sprite.  The sprite is a semitransparent
 * magenta/cyan diamond outlined in black.
 */

static void draw_layer_2(void)
{
    int row, col;
    const uint32_t hw = LCD_LAYER2_WIDTH / 2;
    const uint32_t hh = LCD_LAYER2_HEIGHT / 2;
    const uint32_t sz = (hw + hh) / 2;

    for (row = 0; row < LCD_LAYER2_HEIGHT; row++) {
        for (col = 0; col < LCD_LAYER2_WIDTH; col++) {
            size_t i = row * LCD_LAYER2_WIDTH + col;
            uint8_t dx = abs(col  - hw);
            uint8_t dy = abs(row  - hh);
            uint8_t dxy = dx + dy;
            uint8_t a = dxy <= sz ? 0xF * dxy / (sz / 2) : 0x0;
            if (a > 0xF) {
                if (a < 0x14)
                    a = 0xF;
                else
                    a &= 0xF;
            }
            uint8_t r = dx >= dy ? 0xF : 0x0;
            uint8_t g = dy >= dx ? 0xF : 0x0;
            uint8_t b = 0xF;
            if (dx + dy >= sz - 2 || dx == dy)
                r = g = b = 0;
            layer2_pixel pix = a << 12 | r << 8 | g << 4 | b << 0;
            lcd_layer2_frame_buffer[i] = pix;
        }
    }
}

void lcd_dma_setup(void)
{
    draw_layer_1();
    draw_layer_2();

    gpio_init_pins(lcd_tft_pins, lcd_tft_pin_count);

    /*
     * The datasheet says (Figure 16, page 151):
     *     The LCD-TFT clock comes from PLLSAI.
     *     PLLSRC selects either HSI or HSE.
     *     PLLSAI's input clock is either HSI or HSE divided by PLLM.
     *     PLLSAI's PLLLCDCLK output is the input * PLLSAIN / PLLSAIR.
     *     LCD-TFT clock is PLLLCDCLK divided by PLLSAIDIVR.
     *
     * PLLSRC and PLLM are in the RCC_PLLCFGR register.
     * PLLSAIN and PLLSAIR are in RCC_PLLSAICFGR.
     * PLLSAIDIVR is in RCC_DCKCFGR;
     *
     * In our case,
     * PLLSRC already selected HSE, which is 8 MHz.
     * PLLM is already set to 8.  8 MHz / 8 = 1 MHz.
     * We set PLLSAIN = 180 and PLLSAIR = 3.  1 MHz * 180 / 3 = 60 MHz.
     * We set PLLSAIDIVR to 2.  60 MHz / 2 = 30 MHz.
     * So the LCD-TFT pixel clock is 30 MHz.
     *
     * The number of clocks per frame is
     * (VSYNC + VBP + LCD_HEIGHT + VFP) * (HSYNC + HBP + LCD_WIDTH + HFP) =
     * (3 + 32 + 480 + 13) * (48 + 88 + 800 + 40) = 515328.
     *
     * So the refresh frequency is 30 MHz / 515328 ~= 58.2 Hz.
     */

    uint32_t sain = 180;
#if !defined(ARGB4444) && !defined(ARGB1555) && !defined(RGB565)
    sain = 90;
#endif
    uint32_t saiq = (RCC_PLLSAICFGR >> RCC_PLLSAICFGR_PLLSAIQ_SHIFT) &
        RCC_PLLSAICFGR_PLLSAIQ_MASK;
    uint32_t sair = 3;
    RCC_PLLSAICFGR = (sain << RCC_PLLSAICFGR_PLLSAIN_SHIFT |
                      saiq << RCC_PLLSAICFGR_PLLSAIQ_SHIFT |
                      sair << RCC_PLLSAICFGR_PLLSAIR_SHIFT);
    RCC_DCKCFGR |= RCC_DCKCFGR_PLLSAIDIVR_DIVR_2;
    RCC_CR |= RCC_CR_PLLSAION;
    while ((RCC_CR & RCC_CR_PLLSAIRDY) == 0)
        continue;
    RCC_APB2ENR |= RCC_APB2ENR_LTDCEN;

    /*
     * Busy-wait until the CPU has been up for 16 msec.
     * The KD50G21 datasheet says that power needs to be stable for
     * 16 msec before the video signal starts.
     */
    while (system_millis < 16)
        continue;

    /*
     * Configure the Synchronous timings: VSYNC, HSYNC,
     * Vertical and Horizontal back porch, active data area, and
     * the front porch timings.
     */
    LTDC_SSCR = (HSYNC - 1) << LTDC_SSCR_HSW_SHIFT |
        (VSYNC - 1) << LTDC_SSCR_VSH_SHIFT;
    LTDC_BPCR = (HSYNC + HBP - 1) << LTDC_BPCR_AHBP_SHIFT |
        (VSYNC + VBP - 1) << LTDC_BPCR_AVBP_SHIFT;
    LTDC_AWCR = (HSYNC + HBP + LCD_WIDTH - 1) << LTDC_AWCR_AAW_SHIFT |
        (VSYNC + VBP + LCD_HEIGHT - 1) << LTDC_AWCR_AAH_SHIFT;
    LTDC_TWCR =
        (HSYNC + HBP + LCD_WIDTH + HFP - 1) << LTDC_TWCR_TOTALW_SHIFT |
        (VSYNC + VBP + LCD_HEIGHT + VFP - 1) << LTDC_TWCR_TOTALH_SHIFT;

    /* Configure the synchronous signals and clock polarity. */
    LTDC_GCR |= LTDC_GCR_PCPOL_ACTIVE_LOW;
    LTDC_GCR |= LTDC_GCR_DITHER_ENABLE;

    /* If needed, configure the background color. */
    LTDC_BCCR = 0x00000000;

    /* Configure the needed interrupts. */
    LTDC_IER = LTDC_IER_RRIE;
    nvic_enable_irq(NVIC_LCD_TFT_IRQ);

    /* Configure the Layer 1 parameters.
     * (Layer 1 is the bottom layer.)    */
    {
        /* The Layer window horizontal and vertical position */
        uint32_t h_start = HSYNC + HBP + 0;
        uint32_t h_stop = HSYNC + HBP + LCD_LAYER1_WIDTH - 1;
        LTDC_L1WHPCR = h_stop << LTDC_LxWHPCR_WHSPPOS_SHIFT |
            h_start << LTDC_LxWHPCR_WHSTPOS_SHIFT;
        uint32_t v_start = VSYNC + VBP + 0;
        uint32_t v_stop = VSYNC + VBP + LCD_LAYER1_HEIGHT - 1;
        LTDC_L1WVPCR = v_stop << LTDC_LxWVPCR_WVSPPOS_SHIFT |
            v_start << LTDC_LxWVPCR_WVSTPOS_SHIFT;

        /* The pixel input format */
        LTDC_L1PFCR = LCD_LAYER1_PIXFORMAT;

        /* The color frame buffer start address */
        LTDC_L1CFBAR = (uint32_t)lcd_layer1_frame_buffer;

        /* The line length and pitch of the color frame buffer */
        uint32_t pitch = LCD_LAYER1_WIDTH * LCD_LAYER1_PIXEL_SIZE;
        uint32_t length = LCD_LAYER1_WIDTH * LCD_LAYER1_PIXEL_SIZE + 3;
        LTDC_L1CFBLR = pitch << LTDC_LxCFBLR_CFBP_SHIFT |
            length << LTDC_LxCFBLR_CFBLL_SHIFT;

        /* The number of lines of the color frame buffer */
        LTDC_L1CFBLNR = LCD_LAYER1_HEIGHT;

        /* If needed, load the CLUT */
        /* (not using CLUT) */

        /* If needed, configure the default color and blending
         * factors
         */
        LTDC_L1CACR = 0x000000FF;
        LTDC_L1BFCR = LTDC_LxBFCR_BF1_PIXEL_ALPHA_x_CONST_ALPHA |
            LTDC_LxBFCR_BF2_PIXEL_ALPHA_x_CONST_ALPHA;
    }

    /* Configure the Layer 2 parameters. */
    {
        /* The Layer window horizontal and vertical position */
        uint32_t h_start = HSYNC + HBP + 0;
        uint32_t h_stop = HSYNC + HBP + LCD_LAYER2_WIDTH - 1;
        LTDC_L2WHPCR = h_stop << LTDC_LxWHPCR_WHSPPOS_SHIFT |
            h_start << LTDC_LxWHPCR_WHSTPOS_SHIFT;
        uint32_t v_start = VSYNC + VBP + 0;
        uint32_t v_stop = VSYNC + VBP + LCD_LAYER2_HEIGHT - 1;
        LTDC_L2WVPCR = v_stop << LTDC_LxWVPCR_WVSPPOS_SHIFT |
            v_start << LTDC_LxWVPCR_WVSTPOS_SHIFT;

        /* The pixel input format */
        LTDC_L2PFCR = LCD_LAYER2_PIXFORMAT;

        /* The color frame buffer start address */
        LTDC_L2CFBAR = (uint32_t)lcd_layer2_frame_buffer;

        /* The line length and pitch of the color frame buffer */
        uint32_t pitch = LCD_LAYER2_WIDTH * LCD_LAYER2_PIXEL_SIZE;
        uint32_t length = LCD_LAYER2_WIDTH * LCD_LAYER2_PIXEL_SIZE + 3;
        LTDC_L2CFBLR = pitch << LTDC_LxCFBLR_CFBP_SHIFT |
            length << LTDC_LxCFBLR_CFBLL_SHIFT;

        /* The number of lines of the color frame buffer */
        LTDC_L2CFBLNR = LCD_LAYER2_HEIGHT;

        /* If needed, load the CLUT */
        /* (not using CLUT) */

        /* If needed, configure the default color and blending
         * factors
         */
        LTDC_L2CACR = 0x000000FF;
        LTDC_L2BFCR = LTDC_LxBFCR_BF1_PIXEL_ALPHA_x_CONST_ALPHA |
            LTDC_LxBFCR_BF2_PIXEL_ALPHA_x_CONST_ALPHA;
    }

    /* Enable Layer1 and if needed the CLUT */
    LTDC_L1CR |= LTDC_LxCR_LAYER_ENABLE;

    /* Enable Layer2 and if needed the CLUT */
    LTDC_L2CR |= LTDC_LxCR_LAYER_ENABLE;

    /* If needed, enable dithering and/or color keying. */
#ifdef RGB565
    LTDC_L1CKCR = L1_COLOR_KEY;
    LTDC_L1CR = LTDC_LxCR_LAYER_ENABLE | LTDC_LxCR_COLKEY_ENABLE;
#else
    /* (Not needed) */
#endif

    /* Reload the shadow registers to active registers. */
    LTDC_SRCR |= LTDC_SRCR_VBR;

    /* Enable the LCD-TFT controller. */
    LTDC_GCR |= LTDC_GCR_LTDC_ENABLE;
}

static void mutate_background_color(void)
{
    static uint32_t ints;
    ints += 3;
    uint32_t shift = ints >> 9;
    if (shift >= 3)
        ints = shift = 0;
    uint32_t component = ints & 0xFF;
    if (ints & 0x100)
        component = 0xff - component;

    LTDC_BCCR = component << 8 * shift;
}

/*
 * The sprite bounce algorithm works surprisingly well for a first
 * guess.  Whenever the sprite touches a wall, we reverse its velocity
 * normal to the wall, and pick a random velocity from -3 to +3
 * parallel to the wall.  (e.g., if it touches a side, reverse the X
 * velocity and pick a random Y velocity.)  That gives enough
 * unpredictability to make it interesting.
 *
 * The random numbers come from rand(), and we do not call srand().
 * That means the sprite makes exactly the same moves every time the
 * demo is run.  (Repeatability is a feature.)
 */

static void move_sprite(void)
{
    static int8_t dx = 1, dy = 1;
    static int16_t x, y;
    static int16_t age;
    x += dx;
    y += dy;
    if (x < 0) {
        dy = rand() % 7 - 3;
        dx = -dx;
        x = 0;
        age = 0;
    } else if (x >= LCD_WIDTH - LCD_LAYER2_WIDTH) {
        dy = rand() % 7 - 3;
        dx = -dx;
        x = LCD_WIDTH - LCD_LAYER2_WIDTH - 1;
        age = 0;
    }
    if (y < 0) {
        dx = rand() % 7 - 3;
        dy = -dy;
        y = 0;
        age = 0;
    } else if (y >= LCD_HEIGHT - LCD_LAYER2_HEIGHT) {
        dx = rand() % 7 - 3;
        dy = -dy;
        y = LCD_HEIGHT - LCD_LAYER2_HEIGHT - 1;
        age = 0;
    }
    if (dy == 0 && dx == 0)
        dy = y ? -1 : +1;
    uint32_t h_start = HSYNC + HBP + x;
    uint32_t h_stop = h_start + LCD_LAYER2_WIDTH - 1;
    LTDC_L2WHPCR = h_stop << LTDC_LxWHPCR_WHSPPOS_SHIFT |
                   h_start << LTDC_LxWHPCR_WHSTPOS_SHIFT;
    uint32_t v_start = VSYNC + VBP + y;
    uint32_t v_stop = v_start + LCD_LAYER2_HEIGHT - 1;
    LTDC_L2WVPCR = v_stop << LTDC_LxWVPCR_WVSPPOS_SHIFT |
                   v_start << LTDC_LxWVPCR_WVSTPOS_SHIFT;

    /* The sprite fades away as it ages. */
    age += 1;
    if (age > 0xFF)
        age = 0xFF;
    LTDC_L2CACR = 0x000000FF - age;
}

/*
 * Here is where all the work is done.  We poke a total of 6 registers
 * for each frame.
 */

void lcd_tft_isr(void)
{
    // Clear Register Reload Interrupt Flag.
    // Register Reload interrupt is generated when a vertical blanking
    // reload occurs (and the first line after the active area is reached)
    LTDC_ICR |= LTDC_ICR_CRRIF;

    mutate_background_color();
    move_sprite();

    // Vertical Blanking Reload
    // The shadow registers are reloaded during the vertical blanking
    // period (at the beginning of the first line after the Active
    // Display Area)
    LTDC_SRCR |= LTDC_SRCR_VBR;
}
