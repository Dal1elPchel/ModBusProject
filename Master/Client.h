#pragma once
#include "SerialPort.h"
#include <vector>
#include <string>

class Client : public SerialPort
{
private:
    // Ожидание подтверждения от Slave
    void WaitForAck(char expected);
    // Отправка заголовка файла
    void SendHeader(uint64_t fileSize);
    // Отправка одного блока файла
    void SendChunk(const char* data, size_t size);

public:
    Client(const std::string& portName, int baudRate);

    void Send(const std::string& message);
    void SendFile(const std::string& filePath, size_t chunkSize = 1024);
};
