#include "MultiPCF8591.h"

MultiPCF8591::MultiPCF8591(uint8_t moduleCount, uint8_t firstAddress, TwoWire &wire)
    : _moduleCount(moduleCount > MAX_MODULES ? MAX_MODULES : moduleCount),
      _wire(&wire) {
    for (uint8_t i = 0; i < MAX_MODULES; ++i) {
        _modules[i].address = 0;
        _modules[i].control = 0;
        _modules[i].found = false;
    }

    for (uint8_t i = 0; i < _moduleCount; ++i) {
        _modules[i].address = firstAddress + i;
        _modules[i].control = 0;
        _modules[i].found = false;
    }
}

bool MultiPCF8591::_validAddress(uint8_t addressValue) const {
    return addressValue >= 0x48 && addressValue <= 0x4F;
}

uint8_t MultiPCF8591::moduleCount() const {
    return _moduleCount;
}

uint8_t MultiPCF8591::address(uint8_t module) const {
    if (!validModule(module)) {
        return 0;
    }
    return _modules[module].address;
}

bool MultiPCF8591::setAddress(uint8_t module, uint8_t addressValue) {
    if (!validModule(module) || !_validAddress(addressValue)) {
        return false;
    }

    for (uint8_t i = 0; i < _moduleCount; ++i) {
        if (i != module && _modules[i].address == addressValue) {
            return false;
        }
    }

    _modules[module].address = addressValue;
    _modules[module].found = false;
    return true;
}

bool MultiPCF8591::validModule(uint8_t module) const {
    return module < _moduleCount && _validAddress(_modules[module].address);
}

bool MultiPCF8591::moduleFound(uint8_t module) const {
    return validModule(module) && _modules[module].found;
}

bool MultiPCF8591::ping(uint8_t module) {
    if (!validModule(module)) {
        return false;
    }

    _wire->beginTransmission(_modules[module].address);
    uint8_t result = _wire->endTransmission();

    _modules[module].found = (result == 0);
    return _modules[module].found;
}

bool MultiPCF8591::begin() {
    bool allFound = true;

    for (uint8_t i = 0; i < _moduleCount; ++i) {
        if (!ping(i)) {
            allFound = false;
        }
    }

    return allFound;
}

uint8_t MultiPCF8591::_modeBits(InputMode mode) const {
    switch (mode) {
        case DIFFERENTIAL_3: return 0x10;
        case MIXED:          return 0x20;
        case DIFFERENTIAL_2: return 0x30;
        case SINGLE_ENDED:
        default:             return 0x00;
    }
}

uint8_t MultiPCF8591::_channelCount(InputMode mode) const {
    switch (mode) {
        case DIFFERENTIAL_3: return 3;
        case DIFFERENTIAL_2: return 2;
        case MIXED:          return 4;
        case SINGLE_ENDED:
        default:             return 4;
    }
}

bool MultiPCF8591::setInputMode(uint8_t module, InputMode mode) {
    if (!validModule(module) || mode > DIFFERENTIAL_2) {
        return false;
    }

    uint8_t control = _modules[module].control;
    control &= 0x40;                 // keep DAC enable; reset mode/channel/auto-increment
    control |= _modeBits(mode);
    _modules[module].control = control;

    return _writeControl(module, control);
}

MultiPCF8591::InputMode MultiPCF8591::inputMode(uint8_t module) const {
    if (!validModule(module)) {
        return SINGLE_ENDED;
    }

    return static_cast<InputMode>((_modules[module].control >> 4) & 0x03);
}

bool MultiPCF8591::_writeControl(uint8_t module, uint8_t controlByteValue) {
    if (!validModule(module)) {
        return false;
    }

    // Bits 7 and 3 are reserved and must remain zero.
    controlByteValue &= 0x77;

    _wire->beginTransmission(_modules[module].address);
    _wire->write(controlByteValue);
    uint8_t result = _wire->endTransmission();

    if (result == 0) {
        _modules[module].control = controlByteValue;
        _modules[module].found = true;
        return true;
    }

    _modules[module].found = false;
    return false;
}

