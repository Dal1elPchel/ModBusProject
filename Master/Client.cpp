#include "Client.h"
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace std;

Client::Client(const string& portName, int baudRate)
    : SerialPort(portName, baudRate) {}

void Client::WaitForAck(char expected)
{
    char response = 0;
    ReceiveBytes(&response, 1);
    if (response != expected)
    {
        throw runtime_error(
            "Invalid response from Slave"
        );
    }
}

void Client::SendHeader(uint64_t fileSize)
{
    uint32_t signature = 0x12345678;
    string header(12, '\0');
    memcpy(
        &header[0],
        &signature,
        sizeof(signature)
    );

    memcpy(
        &header[4],
        &fileSize,
        sizeof(fileSize)
    );

    SendBytes(header);

    // Ждем подтверждения заголовка
    WaitForAck('A');
}

void Client::SendChunk(const char* data, size_t size)
{
    string chunk(data, size);
    SendBytes(chunk);

    // Ждем подтверждения блока
    WaitForAck('A');
}

void Client::Send(const string& message)
{
    SendLine(message);
}

void Client::SendFile(const string& filePath, size_t chunkSize)
{
    if (chunkSize == 0)
    {
        throw invalid_argument(
            "Chunk size cannot be zero"
        );
    }

    if (chunkSize > numeric_limits<DWORD>::max())
    {
        throw invalid_argument(
            "Chunk size is too large"
        );
    }

    ifstream inputFile(
        filePath,
        ios::binary | ios::ate
    );

    if (!inputFile.is_open())
    {
        throw runtime_error(
            "Cannot open file: " + filePath
        );
    }

    // Получаем размер файла
    uint64_t fileSize = static_cast<uint64_t>(
        inputFile.tellg()
        );

    inputFile.seekg(0, ios::beg);
    /*cout << "File size: "
        << fileSize << " bytes" << endl;*/

    // 1. Отправляем заголовок
    SendHeader(fileSize);
    //cout << "Header accepted by Slave" << endl;

    // 2. Подготавливаем буфер
    vector<char> buffer(chunkSize);
    uint64_t totalBytesSent = 0;

    // 3. Отправляем файл блоками
    while (totalBytesSent < fileSize)
    {
        uint64_t remaining = fileSize - totalBytesSent;
        size_t currentChunkSize = static_cast<size_t>(
            min(remaining, static_cast<uint64_t>(chunkSize))
        );

        // Читаем очередной блок из файла
        inputFile.read(
            buffer.data(),
            static_cast<streamsize>(currentChunkSize)
        );

        if (inputFile.gcount() !=
            static_cast<streamsize>(currentChunkSize))
        {
            throw runtime_error(
                "Error reading file"
            );
        }

        // Отправляем блок и ждем ACK
        SendChunk(
            buffer.data(),
            currentChunkSize
        );

        totalBytesSent += currentChunkSize;
    }
    //cout << endl;

    // 4. Сообщаем Slave об окончании передачи
    SendBytes("END");

    // 5. Ждем финальное подтверждение
    WaitForAck('F');

    //cout << "File successfully sent!" << endl;
}