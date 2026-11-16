#pragma once

#include "SerialPort.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

class Client : public SerialPort
{
private:

    // Ожидание подтверждения от Slave
    void WaitForAck(char expected)
    {
        char response = 0;

        ReceiveBytes(&response, 1);

        if (response != expected)
        {
            throw std::runtime_error(
                "Invalid response from Slave"
            );
        }
    }

    // Отправка заголовка файла
    void SendHeader(uint64_t fileSize)
    {
        uint32_t signature = 0x12345678;

        std::string header(12, '\0');

        std::memcpy(
            &header[0],
            &signature,
            sizeof(signature)
        );

        std::memcpy(
            &header[4],
            &fileSize,
            sizeof(fileSize)
        );

        SendBytes(header);

        // Ждем подтверждения заголовка
        WaitForAck('A');
    }

    // Отправка одного блока файла
    void SendChunk(const char* data, size_t size)
    {
        std::string chunk(data, size);

        SendBytes(chunk);

        // Ждем подтверждения блока
        WaitForAck('A');
    }

public:

    Client(const std::string& portName)
        : SerialPort(portName)
    {
    }

    void Send(const std::string& message)
    {
        SendLine(message);
    }

    void SendFile(
        const std::string& filePath,
        size_t chunkSize = 1024
    )
    {
        if (chunkSize == 0)
        {
            throw std::invalid_argument(
                "Chunk size cannot be zero"
            );
        }

        if (chunkSize > std::numeric_limits<DWORD>::max())
        {
            throw std::invalid_argument(
                "Chunk size is too large"
            );
        }

        std::ifstream inputFile(
            filePath,
            std::ios::binary | std::ios::ate
        );

        if (!inputFile.is_open())
        {
            throw std::runtime_error(
                "Cannot open file: " + filePath
            );
        }

        // Получаем размер файла
        uint64_t fileSize = static_cast<uint64_t>(
            inputFile.tellg()
            );

        inputFile.seekg(0, std::ios::beg);

        std::cout << "File size: "
            << fileSize << " bytes" << std::endl;

        // 1. Отправляем заголовок
        SendHeader(fileSize);

        std::cout << "Header accepted by Slave" << std::endl;

        // 2. Подготавливаем буфер
        std::vector<char> buffer(chunkSize);

        uint64_t totalBytesSent = 0;

        // 3. Отправляем файл блоками
        while (totalBytesSent < fileSize)
        {
            uint64_t remaining = fileSize - totalBytesSent;

            size_t currentChunkSize = static_cast<size_t>(
                std::min(
                    remaining,
                    static_cast<uint64_t>(chunkSize)
                )
                );

            // Читаем очередной блок из файла
            inputFile.read(
                buffer.data(),
                static_cast<std::streamsize>(currentChunkSize)
            );

            if (inputFile.gcount() !=
                static_cast<std::streamsize>(currentChunkSize))
            {
                throw std::runtime_error(
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

        std::cout << std::endl;

        // 4. Сообщаем Slave об окончании передачи
        SendBytes("END");

        // 5. Ждем финальное подтверждение
        WaitForAck('F');

        std::cout << "File successfully sent!" << std::endl;
    }
};
