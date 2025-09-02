// Only required headers
#include "wifi_portal.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_BMP280.h>
#include "logo.h"
// ...existing code...

#define LED_PIN 7  // ESP32-C3 onboard LED
#define PUMP_PIN 8 // Output pin for timer Avoid Boot 9
#define TFT_CS 10  // Chip select
#define TFT_DC 3   // Data/command
#define TFT_RST 2  // Reset
#define TFT_SCLK 4 // SCL (SPI clock)
#define TFT_MOSI 6 // SDA (SPI data)
#define I2C_SDA 18
#define I2C_SCL 19
#define OC_TEAL    0x6EFD
#define OC_LIGHT   0x7F1E

// Timer durations (ms)
unsigned long ledOnDuration = 5000;   // Default: 5s ON
unsigned long ledOffDuration = 2000;  // Default: 2s OFF
unsigned long pumpOnDuration = 5000;  // Default: 5s ON
unsigned long pumpOffDuration = 2000; // Default: 2s OFF

// Timer state variables
unsigned long ledPrevMillis = 0;
unsigned long pumpPrevMillis = 0;
bool timerLEDState = false;
bool timerPumpState = false;

// WiFi connection status (global for lambda access)

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_BMP280 bmp;

// Function to update timer durations
void setTimerDurations(unsigned long ledOn, unsigned long ledOff, unsigned long pumpOn, unsigned long pumpOff) {
    ledOnDuration = ledOn;
    ledOffDuration = ledOff;
    pumpOnDuration = pumpOn;
    pumpOffDuration = pumpOff;
    Serial.println("Timer durations updated:");
    Serial.printf("LED: %lums ON, %lums OFF\n", ledOnDuration, ledOffDuration);
    Serial.printf("Pump: %lums ON, %lums OFF\n", pumpOnDuration, pumpOffDuration);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== ESP32-C3 Starting ===");
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(PUMP_PIN, HIGH);
    Wire.begin(I2C_SDA, I2C_SCL);
    tft.initR(INITR_GREENTAB); // Most ST7735S modules use BLACKTAB
    tft.invertDisplay(1);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    // Draw logo centered (128x128 logo, display is 160x128)
    tft.drawRGBBitmap(16, 0, openchain_logo, 128, 128);
    delay(1500);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(OC_TEAL, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 30);
    tft.println("Hello Gardner!");
    tft.setTextSize(1);
    tft.setCursor(10, 40);
    unsigned long bmpStart = millis();
    bool bmpOk = false;
    while (millis() - bmpStart < 5000)
    {
        if (bmp.begin(0x76) || bmp.begin(0x77))
        {
            bmpOk = true;
            break;
        }
        delay(100);
    }
    if (bmpOk)
    {
        tft.println("Temp Sensor OK");
    }
    else
    {
        tft.println("BMP280 fail");
    }
    delay(1500);

    // Force stop any existing WiFi and start our AP
    Serial.println("Stopping existing WiFi...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);
    Serial.println("Starting AP mode...");
    WiFi.mode(WIFI_AP);
    bool apResult = WiFi.softAP("OpenChain", "12345");
    Serial.print("AP Start Result: ");
    Serial.println(apResult);
    Serial.print("AP SSID should be: OpenChain, actual: ");
    Serial.println(WiFi.softAPSSID());
    delay(1000);

    // Show WiFi setup instructions if needed
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(OC_TEAL, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 30);
    tft.println("Checking WiFi...");
    
    // Start WiFi portal (will connect if credentials exist, or start AP)
    Serial.println("Calling setupWifiPortal()...");
    setupWifiPortal();
    Serial.print("After setupWifiPortal - AP SSID: ");
    Serial.println(WiFi.softAPSSID());
    Serial.print("WiFi Mode: ");
    Serial.println(WiFi.getMode());
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());
    
    // Update display based on connection status
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(10, 30);
    if (WiFi.status() == WL_CONNECTED) {
        tft.println("WiFi Connected!");
    } else {
        tft.println("WiFi Setup Required");
        tft.setCursor(10, 45);
        tft.println("Connect to AP: OpenChain");
        tft.setCursor(10, 60);
        tft.println("Password: 12345");
        tft.setCursor(10, 75);
        tft.println("Open browser: 192.168.4.1");
        tft.setCursor(10, 90);
        tft.println("Enter WiFi credentials");
    }
}

