#pragma once
#include "SerialPort.h"
#include <vector>

class Master : public SerialPort
{
private:
    uint16_t CalculateCRC(const uint8_t* data, size_t length);
    std::vector<uint8_t> SendRequest(std::vector<uint8_t> request, size_t length);

public:
    Master(const std::string& portName, int baudRate);

    std::vector<uint16_t> ReadHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
};
