#include <Wire.h>
#include <MultiPCF8591.h>

// Three modules: 0x48, 0x49 and 0x4A.
MultiPCF8591 io(3, 0x48);

void setup() {
    Serial.begin(115200);

    // Choose the I2C pins appropriate for your board.
    // ESP8266 example:
    // Wire.begin(D2, D1);
    //
    // ESP32 example:
    // Wire.begin(21, 22);
    //
    // Generic Arduino:
    Wire.begin();

    if (!io.begin()) {
        Serial.println("One or more PCF8591 modules were not found.");
    }

    for (uint8_t module = 0; module < io.moduleCount(); ++module) {
        Serial.print("Module ");
        Serial.print(module);
        Serial.print(" @ 0x");
        Serial.print(io.address(module), HEX);
        Serial.print(": ");
        Serial.println(io.moduleFound(module) ? "OK" : "NOT FOUND");
    }

    // Enable DAC and set module 0 to approximately half scale.
    if (io.moduleFound(0)) {
        io.analogWrite(0, 128);
    }
}

void loop() {
    for (uint8_t module = 0; module < io.moduleCount(); ++module) {
        uint8_t values[4];

        if (!io.moduleFound(module)) {
            continue;
        }

        if (io.readAll(module, values)) {
            Serial.print("M");
            Serial.print(module);
            Serial.print(": ");

            for (uint8_t channel = 0; channel < 4; ++channel) {
                Serial.print(values[channel]);
                if (channel < 3) {
                    Serial.print('\t');
                }
            }

            Serial.println();
        }
    }

    delay(250);
}
