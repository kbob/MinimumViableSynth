#include <Bounce.h>
#include <DMAChannel.h>
#include <SPI.h>
#include <UniWS.h>

#define USE_SERIAL 1

// See synth-notes.txt for full information.
    // Teensy     Attributes        Use
    //  Pin        (subset)
    //   0                          choice button
    //   1                          destination button 3
    //   2                          destination button 4
    //   3        PWM               choice LED 1
    //   4        PWM               choice LED 2
    //   5        20mA              destination button 1
    //   6        PWM               choice LED 3
    //   7        MOSI0             destination button 2
    //   8        MISO0             assign button
    //   9        PWM               choice LED 4
    //  10        PWM,CS0           SPI
    //  11        MOSI0             SPI
    //  12        MISO0             SPI
    //  13        SCK0,LED          SPI
    //  14/A0     ADC,SCK0          knob wiper 1
    //  15/A1     ADC               knob wiper 2
    //  16/A2     ADC,PWM,20mA      knob wiper 3
    //  17/A3     ADC,PWM,5V,20mA   WS2812 strand
    //  18/A4     ADC               knob wiper 4
    //  19/A5     ADC               knob wiper 5
    //  20/A6     ADC,PWM           choice LED 5
    //  21/A7     ADC,20mA           -
    //  22/A8     ADC,PWM           choice LED 6
    //  23/A9     ADC,PWM           assign LED
    //  24/A10    ADC                -
    //  25/A11    ADC                -
    //  26/A12    ADC                -

const int CHOICE_BUTTON_pin =  0;
const int CHOICE_LED_1_pin  =  3;
const int CHOICE_LED_2_pin  =  4;
const int CHOICE_LED_3_pin  =  6;
const int CHOICE_LED_4_pin  =  9;
const int CHOICE_LED_5_pin  = 20;
const int CHOICE_LED_6_pin  = 22;
const int ANALOG_1_pin      = 14;
const int ANALOG_2_pin      = 15;
const int ANALOG_3_pin      = 16;
const int ANALOG_4_pin      = 18;
const int ANALOG_5_pin      = 19;
const int DEST_BUTTON_1_pin =  5;
const int DEST_BUTTON_2_pin =  7;
const int DEST_BUTTON_3_pin =  1;
const int DEST_BUTTON_4_pin =  2;
const int ASSIGN_BUTTON_pin =  8;
const int ASSIGN_LED_pin    = 23;
const int SPI_CS_pin        = 10;

const uint8_t  STX = '\02';
const uint8_t  ETX = '\03';
const uint8_t  SYN = '\26';

const size_t   MSG_MAX = 32;
const size_t   MAX_PIXELS = 6;
const size_t   MAX_ANALOGS = 5;
const int      DEBOUNCE_MSEC = 5;
const uint32_t SPI_TIMEOUT_MSEC = 1500;

const int analog_pins[] = {
    ANALOG_1_pin,
    ANALOG_2_pin,
    ANALOG_3_pin,
    ANALOG_4_pin,
    ANALOG_5_pin,
};
static const size_t analog_count = (&analog_pins)[1] - analog_pins;

static const uint8_t choice_LED_pins[] = {
    CHOICE_LED_1_pin,
    CHOICE_LED_2_pin,
    CHOICE_LED_3_pin,
    CHOICE_LED_4_pin,
    CHOICE_LED_5_pin,
    CHOICE_LED_6_pin,
};
static const size_t choice_LED_count = (&choice_LED_pins)[1] - choice_LED_pins;

static const uint8_t dest_button_pins[] = {
    DEST_BUTTON_1_pin,
    DEST_BUTTON_2_pin,
    DEST_BUTTON_3_pin,
    DEST_BUTTON_4_pin,
};
static const size_t dest_button_count =
    (&dest_button_pins)[1] - dest_button_pins;

typedef uint8_t message_buf[MSG_MAX];

typedef struct LED_state {
    uint8_t pin;
    uint8_t value;
} LED_state;

Bounce c_button(CHOICE_BUTTON_pin, DEBOUNCE_MSEC);
Bounce k1_button(DEST_BUTTON_1_pin, DEBOUNCE_MSEC);
Bounce k2_button(DEST_BUTTON_2_pin, DEBOUNCE_MSEC);
Bounce k3_button(DEST_BUTTON_3_pin, DEBOUNCE_MSEC);
Bounce k4_button(DEST_BUTTON_4_pin, DEBOUNCE_MSEC);
Bounce a_button(ASSIGN_BUTTON_pin, DEBOUNCE_MSEC);
LED_state c1_led = { CHOICE_LED_1_pin, 0 };
LED_state c2_led = { CHOICE_LED_2_pin, 0 };
LED_state c3_led = { CHOICE_LED_3_pin, 0 };
LED_state c4_led = { CHOICE_LED_4_pin, 0 };
LED_state c5_led = { CHOICE_LED_5_pin, 0 };
LED_state c6_led = { CHOICE_LED_6_pin, 0 };
LED_state a_led  = { ASSIGN_LED_pin,   0 };
UniWS LED_strand(MAX_PIXELS);
int analog_values[MAX_ANALOGS];
DMAChannel SPI_rx_dma;
DMAChannel SPI_tx_dma;

