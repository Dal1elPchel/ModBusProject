#pragma once
#include "SerialPort.h"
#include <windows.h>
#include <vector>
#include <string>

class Server : public SerialPort
{
private:
    void sendAck(char ackType);

public:
    Server(const std::string& portName, int baudRate);

    void receiveFile(const std::string& outputPath, size_t chunkSize = 1024);
};
