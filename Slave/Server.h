#pragma once
#include <windows.h>
#include <vector>
#include <string>

class Server
{
private:
    HANDLE hComPort;
    std::string comPortName;
    int baudRate;

    static inline size_t BUFFER_SIZE = 1024;

    bool receiveData(std::vector<char>& data, size_t bufferSize);
    bool sendAck(char ackType);

public:
    Server(const std::string& port, int rate);
    ~Server();

    bool openPort();
    bool receiveFile(const std::string& outputPath, size_t chunkSize = BUFFER_SIZE);
};