Bounce *const buttons[] = {
    &c_button,
    &a_button,
    &k1_button,
    &k2_button,
    &k3_button,
    &k4_button,
};
const size_t button_count = sizeof buttons / sizeof buttons[0];

message_buf recv_buf, send_buf;

#if USE_SERIAL

static bool serial_is_up;

static void begin_serial()
{
    int i;
    const int limit = 5000000;

    Serial.begin(9600);
    for (i = 0; i < limit && !Serial; i++)
        continue;
    if (i < limit) {
        serial_is_up = true;
        Serial.printf("Hello from Slave: %d\n", i);
    }
}

#define SERIAL_PRINTF(...) \
    (serial_is_up && Serial.printf(__VA_ARGS__))

#else

static void begin_serial() {}

#define SERIAL_PRINTF(fmt, ...) (0)

#endif

static void begin_slave()
{
    // Route clock.
    SIM_SCGC4 |= SIM_SCGC4_SPI0;

    // Enable pins.
    CORE_PIN10_CONFIG =                PORT_PCR_MUX(2); // CS0   = 10 (PTC4)
    CORE_PIN11_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2); // MOSI0 = 11 (PTC6)
    CORE_PIN12_CONFIG =                PORT_PCR_MUX(2); // MISO0 = 12 (PTC7)
    CORE_PIN13_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2); // SCK0  = 13 (PTC5)
}

static size_t do_transfer(uint8_t       *receive_buffer, size_t receive_count,
                          uint8_t const *send_buffer,    size_t send_count,
                          uint32_t       timeout_msec)
{
    uint32_t timeout_abs_msec = millis() + timeout_msec;

    // Disable SPI to reset to idle state.
    SPI0_C1 = 0;

    // Initialize SPI
    SPI0_C1 = SPI_C1_CPHA;
    SPI0_C2 = SPI_C2_RXDMAE | SPI_C2_TXDMAE;
    SPI0_ML = ETX;

    SPI_rx_dma.clearComplete();
    SPI_rx_dma.disableOnCompletion();
    SPI_rx_dma.source(SPI0_DL);
    SPI_rx_dma.destinationBuffer(receive_buffer, receive_count);
    SPI_rx_dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_RX);
    SPI_rx_dma.enable();

    SPI_tx_dma.clearComplete();
    SPI_tx_dma.disableOnCompletion();
    SPI_tx_dma.sourceBuffer(send_buffer, send_count);
    SPI_tx_dma.destination(SPI0_DL);
    SPI_tx_dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX);
    SPI_tx_dma.enable();

    // Wait for falling edge on SPI CS pin.
    while (digitalRead(SPI_CS_pin) != HIGH)
        if ((int32_t)(timeout_abs_msec - millis()) < 0)
            return 0;
    while (digitalRead(SPI_CS_pin) == HIGH)
        if ((int32_t)(timeout_abs_msec - millis()) < 0)
            return 0;

    // Enable SPI.
    SPI0_C1 |= SPI_C1_SPE;

    // Wait for rising edge on SPI CS pin.
    while (digitalRead(SPI_CS_pin) != HIGH)
        if ((int32_t)(timeout_abs_msec - millis()) < 0)
            return 0;

    // Disable SPI.
    SPI0_C1 = 0;

    // Disable DMA.
    SPI_rx_dma.disable();
    SPI_tx_dma.disable();
    size_t ri = (uint8_t *)SPI_rx_dma.destinationAddress() - receive_buffer;
    return ri;
}

static uint16_t fletcher16(const uint8_t *buf, size_t count)
{
    uint16_t sum0 = 0, sum1 = 0;
    for (size_t i = 0; i < count; i++) {
        sum0 += buf[i];
        if (sum0 >= 255)
            sum0 -= 255;
        sum1 += sum0;
        if (sum1 >= 255)
            sum1 -= 255;
    }
    return sum1 << 8 | sum0;
}

namespace {                     // declare these functions in an
                                // unnamed namespace to thwart the
                                // stupid Arduino source mangler.