void loop()
{
    // If in AP mode, block here and only handle WiFi portal
    if (WiFi.getMode() == WIFI_AP) {
        handleWifiPortal();
        delay(10);
        return;
    }
    
    unsigned long now = millis();
    
    // Handle WiFi portal if in AP mode
    handleWifiPortal();

    // Non-blocking timer logic (5s ON, 2s OFF)
    if (timerLEDState)
    {
        if (now - ledPrevMillis >= ledOnDuration)
        {
            digitalWrite(LED_PIN, LOW);
            timerLEDState = false;
            ledPrevMillis = now;
        }
    }
    else
    {
        if (now - ledPrevMillis >= ledOffDuration)
        {
            digitalWrite(LED_PIN, HIGH);
            timerLEDState = true;
            ledPrevMillis = now;
        }
    }

    if (timerPumpState)
    {
        if (now - pumpPrevMillis >= pumpOnDuration)
        {
            digitalWrite(PUMP_PIN, LOW);
            timerPumpState = false;
            pumpPrevMillis = now;
        }
    }
    else
    {
        if (now - pumpPrevMillis >= pumpOffDuration)
        {
            digitalWrite(PUMP_PIN, HIGH);
            timerPumpState = true;
            pumpPrevMillis = now;
        }
    }

    // Update display every 1000ms
    static unsigned long displayPrevMillis = 0;
    if (now - displayPrevMillis >= 1000)
    {
        displayPrevMillis = now;
        static bool bOnce = false;
        if (!bOnce)
        {
            tft.fillScreen(ST77XX_BLACK);
            bOnce = true;
        }
        tft.setTextSize(1);
        // Update display every 1000ms
        static unsigned long displayPrevMillis = 0;
        static float lastTemp = NAN;
        static float lastPres = NAN;
        static bool lastValid = false;
        if (now - displayPrevMillis >= 1000)
        {
            displayPrevMillis = now;
            float temp = bmp.readTemperature();
            float pres = bmp.readPressure() / 100.0F;
            bool tempValid = !isnan(temp);
            bool presValid = !isnan(pres);
            if (tempValid && presValid)
            {
                lastTemp = temp;
                lastPres = pres;
                lastValid = true;
            }
            // Only clear the region where text is drawn to reduce flicker
            tft.fillRect(0, 40, 160, 60, ST77XX_BLACK);
            tft.setTextSize(1);
            tft.setTextColor(OC_TEAL, ST77XX_BLACK);
            if (lastValid)
            {
                tft.setCursor(10, 30);
                tft.print("Temp: ");
                tft.print(lastTemp, 1);
                tft.print(" C");
                tft.setCursor(10, 45);
                tft.print("Pres: ");
                tft.print(lastPres, 1);
                tft.print(" hPa");
                if (!tempValid || !presValid)
                {
                    tft.setCursor(10, 65);
                    tft.print("Sensor error");
                }
            }
            else
            {
                tft.setCursor(10, 65);
                tft.print("No data yet");
            }
            // Show output status for LED and PUMP
            tft.setCursor(10, 60);
            tft.setTextColor(ST7735_YELLOW, ST77XX_BLACK);
            tft.print("LED: ");
            tft.print(digitalRead(LED_PIN) == HIGH ? "OFF" : "ON");
            tft.setCursor(60, 60);
            tft.print(" | ");
            tft.print("PUMP: ");
            tft.print(digitalRead(PUMP_PIN) == HIGH ? "OFF" : "ON");

                // ...existing code...
            // Show IP address if connected
            tft.setCursor(10, 90);
            tft.setTextColor(OC_LIGHT, ST77XX_BLACK);
            if (WiFi.status() == WL_CONNECTED) {
                tft.print("IP: ");
                tft.print(WiFi.localIP());
            } else {
                tft.print("IP: N/A");
            }
        }
    }
}
