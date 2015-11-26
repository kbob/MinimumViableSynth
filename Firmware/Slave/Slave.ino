#include <Bounce.h>
#include <SPI.h>
#include <UniWS.h>

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

const uint8_t STX = '\02';
const uint8_t ETX = '\03';
const uint8_t SYN = '\26';

const size_t MSG_MAX = 32;
const int DEBOUNCE_MSEC = 5;
const size_t MAX_PIXELS = 6;
const size_t MAX_ANALOGS = 5;

const int analog_pins[] = {
    ANALOG_1_pin,
    ANALOG_2_pin,
    ANALOG_3_pin,
    ANALOG_4_pin,
    ANALOG_5_pin,
};

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

void begin_slave()
{
    // Route clock.
    SIM_SCGC4 |= SIM_SCGC4_SPI0;

    // Enable pins.
    CORE_PIN10_CONFIG =                PORT_PCR_MUX(2); // CS0   = 10 (PTC4)
    CORE_PIN11_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2); // MOSI0 = 11 (PTC6)
    CORE_PIN12_CONFIG =                PORT_PCR_MUX(2); // MISO0 = 12 (PTC7)
    CORE_PIN13_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2); // SCK0  = 13 (PTC5)
}

size_t do_transfer(uint8_t *receive_buffer, size_t receive_count,
                   uint8_t *send_buffer,    size_t send_count)
{
    // Disable SPI to reset to idle state.
    SPI0_C1 = 0;

    // Initialize SPI
    SPI0_C1 = SPI_C1_SPE | SPI_C1_CPHA;
    SPI0_C2 = 0;
    SPI0_ML = ETX;
    size_t ri = 0, si = 0;
    uint32_t t0 = micros();
    while (ri < receive_count || si < send_count) {
        uint32_t t1 = micros();
        if (si > 2 && t1 - t0 > 100) {
            // timeout
            break;
        }
        uint8_t s = SPI0_S;
        if (s & SPI_S_SPTEF) {
            // transmit buffer empty
            uint8_t c = SYN;
            if (si < send_count)
                c = send_buffer[si++];
            if (si == 3)
                __disable_irq();
            SPI0_DL = c;
            t0 = t1;
        }
        if (s & SPI_S_SPRF) {
            // receive buffer full
            uint8_t c = SPI0_DL;
            if (ri < receive_count)
                receive_buffer[ri++] = c;
            t0 = t1;
        }
    }
    if (si >= 3)
        __enable_irq();
    return ri;
}

uint16_t fletcher16(const uint8_t *buf, size_t count)
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

    bool message_valid(const message_buf buf)
    {
        if (buf[0] != STX) {
            Serial.printf("buf[0] = \\%03o != STX\n", buf[0]);
            return false;
        }

        uint8_t cfg0 = buf[1];
        uint8_t cfg1 = buf[2];
        uint8_t pixel_count = cfg1 & 0x07;
        uint8_t analog_count = (cfg1 >> 3) & 0x07;
        if (pixel_count > MAX_PIXELS || analog_count > MAX_ANALOGS) {
            Serial.printf("pixel_count = %d, analog_count = %d\n",
                          pixel_count, analog_count);
            return false;
        }
        size_t len = 6;             // minimal message: STX cfg cfg chk chk ETX
        for (uint8_t bit = 1; bit; bit <<= 1)
            if (cfg0 & bit)
                len++;              // one byte per PWM LED
        len += 3 * (cfg1 & 0x7);   // three bytes per pixel
        if (len > MSG_MAX) {
            Serial.printf("len = %d\n", len);
            return false;
        }

        if (buf[len - 1] != ETX) {
            Serial.printf("buf[%d - 1] = \\%03o != ETX\n", len, buf[len - 1]);
            return false;
        }
        uint16_t chk = (uint16_t)buf[len - 3] << 8 | buf[len - 2];
        if (chk != fletcher16(buf + 1, len - 4)) {
            Serial.printf("chk = %#x != %#x\n",
                          chk, fletcher16(buf + 1, len - 3));
            return false;
        }

        Serial.printf("valid message received\n");
        return true;
    }

    void update_LED(LED_state *lp, uint8_t new_value)
    {
        if (lp->value != new_value) {
            analogWrite(lp->pin, new_value);
            lp->value = new_value;
        }
    }
};

uint8_t update_analog(uint8_t index, uint8_t **pptr)
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

static void print_buf(const char *label, const uint8_t *buf, size_t count)
{
    Serial.printf("%s", label);
    for (size_t i = 0; i < count; i++) {
        uint8_t c = buf[i];
        if (c == STX)
            Serial.printf(" STX");
        else if (c == ETX)
            Serial.printf(" ETX");
        else if (c == SYN)
            Serial.printf(" SYN");
        else if (c >= ' ' && c < '\177')
            Serial.printf(" %c", c);
        else
            Serial.printf(" \\%03o", c);
    }
    Serial.printf("\n");
}

void setup()
{
    Serial.begin(9600);
    while (!Serial)
        continue;
    Serial.printf("Hello from Slave\n");

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
    do_transfer(recv_buf, MSG_MAX, send_buf, MSG_MAX);
    print_buf("received: ", recv_buf, MSG_MAX);
    if (!message_valid(recv_buf)) {
        Serial.printf("invalid message received\n");
        memset(send_buf, SYN, sizeof send_buf);
        return;
    }

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
    uint16_t chk = fletcher16(send_buf + 1, (tx_ptr - send_buf) - 1);
    *tx_ptr++ = chk >> 8;
    *tx_ptr++ = chk & 0xFF;
    *tx_ptr++ = ETX;
    while (tx_ptr < send_buf + MSG_MAX)
        *tx_ptr++ = SYN;
}