    static bool message_valid(const message_buf buf, size_t count)
    {
        if (count < 6) {
            SERIAL_PRINTF("count too short: %u < 6\n", count);
            return false;
        }

        if (buf[0] != STX) {
            SERIAL_PRINTF("first byte is %#o, not STX\n", buf[0]);
            return false;
        }

        uint8_t cfg0 = buf[1];
        uint8_t cfg1 = buf[2];
        uint8_t pixel_count = cfg1 & 0x07;
        uint8_t analog_count = (cfg1 >> 3) & 0x07;
        if (pixel_count > MAX_PIXELS || analog_count > MAX_ANALOGS) {
            SERIAL_PRINTF("pixel_count = %d, analog_count = %d\n",
                          pixel_count, analog_count);
            return false;
        }
        size_t len = 6;    // minimal message: STX cfg cfg chk chk ETX
        for (uint8_t bit = 1; bit; bit <<= 1)
            if (cfg0 & bit)
                len++;           // one byte per PWM LED
        len += 3 * (cfg1 & 0x7); // three bytes per pixel
        if (len > MSG_MAX) {
            SERIAL_PRINTF("len = %d\n", len);
            return false;
        }

        if (count < len) {
            SERIAL_PRINTF("short message: %u < %u\n", count, len);
            return false;
        }

        if (buf[len - 1] != ETX) {
            SERIAL_PRINTF("buf[%d - 1] = \\%03o != ETX\n", len, buf[len - 1]);
            return false;
        }
        uint16_t rx_chk = (uint16_t)buf[len - 3] << 8 | buf[len - 2];
        uint16_t cc_chk = fletcher16(buf + 1, len - 4);
        if (rx_chk != cc_chk) {
            SERIAL_PRINTF("chk: got %#x, not %#x\n", rx_chk, cc_chk);
            SERIAL_PRINTF("    len = %u\n", len);
            for (int i = 0; i < len; i++)
                SERIAL_PRINTF("    %3d: %d\n", i, buf[i]);
            return false;
        }

        return true;
    }

    static void update_LED(LED_state *lp, uint8_t new_value)
    {
        if (lp->value != new_value) {
            analogWrite(lp->pin, new_value);
            lp->value = new_value;
        }
    }
};

static uint8_t update_analog(uint8_t index, uint8_t **pptr)
{
    int new_value = analogRead(analog_pins[index]);
    int old_value = analog_values[index];
    
    if (new_value >> 2 != old_value >> 2) {
        if (new_value > old_value + 1 || new_value < old_value - 1) {
            analog_values[index] = new_value;
            *(*pptr)++ = new_value >> 2;
            return 1 << index;
        }
    }
    return 0;
}

// static void print_buf(const char *label, const uint8_t *buf, size_t count)
// {
//     SERIAL_PRINTF("%s", label);
//     for (size_t i = 0; i < count; i++) {
//         uint8_t c = buf[i];
//         if (c == STX)
//             SERIAL_PRINTF(" STX");
//         else if (c == ETX)
//             SERIAL_PRINTF(" ETX");
//         else if (c == SYN)
//             SERIAL_PRINTF(" SYN");
//         else if (c >= ' ' && c < '\177')
//             SERIAL_PRINTF(" %c", c);
//         else
//             SERIAL_PRINTF(" \\%03o", c);
//     }
//     SERIAL_PRINTF("\n");
// }

static uint8_t biramp(uint32_t i, uint32_t n)
{
    if (i >= n/2)
        i = n - i - 1;
    if (i >= n/2)
        return 0;
    return i * 255 / (n/2 - 1);
}

static void self_test_loop()
{
    static uint32_t t;
    uint32_t now = millis();

    if ((int32_t)(t - now) > 0)
        return;
    t += 10;                    // 100 Hz loop

    // Choice buttons step 2 Hz.
    uint8_t choice = now / 500 % choice_LED_count;
    uint8_t choice_level = biramp(now % 500, 500);
    bool cb_down = digitalRead(CHOICE_BUTTON_pin) == LOW;
    for (size_t i = 0; i < choice_LED_count; i++)
        if (cb_down)
            analogWrite(choice_LED_pins[i], 10);
        else
            analogWrite(choice_LED_pins[i], (choice == i) * choice_level);

    // Assign button blink 10 Hz.
    uint8_t assign_level = biramp(now % 100, 100);
    bool ab_down = digitalRead(ASSIGN_BUTTON_pin) == LOW;
    if (ab_down)
        analogWrite(ASSIGN_LED_pin, 10);
    else
        analogWrite(ASSIGN_LED_pin, assign_level);

    // Strand cycles through R, G, B, 1/6 Hz.
    uint32_t npix = LED_strand.numPixels();
    uint32_t pt = now % 6000;
    uint8_t  component = pt / 2000;
    uint32_t  pixel = pt % 2000 / (2000 / npix);
    uint8_t  level = biramp(pt % (2000 / npix), 2000 / npix);

    for (uint32_t i = 0; i < npix; i++) {
        bool db_down = ((i - 1) < dest_button_count &&
                        digitalRead(dest_button_pins[i - 1]) == LOW);
        uint16_t analog = ((i - 1) < analog_count
                           ? analogRead(analog_pins[i - 1])
                           : 0);
        uint8_t pix_level = 0;
        if (db_down)
            pix_level = 63;
        else if (pixel == i)
            pix_level = level;
        uint8_t primary_level = pix_level * (1023 - analog) / 1023;
        uint8_t secondary_level = pix_level * analog / 1023;
        uint8_t r = (component == 0) ? primary_level : secondary_level;
        uint8_t g = (component == 1) ? primary_level : secondary_level;
        uint8_t b = (component == 2) ? primary_level : secondary_level;
        LED_strand.setPixel(i, r, g, b);
    }
    LED_strand.show();
}