bool MultiPCF8591::writeControl(uint8_t module, uint8_t controlByteValue) {
    return _writeControl(module, controlByteValue);
}

uint8_t MultiPCF8591::controlByte(uint8_t module) const {
    if (!validModule(module)) {
        return 0;
    }
    return _modules[module].control;
}

bool MultiPCF8591::_readBytes(uint8_t module, uint8_t *buffer, uint8_t count) {
    if (!validModule(module) || buffer == 0 || count == 0) {
        return false;
    }

    uint8_t received = _wire->requestFrom(_modules[module].address, count, true);

    if (received != count) {
        _modules[module].found = false;
        return false;
    }

    for (uint8_t i = 0; i < count; ++i) {
        if (!_wire->available()) {
            _modules[module].found = false;
            return false;
        }
        buffer[i] = static_cast<uint8_t>(_wire->read());
    }

    _modules[module].found = true;
    return true;
}

bool MultiPCF8591::_readSingle(uint8_t module, uint8_t channel, uint8_t &value) {
    if (!validModule(module) || channel > 3) {
        return false;
    }

    // Single-ended mode is the natural interpretation of channels 0..3.
    // In other modes the same channel number selects the device-defined
    // differential/mixed pair described by the datasheet.
    uint8_t control = _modules[module].control;
    control &= 0x70;             // preserve DAC enable + input mode; clear auto-increment/channel
    control |= (channel & 0x03);

    if (!_writeControl(module, control)) {
        return false;
    }

    uint8_t data[2];
    if (!_readBytes(module, data, 2)) {
        return false;
    }

    // The first byte is always the conversion result from the previous
    // read cycle. The second byte is the conversion requested here.
    value = data[1];
    return true;
}

uint8_t MultiPCF8591::analogRead(uint8_t module, uint8_t channel) {
    uint8_t value = 0;

    if (!_readSingle(module, channel, value)) {
        return 0;
    }

    return value;
}

uint8_t MultiPCF8591::analogRead(uint8_t virtualChannel) {
    if (_moduleCount == 0) {
        return 0;
    }

    uint8_t module = virtualChannel / 4;
    uint8_t channel = virtualChannel % 4;

    if (!validModule(module)) {
        return 0;
    }

    return analogRead(module, channel);
}

bool MultiPCF8591::readAll(uint8_t module, uint8_t values[4]) {
    if (!validModule(module) || values == 0) {
        return false;
    }

    if (inputMode(module) != SINGLE_ENDED) {
        return false;
    }

    // Select AIN0 and enable auto-increment. One control write followed by
    // one read of five bytes obtains the four requested conversions. The
    // first byte is the stale result from the preceding conversion cycle.
    uint8_t control = _modules[module].control;
    control &= 0x70;             // mode + DAC enable, channel/AI cleared
    control |= 0x04;             // auto-increment
    if (!_writeControl(module, control)) {
        return false;
    }

    uint8_t data[5];
    if (!_readBytes(module, data, 5)) {
        return false;
    }

    values[0] = data[1];
    values[1] = data[2];
    values[2] = data[3];
    values[3] = data[4];

    return true;
}

bool MultiPCF8591::analogWrite(uint8_t module, uint8_t value) {
    if (!validModule(module)) {
        return false;
    }

    // AOUT must be enabled (bit 6) for the DAC output amplifier to be active.
    uint8_t control = _modules[module].control | 0x40;
    control &= 0x77;

    _wire->beginTransmission(_modules[module].address);
    _wire->write(control);
    _wire->write(value);
    uint8_t result = _wire->endTransmission();

    if (result != 0) {
        _modules[module].found = false;
        return false;
    }

    _modules[module].control = control;
    _modules[module].found = true;
    return true;
}
