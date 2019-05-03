
#include <Arduino.h>

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <U8x8lib.h>

#include "secrets/lorawan_keys.h"

// the OLED used
// ux8x(clock, data, reset)
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);


static uint8_t mydata[] = { 0x12, 0x34};
static osjob_t sendjob;
static int counter = 0;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60*5;
#define SPI_LORA_CLK 5
#define SPI_LORA_MISO 19
#define SPI_LORA_MOSI 27
#define LORA_RST 14
#define LORA_CS 18
#define LORA_DIO_0 26
#define LORA_DIO_1 34
#define LORA_DIO_2 35

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = LORA_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LORA_RST,
    .dio = {LORA_DIO_0, LORA_DIO_1, LORA_DIO_2},
};

void os_getArtEui(u1_t *buf)
{
    Serial.println(F("Get APPEUI"));
    memcpy_P(buf, APPEUI, 8);
}
void os_getDevEui(u1_t *buf)
{
    Serial.println(F("Get DEVEUI"));
    memcpy_P(buf, DEVEUI, 8);
}
void os_getDevKey(u1_t *buf)
{
    Serial.println(F("Get APPKEY"));
    memcpy_P(buf, APPKEY, 16);
}

void writeOled(const char *buf) {
    u8x8.clearLine(0);
    u8x8.drawString(0, 0, buf);
}

void do_send(osjob_t *j)
{
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND)
    {
        Serial.println(F("OP_TXRXPEND, not sending"));
    }
    else
    {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
        char buf[20];
        sprintf(buf,"Send %d",counter++);
        Serial.printf("%s\n",buf);
        writeOled(buf);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent(ev_t ev)
{
    Serial.print(os_getTime());
    Serial.print(": ");
    switch (ev)
    {
    case EV_SCAN_TIMEOUT:
        Serial.println(F("EV_SCAN_TIMEOUT"));
        break;
    case EV_BEACON_FOUND:
        Serial.println(F("EV_BEACON_FOUND"));
        break;
    case EV_BEACON_MISSED:
        Serial.println(F("EV_BEACON_MISSED"));
        break;
    case EV_BEACON_TRACKED:
        Serial.println(F("EV_BEACON_TRACKED"));
        break;
    case EV_JOINING:
        Serial.println(F("EV_JOINING"));
        writeOled("Joining ...");
        break;
    case EV_JOINED:
        Serial.println(F("EV_JOINED"));
        writeOled("Joined.");

        // Disable link check validation (automatically enabled
        // during join, but not supported by TTN at this time).
        LMIC_setLinkCheckMode(0);
        break;
    case EV_RFU1:
        Serial.println(F("EV_RFU1"));
        break;
    case EV_JOIN_FAILED:
        Serial.println(F("EV_JOIN_FAILED"));
        break;
    case EV_REJOIN_FAILED:
        Serial.println(F("EV_REJOIN_FAILED"));
        break;
        break;
    case EV_TXCOMPLETE:
        Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
        if (LMIC.txrxFlags & TXRX_ACK)
            Serial.println(F("Received ack"));
        if (LMIC.dataLen)
        {
            Serial.println(F("Received "));
            Serial.println(LMIC.dataLen);
            Serial.println(F(" bytes of payload"));
        }
        // Schedule next transmission
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
        break;
    case EV_LOST_TSYNC:
        Serial.println(F("EV_LOST_TSYNC"));
        break;
    case EV_RESET:
        Serial.println(F("EV_RESET"));
        break;
    case EV_RXCOMPLETE:
        // data received in ping slot
        Serial.println(F("EV_RXCOMPLETE"));
        break;
    case EV_LINK_DEAD:
        Serial.println(F("EV_LINK_DEAD"));
        break;
    case EV_LINK_ALIVE:
        Serial.println(F("EV_LINK_ALIVE"));
        break;
    default:
        Serial.println(F("Unknown event"));
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    u8x8.begin();
    //u8x8.setFont(u8x8_font_courB18_2x3_r);
    u8x8.setFont(u8x8_font_chroma48medium8_r);

    u8x8.drawString(0, 0, "OTAA 0.1");
    u8x8.drawString(0, 1, "Start ...");
    delay(3000);
    u8x8.clearDisplay();
    u8x8.drawString(0, 0, "Ready");
    Serial.println(F("OTAA Node v0.1"));

    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 10; i++)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
    }

    SPI.begin(SPI_LORA_CLK, SPI_LORA_MISO, SPI_LORA_MOSI);

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop()
{
    os_runloop_once();
}
