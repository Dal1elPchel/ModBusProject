#include "Server.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>

using namespace std;

Server::Server(const string& port, int rate) :
    comPortName(port), baudRate(rate), hComPort(INVALID_HANDLE_VALUE) {
}

Server::~Server()
{
    if (hComPort != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hComPort);
    }
}

bool Server::receiveData(vector<char>& buffer, size_t bufferSize)
{
    buffer.resize(bufferSize);

    size_t totalBytesRead = 0;

    while (totalBytesRead < bufferSize)
    {
        size_t remaining = bufferSize - totalBytesRead;

        DWORD bytesToRead = static_cast<DWORD>(remaining);

        DWORD bytesRead = 0;

        if (!ReadFile(
            hComPort,
            buffer.data() + totalBytesRead,
            bytesToRead,
            &bytesRead,
            NULL
        ))
        {
            cerr << "Ошибка чтения данных: " << GetLastError() << endl;
            return false;
        }

        if (bytesRead == 0)
        {
            cerr << "Истекло время ожидания данных" << endl;
            return false;
        }

        totalBytesRead += bytesRead;
    }

    return true;
}

bool Server::sendAck(char ackType)
{
    DWORD bytesWritten = 0;

    if (!WriteFile(
        hComPort,
        &ackType,
        1,
        &bytesWritten,
        NULL
    ))
    {
        cerr << "Ошибка отправки подтверждения: "
            << GetLastError() << endl;

        return false;
    }

    return bytesWritten == 1;
}

bool Server::openPort()
{
    hComPort = CreateFileA(
        comPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hComPort == INVALID_HANDLE_VALUE)
    {
        cerr << "Ошибка открытия порта: " << GetLastError() << endl;
        return false;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hComPort, &dcbSerialParams))
    {
        cerr << "Ошибка получения параметров порта: " << GetLastError() << endl;
        CloseHandle(hComPort);
        hComPort = INVALID_HANDLE_VALUE;
        return false;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hComPort, &dcbSerialParams))
    {
        cerr << "Ошибка установки параметров порта: " << GetLastError() << endl;
        CloseHandle(hComPort);
        hComPort = INVALID_HANDLE_VALUE;
        return false;
    }

    // Установка таймаутов
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 5000;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hComPort, &timeouts))
    {
        cerr << "Ошибка установки таймаутов: " << GetLastError() << std::endl;
        CloseHandle(hComPort);
        hComPort = INVALID_HANDLE_VALUE;
        return false;
    }
    return true;
}

bool Server::receiveFile(const string& outputPath, size_t chunkSize)
{
    if (chunkSize == 0)
    {
        cerr << "Размер блока не может быть равен нулю" << endl;
        return false;
    }

    ofstream outputFile(outputPath, ios::binary);

    if (!outputFile.is_open())
    {
        cerr << "Ошибка открытия файла для записи: "
            << outputPath << endl;

        return false;
    }

    cout << "Ожидание заголовка файла..." << endl;

    // 1. Прием заголовка: 4 байта сигнатуры + 8 байт размера

    vector<char> headerBuffer;

    if (!receiveData(headerBuffer, 12))
    {
        cerr << "Ошибка приема заголовка" << endl;
        return false;
    }

    // 2. Извлечение сигнатуры

    uint32_t signature = 0;

    memcpy(
        &signature,
        headerBuffer.data(),
        sizeof(signature)
    );

    if (signature != 0x12345678)
    {
        cerr << "Неверная сигнатура заголовка: 0x"
            << hex << signature << dec << endl;

        return false;
    }

    // 3. Извлечение размера файла

    uint64_t fileSize = 0;

    memcpy(
        &fileSize,
        headerBuffer.data() + 4,
        sizeof(fileSize)
    );

    cout << "Получен размер файла: "
        << fileSize << " байт" << endl;

    // 4. Подтверждение приема заголовка

    if (!sendAck('A'))
    {
        cerr << "Ошибка отправки подтверждения заголовка" << endl;
        return false;
    }

    cout << "Начинаем прием файла..." << endl;

    // 5. Прием файла блоками

    uint64_t totalBytesReceived = 0;
    size_t chunksReceived = 0;

    vector<char> chunkBuffer;

    while (totalBytesReceived < fileSize)
    {
        // Сколько байт осталось получить

        uint64_t remaining = fileSize - totalBytesReceived;

        // Не запрашиваем больше, чем осталось в файле

        size_t currentChunkSize = static_cast<size_t>(
            min(
                remaining,
                static_cast<uint64_t>(chunkSize)
            )
            );

        // Получаем ровно currentChunkSize байт

        if (!receiveData(chunkBuffer, currentChunkSize))
        {
            cerr << "\nОшибка приема блока данных" << endl;
            return false;
        }

        // Записываем блок в файл

        outputFile.write(
            chunkBuffer.data(),
            static_cast<streamsize>(currentChunkSize)
        );

        if (!outputFile)
        {
            cerr << "Ошибка записи данных в файл" << endl;
            return false;
        }

        // Обновляем счетчики

        totalBytesReceived += currentChunkSize;
        chunksReceived++;

        // Подтверждаем прием блока

        if (!sendAck('A'))
        {
            cerr << "Ошибка отправки подтверждения блока "
                << chunksReceived << endl;

            return false;
        }

        // Отображаем прогресс

        double progress = static_cast<double>(totalBytesReceived)
            / static_cast<double>(fileSize) * 100.0;

        cout << "\rПрогресс: " << progress
            << "% (" << totalBytesReceived
            << "/" << fileSize << " байт)"
            << flush;
    }

    cout << endl;

    // 6. Принимаем завершающее сообщение END

    vector<char> endBuffer;

    if (!receiveData(endBuffer, 3))
    {
        cerr << "Ошибка приема завершающего сообщения" << endl;
        return false;
    }

    if (endBuffer[0] != 'E' ||
        endBuffer[1] != 'N' ||
        endBuffer[2] != 'D')
    {
        cerr << "Неверное завершающее сообщение" << endl;
        return false;
    }

    cout << "Получено завершающее сообщение END" << endl;

    // 7. Проверяем и закрываем файл

    outputFile.close();

    if (!outputFile)
    {
        cerr << "Ошибка закрытия файла" << endl;
        return false;
    }

    // 8. Финальное подтверждение

    if (!sendAck('F'))
    {
        cerr << "Ошибка отправки финального подтверждения" << endl;
        return false;
    }

    cout << "Файл успешно сохранен: "
        << outputPath << endl;

    return true;
}
