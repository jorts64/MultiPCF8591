#ifndef MULTI_PCF8591_H
#define MULTI_PCF8591_H

#include <Arduino.h>
#include <Wire.h>

class MultiPCF8591 {
public:
    static const uint8_t MAX_MODULES = 8;
    static const uint8_t DEFAULT_ADDRESS = 0x48;

    enum InputMode {
        SINGLE_ENDED = 0,
        DIFFERENTIAL_3 = 1,
        MIXED = 2,
        DIFFERENTIAL_2 = 3
    };

    MultiPCF8591(uint8_t moduleCount, uint8_t firstAddress = DEFAULT_ADDRESS,
                 TwoWire &wire = Wire);

    bool begin();
    uint8_t moduleCount() const;
    uint8_t address(uint8_t module) const;
    bool setAddress(uint8_t module, uint8_t address);
    bool validModule(uint8_t module) const;
    bool moduleFound(uint8_t module) const;

    bool setInputMode(uint8_t module, InputMode mode);
    InputMode inputMode(uint8_t module) const;

    uint8_t analogRead(uint8_t module, uint8_t channel);
    uint8_t analogRead(uint8_t virtualChannel);
    bool readAll(uint8_t module, uint8_t values[4]);

    bool analogWrite(uint8_t module, uint8_t value);

    bool writeControl(uint8_t module, uint8_t controlByte);
    uint8_t controlByte(uint8_t module) const;

    bool ping(uint8_t module);

private:
    struct Module {
        uint8_t address;
        uint8_t control;
        bool found;
    };

    Module _modules[MAX_MODULES];
    uint8_t _moduleCount;
    TwoWire *_wire;

    bool _validAddress(uint8_t address) const;
    bool _writeControl(uint8_t module, uint8_t controlByte);
    bool _readBytes(uint8_t module, uint8_t *buffer, uint8_t count);
    bool _readSingle(uint8_t module, uint8_t channel, uint8_t &value);
    uint8_t _modeBits(InputMode mode) const;
    uint8_t _channelCount(InputMode mode) const;
};

#endif
