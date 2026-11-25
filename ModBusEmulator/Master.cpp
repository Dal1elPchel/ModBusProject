#include "Master.h"

using namespace std;

uint16_t Master::CalculateCRC(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < length; i++)
    {
        crc^= *(data + i);
        for (int j = 0; j < 8; j++)
        {
            if ((crc & 1) != 0)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

vector<uint8_t> Master::SendRequest(vector<uint8_t> request, size_t length)
{
    uint16_t reqCrc = CalculateCRC(request.data(), request.size());
    request.push_back(reqCrc & 0xFF); // Младший байт
    request.push_back(reqCrc >> 8); // Старший байт

    SendBytes(request);

    std::vector<uint8_t> response(length);
    ReceiveBytes(response.data(), length);

    if (response[0] != request[0])
        throw runtime_error("Wrong Slave ID in response");

    if (response[1] != request[1] && response[1] != (request[1] | 0x80))
        throw runtime_error("Wrong Function Code in response");

    uint16_t respCrc = CalculateCRC(response.data(), response.size() - 2);
    uint16_t receivedCrc = response[response.size() - 2] | (response[response.size() - 1] << 8);

    if (respCrc != receivedCrc) {
        throw runtime_error("CRC mismatch in response! Data corrupted.");
    }

    return response;
}

Master::Master(const string& portName, int baudRate)
    : SerialPort(portName, baudRate) {}

vector<uint16_t> Master::ReadHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity)
{
    vector<uint8_t> request;

    request.push_back(slaveId);
    request.push_back(0x03); // Функция 03
    request.push_back(startAddress >> 8);
    request.push_back(startAddress & 0xFF);
    request.push_back(quantity >> 8);
    request.push_back(quantity & 0xFF);

    size_t responseLen = 3 + (quantity * 2) + 2;
    vector<uint8_t> response = SendRequest(request, responseLen);

    if (response[1] & 0x80) 
    {
        throw runtime_error("Modbus Exception from slave: Code " + to_string(response[2]));
    }

    vector<uint16_t> result;
    size_t dataStartIndex = 3; // Первые 3 байта: ID, FC, ByteCount

    for (size_t i = 0; i < quantity; ++i) 
    {
        uint16_t value = (response[dataStartIndex + i * 2] << 8) | response[dataStartIndex + i * 2 + 1];
        result.push_back(value);
    }

    return result;
}
