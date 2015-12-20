#include "UniWS.h"

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

const size_t MAX_PIXELS = 6;

static const uint8_t analog_pins[] = {
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

UniWS LED_strand(MAX_PIXELS);

static uint8_t biramp(uint32_t i, uint32_t n)
{
    if (i >= n/2)
        i = n - i - 1;
    if (i >= n/2)
        return 0;
    return i * 255 / (n/2 - 1);
}

void setup()
{
    Serial.begin(9600);
    for (int i = 0; i < 5000000 && !Serial; i++)
        continue;
    if (Serial)
        Serial.printf("Hello, World!\n");

    pinMode(CHOICE_BUTTON_pin, INPUT_PULLUP);
    pinMode(ASSIGN_BUTTON_pin, INPUT_PULLUP);
    for (size_t i = 0; i < dest_button_count; i++)
        pinMode(dest_button_pins[i], INPUT_PULLUP);
    
    for (size_t i = 0; i < choice_LED_count; i++)
        pinMode(choice_LED_pins[i], OUTPUT);
    pinMode(ASSIGN_LED_pin, OUTPUT);

    LED_strand.begin();
}

void loop()
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
