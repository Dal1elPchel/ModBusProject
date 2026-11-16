#include "Server.h"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <limits>

using namespace std;

Server::Server(const string& portName, int baudRate) : SerialPort(portName, baudRate) {}

void Server::sendAck(char ackType)
{
    string ack(1, ackType);
    SendBytes(ack);
}

void Server::receiveFile(const string& outputPath, size_t chunkSize)
{
    if (chunkSize == 0)
    {
        throw invalid_argument("Chunk size cannot be zero");
    }

    ofstream outputFile(outputPath, ios::binary);

    if (!outputFile.is_open()) 
    {
        throw runtime_error("Cannot open file for writing: " + outputPath);
    }

    //cout << "Ожидание заголовка файла..." << endl;

    // Прием заголовка: 4 байта сигнатуры + 8 байт размера

    char headerBuffer[12];
    ReceiveBytes(headerBuffer, 12);

    uint32_t signature = 0;
    memcpy(&signature, headerBuffer, sizeof(signature));

    if (signature != 0x12345678) 
    {
        throw runtime_error("Invalid header signature");
    }

    uint64_t fileSize = 0;
    memcpy(&fileSize, headerBuffer + 4, sizeof(fileSize));

    // Подтверждение приема заголовка
    sendAck('A');

    // Прием файла блоками

    uint64_t totalBytesReceived = 0;
    size_t chunksReceived = 0;

    vector<char> chunkBuffer(chunkSize);

    while (totalBytesReceived < fileSize)
    {
        uint64_t remaining = fileSize - totalBytesReceived;
        size_t currentChunkSize = static_cast<size_t>(
            min(remaining,static_cast<uint64_t>(chunkSize))
        );

        ReceiveBytes(chunkBuffer.data(), currentChunkSize);
        outputFile.write(chunkBuffer.data(),static_cast<streamsize>(currentChunkSize));

        if (!outputFile) 
        {
            throw runtime_error("Error writing to file");
        }

        totalBytesReceived += currentChunkSize;

        // Подтверждаем прием блока
        sendAck('A');

        // Отображаем прогресс

        //double progress = static_cast<double>(totalBytesReceived)
        //    / static_cast<double>(fileSize) * 100.0;

        //cout << "\rПрогресс: " << progress
        //    << "% (" << totalBytesReceived
        //    << "/" << fileSize << " байт)"
        //    << flush;
    }

    //cout << endl;

    // Принимаем завершающее сообщение END
    char endBuffer[3];
    ReceiveBytes(endBuffer, 3);
    if (endBuffer[0] != 'E' || endBuffer[1] != 'N' || endBuffer[2] != 'D') 
    {
        throw runtime_error("Invalid END message");
    }

    outputFile.close();
    if (!outputFile) 
    {
        throw runtime_error("Error closing file");
    }

    sendAck('F');
}