void setup()
{
    begin_serial();

    pinMode(CHOICE_BUTTON_pin, INPUT_PULLUP);
    pinMode(DEST_BUTTON_1_pin, INPUT_PULLUP);
    pinMode(DEST_BUTTON_2_pin, INPUT_PULLUP);
    pinMode(DEST_BUTTON_3_pin, INPUT_PULLUP);
    pinMode(DEST_BUTTON_4_pin, INPUT_PULLUP);
    pinMode(ASSIGN_BUTTON_pin, INPUT_PULLUP);
    
    LED_strand.begin();

    begin_slave();

    // Initialize to empty message.
    memset(send_buf, SYN, sizeof send_buf);
}

void loop()
{
    static uint32_t alive_time;
    uint32_t timeout_msec = SPI_TIMEOUT_MSEC;
    if ((int32_t)(millis() - alive_time) > timeout_msec)
        timeout_msec = 10;

    size_t receive_count = do_transfer(recv_buf, MSG_MAX,
                                       send_buf, MSG_MAX,
                                       timeout_msec);
    // print_buf("received: ", recv_buf, MSG_MAX);
    if (receive_count == 0) {
        memset(send_buf, SYN, sizeof send_buf);
        self_test_loop();
        return;
    }
    if (!message_valid(recv_buf, receive_count)) {
        // SERIAL_PRINTF("invalid message received\n");
        memset(send_buf, SYN, sizeof send_buf);
        return;
    }
    alive_time = millis();

    uint8_t cfg0 = recv_buf[1];
    uint8_t cfg1 = recv_buf[2];
    const uint8_t *rx_ptr = recv_buf + 3;

    // Update the PWM LEDs
    update_LED(&c1_led, cfg0 & (1 << 0) ? *rx_ptr++ : 0);
    update_LED(&c2_led, cfg0 & (1 << 1) ? *rx_ptr++ : 0);
    update_LED(&c3_led, cfg0 & (1 << 2) ? *rx_ptr++ : 0);
    update_LED(&c4_led, cfg0 & (1 << 3) ? *rx_ptr++ : 0);
    update_LED(&c5_led, cfg0 & (1 << 4) ? *rx_ptr++ : 0);
    update_LED(&c6_led, cfg0 & (1 << 5) ? *rx_ptr++ : 0);
    update_LED(&a_led,  cfg0 & (1 << 7) ? *rx_ptr++ : 0);

    // Update the pixels.
    int pixel_count = cfg1 & 0x07;
    if (pixel_count) {
        LED_strand.clear();
        for (int i = 0; i < pixel_count; i++) {
            uint8_t r = *rx_ptr++;
            uint8_t g = *rx_ptr++;
            uint8_t b = *rx_ptr++;
            // SERIAL_PRINTF("  LED %d = %x %x %x\n", i, r, g, b);
            LED_strand.setPixel(i, r, g, b);
        }
        LED_strand.show();
    }

    uint8_t *tx_ptr = send_buf + 3;

    // Read analog inputs.
    uint8_t analog_mask = 0;
    int analog_count = (cfg1 >> 3) & 0x07;
    for (int i = 0; i < analog_count; i++)
        analog_mask |= update_analog(i, &tx_ptr);

    // Read buttons.
    uint8_t switch_mask = 0;
    for (size_t i = 0; i < button_count; i++) {
        Bounce *b = buttons[i];
        if (b->update() && b->fallingEdge())
            switch_mask |= 1 << i;
    }

    // compose outgoing message.
    send_buf[0] = STX;
    send_buf[1] = switch_mask;
    send_buf[2] = analog_mask;
    uint16_t chk = fletcher16(send_buf + 1, tx_ptr - send_buf - 1);
    *tx_ptr++ = chk >> 8;
    *tx_ptr++ = chk & 0xFF;
    *tx_ptr++ = ETX;
    while (tx_ptr < send_buf + MSG_MAX)
        *tx_ptr++ = SYN;
}
