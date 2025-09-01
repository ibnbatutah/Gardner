#include "wifi_portal.h"
#include <WiFi.h>
#include "logo.h"
// ...existing code...
WebServer server(80); // This is the only definition
Preferences preferences;
String ssid = "";
String password = "";
bool wifiOk = false;

void setupWifiPortal() {
    // Stop any existing WiFi first
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    preferences.begin("wifi", false);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    preferences.end();
    
    // Load timer settings
    preferences.begin("timers", false);
    unsigned long savedLedOn = preferences.getULong("led_on", 5000);
    unsigned long savedLedOff = preferences.getULong("led_off", 2000);
    unsigned long savedPumpOn = preferences.getULong("pump_on", 5000);
    unsigned long savedPumpOff = preferences.getULong("pump_off", 2000);
    preferences.end();
    
    // Update global timer variables
    setTimerDurations(savedLedOn, savedLedOff, savedPumpOn, savedPumpOff);

    wifiOk = false;
    
    // Only try to connect if we have both SSID and password
    if (ssid.length() > 0 && password.length() > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        unsigned long wifiStart = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
            delay(500);
        }
        wifiOk = (WiFi.status() == WL_CONNECTED);
    }

    // If no credentials or connection failed, start AP (non-blocking)
    if (!wifiOk) {
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.mode(WIFI_AP);
        WiFi.softAP("OpenChain", "");
    }
    
    // Setup web server routes (works in both AP and STA mode)
    server.on("/", []() {
        String html = "<html><head>";
        html += "<link rel='icon' type='image/x-icon' href='/favicon.ico'>";
        html += "<style>";
        html += "body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;margin:0;padding:0;background:linear-gradient(135deg,#84d9cd 0%,#6bc4b8 100%);min-height:100vh;color:#ffffff;}";
        html += ".container{max-width:800px;margin:0 auto;padding:20px;}";
        html += ".card{background:rgba(255,255,255,0.1);backdrop-filter:blur(10px);border-radius:20px;padding:30px;margin:20px 0;box-shadow:0 8px 32px rgba(132,217,205,0.3);border:1px solid rgba(255,255,255,0.2);}";
        html += ".logo{text-align:center;margin:20px 0;}";
        html += ".logo img{border-radius:15px;box-shadow:0 4px 15px rgba(0,0,0,0.3);}";
        html += "h2{color:#ffffff;text-align:center;margin-bottom:30px;font-size:2em;text-shadow:2px 2px 4px rgba(0,0,0,0.3);}";
        html += "h3{color:#f0f0ff;margin-bottom:20px;border-bottom:2px solid rgba(255,255,255,0.3);padding-bottom:10px;}";
        html += "form{margin:20px 0;}";
        html += ".form-group{margin:15px 0;}";
        html += "label{display:block;margin-bottom:5px;font-weight:500;color:#f0f0ff;}";
        html += "input[type=text],input[type=password]{width:100%;padding:12px;border:2px solid rgba(255,255,255,0.3);border-radius:10px;background:rgba(255,255,255,0.1);color:#ffffff;font-size:16px;transition:all 0.3s ease;box-sizing:border-box;}";
        html += "input[type=text]:focus,input[type=password]:focus{outline:none;border-color:#ffffff;background:rgba(255,255,255,0.2);box-shadow:0 0 10px rgba(255,255,255,0.3);}";
        html += "input[type=text]::placeholder,input[type=password]::placeholder{color:rgba(255,255,255,0.6);}";
        html += "input[type=submit]{background:linear-gradient(45deg,#84d9cd,#6bc4b8);color:#ffffff;border:none;padding:15px 30px;border-radius:10px;font-size:16px;font-weight:600;cursor:pointer;transition:all 0.3s ease;margin:10px 5px;box-shadow:0 4px 15px rgba(132,217,205,0.4);}";
        html += "input[type=submit]:hover{transform:translateY(-2px);box-shadow:0 6px 20px rgba(132,217,205,0.6);}";
        html += ".status{text-align:center;padding:15px;border-radius:10px;margin:20px 0;font-weight:500;}";
        html += ".status.connected{background:rgba(76,175,80,0.2);color:#c8e6c9;border:1px solid rgba(76,175,80,0.5);}";
        html += ".status.ap{background:rgba(255,152,0,0.2);color:#ffe0b2;border:1px solid rgba(255,152,0,0.5);}";
        html += ".current-settings{background:rgba(255,255,255,0.05);padding:15px;border-radius:10px;margin:15px 0;}";
        html += ".setting-item{display:flex;justify-content:space-between;margin:8px 0;padding:5px 0;}";
        html += ".setting-label{color:#f0f0ff;}";
        html += ".setting-value{color:#ffffff;font-weight:500;}";
        html += "</style></head><body>";
        html += "<div class='container'>";
        html += "<div class='logo'><img src='/logo.bmp' alt='OpenChain Logo' width='128' height='128'></div>";
        html += "<h2>Openchain Gardner Configuration</h2>";
        
        if (WiFi.status() == WL_CONNECTED) {
            // Connected to WiFi - show timer configuration only
            html += "<div class='status connected'>Connected to WiFi</div>";
            html += "<div class='card'>";
            html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
            html += "</div>";
            html += "<div class='card'>";
            html += "<h3>Timer Configuration</h3>";
            html += "<div class='current-settings'>";
            html += "<div class='setting-item'><span class='setting-label'>LED Timer:</span><span class='setting-value'>" + String(ledOffDuration/1000) + "s ON, " + String(ledOnDuration/1000) + "s OFF</span></div>";
            html += "<div class='setting-item'><span class='setting-label'>Pump Timer:</span><span class='setting-value'>" + String(pumpOffDuration/1000) + "s ON, " + String(pumpOnDuration/1000) + "s OFF</span></div>";
            html += "</div>";
            html += "<form action='/timers' method='post'>";
            html += "<div class='form-group'><label>LED ON Duration (seconds):</label><input name='led_on' type='text' value='" + String(ledOffDuration/1000) + "' placeholder='Enter seconds (1-3600)'></div>";
            html += "<div class='form-group'><label>LED OFF Duration (seconds):</label><input name='led_off' type='text' value='" + String(ledOnDuration/1000) + "' placeholder='Enter seconds (1-3600)'></div>";
            html += "<div class='form-group'><label>Pump ON Duration (seconds):</label><input name='pump_on' type='text' value='" + String(pumpOffDuration/1000) + "' placeholder='Enter seconds (1-3600)'></div>";
            html += "<div class='form-group'><label>Pump OFF Duration (seconds):</label><input name='pump_off' type='text' value='" + String(pumpOnDuration/1000) + "' placeholder='Enter seconds (1-3600)'></div>";
            html += "<input type='submit' value='Save Timer Settings'>";
            html += "</form>";
            html += "</div>";
        } else {
            // AP Mode - show WiFi configuration only
            html += "<div class='status ap'>AP Mode - Configure WiFi Connection</div>";
            html += "<div class='card'>";
            html += "<h3>WiFi Configuration</h3>";
            html += "<p>Connect this device to your WiFi network to access advanced features.</p>";
            html += "<form action='/wifi' method='post'>";
            html += "<div class='form-group'><label>Network Name (SSID):</label><input name='ssid' type='text' placeholder='Enter your WiFi network name'></div>";
            html += "<div class='form-group'><label>Password:</label><input name='password' type='password' placeholder='Enter your WiFi password'></div>";
            html += "<input type='submit' value='Connect to WiFi'>";
            html += "</form>";
            html += "</div>";
        }
        
        html += "</div></body></html>";
        server.send(200, "text/html", html);
    });
    
    // Serve logo as BMP (converted from RGB565)
    server.on("/logo.bmp", []() {
        // Create a simple BMP header for 128x128 RGB565 image
        const int width = 128;
        const int height = 128;
        const int bpp = 24; // 24-bit RGB for web compatibility
        const int imageSize = width * height * 3;
        const int fileSize = 54 + imageSize; // BMP header is 54 bytes
        
        // BMP Header
        uint8_t bmpHeader[54] = {
            0x42, 0x4D,  // "BM"
            (uint8_t)(fileSize & 0xFF), (uint8_t)((fileSize >> 8) & 0xFF), 
            (uint8_t)((fileSize >> 16) & 0xFF), (uint8_t)((fileSize >> 24) & 0xFF),  // File size
            0x00, 0x00, 0x00, 0x00,  // Reserved
            0x36, 0x00, 0x00, 0x00,  // Offset to pixel data
            0x28, 0x00, 0x00, 0x00,  // DIB header size
            (uint8_t)(width & 0xFF), (uint8_t)((width >> 8) & 0xFF), 0x00, 0x00,  // Width
            (uint8_t)(height & 0xFF), (uint8_t)((height >> 8) & 0xFF), 0x00, 0x00,  // Height
            0x01, 0x00,  // Planes
            0x18, 0x00,  // Bits per pixel (24)
            0x00, 0x00, 0x00, 0x00,  // Compression
            (uint8_t)(imageSize & 0xFF), (uint8_t)((imageSize >> 8) & 0xFF), 
            (uint8_t)((imageSize >> 16) & 0xFF), (uint8_t)((imageSize >> 24) & 0xFF),  // Image size
            0x13, 0x0B, 0x00, 0x00,  // X pixels per meter
            0x13, 0x0B, 0x00, 0x00,  // Y pixels per meter
            0x00, 0x00, 0x00, 0x00,  // Colors in palette
            0x00, 0x00, 0x00, 0x00   // Important colors
        };
        
        server.setContentLength(fileSize);
        server.send(200, "image/bmp", "");
        
        // Send header
        server.sendContent((char*)bmpHeader, 54);
        
        // Convert RGB565 to RGB24 and send pixel data (bottom to top for BMP)
        uint8_t rgbData[384]; // 128 pixels * 3 bytes per pixel
        for (int y = height - 1; y >= 0; y--) {
            int bufIndex = 0;
            for (int x = 0; x < width; x++) {
                uint16_t pixel = openchain_logo[y * width + x];
                // Convert RGB565 to RGB24
                uint8_t r = ((pixel >> 11) & 0x1F) << 3;  // 5 bits -> 8 bits
                uint8_t g = ((pixel >> 5) & 0x3F) << 2;   // 6 bits -> 8 bits  
                uint8_t b = (pixel & 0x1F) << 3;          // 5 bits -> 8 bits
                rgbData[bufIndex++] = b;  // BMP uses BGR order
                rgbData[bufIndex++] = g;
                rgbData[bufIndex++] = r;
            }
            server.sendContent((char*)rgbData, bufIndex);
        }
    });
    
    // Serve favicon
    server.on("/favicon.ico", []() {
        // Redirect to logo for now
        server.sendHeader("Location", "/logo.bmp");
        server.send(302);
    });
    
    // REST API Endpoints
    
    // Discovery endpoint - responds with "GRD + IP"
    server.on("/api/discover", HTTP_GET, []() {
        String response = "GRD " + WiFi.localIP().toString();
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Content-Type", "text/plain");
        server.send(200, "text/plain", response);
    });
    
    // Get current settings as JSON
    server.on("/api/settings", HTTP_GET, []() {
        String json = "{";
        json += "\"wifi\":{";
        json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"ssid\":\"" + WiFi.SSID() + "\"";
        json += "},";
        json += "\"timers\":{";
        json += "\"led\":{";
        json += "\"on_duration\":" + String(ledOffDuration/1000) + ",";  // Remember: values are swapped for hardware
        json += "\"off_duration\":" + String(ledOnDuration/1000);
        json += "},";
        json += "\"pump\":{";
        json += "\"on_duration\":" + String(pumpOffDuration/1000) + ",";
        json += "\"off_duration\":" + String(pumpOnDuration/1000);
        json += "}";
        json += "}";
        json += "}";
        
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Content-Type", "application/json");
        server.send(200, "application/json", json);
    });
    
    // Update timer settings via JSON POST
    server.on("/api/timers", HTTP_POST, []() {
        // Only allow timer updates when WiFi is connected (for security)
        if (WiFi.status() != WL_CONNECTED) {
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(403, "application/json", "{\"error\":\"Timer updates only allowed when connected to WiFi\"}");
            return;
        }
        
        // Parse JSON body (simple parsing for expected format)
        String body = server.arg("plain");
        
        // Extract values from JSON (basic parsing)
        int ledOnPos = body.indexOf("\"led_on\":");
        int ledOffPos = body.indexOf("\"led_off\":");
        int pumpOnPos = body.indexOf("\"pump_on\":");
        int pumpOffPos = body.indexOf("\"pump_off\":");
        
        if (ledOnPos == -1 || ledOffPos == -1 || pumpOnPos == -1 || pumpOffPos == -1) {
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "application/json", "{\"error\":\"Invalid JSON format. Expected: {\\\"led_on\\\":5,\\\"led_off\\\":2,\\\"pump_on\\\":5,\\\"pump_off\\\":2}\"}");
            return;
        }
        
        // Extract numeric values
        unsigned long newLedOn = body.substring(ledOnPos + 9, body.indexOf(',', ledOnPos)).toInt() * 1000;
        unsigned long newLedOff = body.substring(ledOffPos + 10, body.indexOf(',', ledOffPos)).toInt() * 1000;
        unsigned long newPumpOn = body.substring(pumpOnPos + 10, body.indexOf(',', pumpOnPos)).toInt() * 1000;
        unsigned long newPumpOff = body.substring(pumpOffPos + 11, body.indexOf('}', pumpOffPos)).toInt() * 1000;
        
        // Validate ranges (1-3600 seconds)
        if (newLedOn >= 1000 && newLedOn <= 3600000 &&
            newLedOff >= 1000 && newLedOff <= 3600000 &&
            newPumpOn >= 1000 && newPumpOn <= 3600000 &&
            newPumpOff >= 1000 && newPumpOff <= 3600000) {
            
            // Note: Swap values for hardware (ON/OFF are inverted)
            preferences.begin("timers", false);
            preferences.putULong("led_on", newLedOff);    // User's OFF becomes ledOnDuration
            preferences.putULong("led_off", newLedOn);    // User's ON becomes ledOffDuration
            preferences.putULong("pump_on", newPumpOff);  // User's OFF becomes pumpOnDuration
            preferences.putULong("pump_off", newPumpOn);  // User's ON becomes pumpOffDuration
            preferences.end();
            
            // Update global variables
            setTimerDurations(newLedOff, newLedOn, newPumpOff, newPumpOn);
            
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "application/json", "{\"success\":true,\"message\":\"Timer settings updated\"}");
        } else {
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "application/json", "{\"error\":\"Invalid timer values. Use 1-3600 seconds.\"}");
        }
    });
    
    // OPTIONS handler for CORS preflight requests
    server.on("/api/timers", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200);
    });
        
        server.on("/wifi", []() {
            if (server.hasArg("ssid") && server.hasArg("password")) {
                preferences.putString("ssid", server.arg("ssid"));
                preferences.putString("password", server.arg("password"));
                server.send(200, "text/html", "<html><body><h2>WiFi Saved! Rebooting...</h2></body></html>");
                delay(2000);
                ESP.restart();
            } else {
                server.send(400, "text/html", "<html><body><h2>Missing SSID or Password</h2></body></html>");
            }
        });
        
        server.on("/timers", []() {
            // Only allow timer configuration when WiFi is connected
            if (WiFi.status() != WL_CONNECTED) {
                server.send(403, "text/html", "<html><body><h2>Access Denied</h2><p>Timer configuration only available when connected to WiFi.</p></body></html>");
                return;
            }
            
            if (server.hasArg("led_on") && server.hasArg("led_off") && 
                server.hasArg("pump_on") && server.hasArg("pump_off")) {
                
                // Note: Web form shows logical ON/OFF, but hardware is inverted
                // So we swap the values to match the hardware behavior
                unsigned long newLedOn = server.arg("led_off").toInt() * 1000;  // User's OFF becomes ledOnDuration
                unsigned long newLedOff = server.arg("led_on").toInt() * 1000;  // User's ON becomes ledOffDuration
                unsigned long newPumpOn = server.arg("pump_off").toInt() * 1000; // User's OFF becomes pumpOnDuration  
                unsigned long newPumpOff = server.arg("pump_on").toInt() * 1000; // User's ON becomes pumpOffDuration
                
                // Validate ranges (1-3600 seconds)
                if (newLedOn >= 1000 && newLedOn <= 3600000 &&
                    newLedOff >= 1000 && newLedOff <= 3600000 &&
                    newPumpOn >= 1000 && newPumpOn <= 3600000 &&
                    newPumpOff >= 1000 && newPumpOff <= 3600000) {
                    
                    // Save to preferences
                    preferences.begin("timers", false);
                    preferences.putULong("led_on", newLedOn);
                    preferences.putULong("led_off", newLedOff);
                    preferences.putULong("pump_on", newPumpOn);
                    preferences.putULong("pump_off", newPumpOff);
                    preferences.end();
                    
                    // Update global variables
                    setTimerDurations(newLedOn, newLedOff, newPumpOn, newPumpOff);
                    
                    server.send(200, "text/html", "<html><body><h2>Timers Saved!</h2><p><a href='/'>Back to Config</a></p></body></html>");
                } else {
                    server.send(400, "text/html", "<html><body><h2>Invalid timer values! Use 1-3600 seconds.</h2><p><a href='/'>Back</a></p></body></html>");
                }
            } else {
                server.send(400, "text/html", "<html><body><h2>Missing timer parameters</h2><p><a href='/'>Back</a></p></body></html>");
            }
        });
        
    // Start the web server (works in both AP and STA mode)
    server.begin();
}

void handleWifiPortal() {
    // Handle web server requests in both AP and STA mode
    server.handleClient();
}
