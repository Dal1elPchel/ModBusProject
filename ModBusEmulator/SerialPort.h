#pragma once
#define NOMINMAX

#include <windows.h>
#include <string>
#include <stdexcept>
#include <system_error>
#include <limits>
#include <vector>

class SerialPort
{
private:
    HANDLE hSerial = INVALID_HANDLE_VALUE;

    void open(const std::string& portName)
    {
        std::string deviceName = "\\\\.\\" + portName;

        hSerial = CreateFileA(
            deviceName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hSerial == INVALID_HANDLE_VALUE)
        {
            throw std::system_error(
                GetLastError(),
                std::system_category(),
                "Failed to open COM"
            );
        }
    }

    void configure(int baudRate)
    {
        DCB dcb = {};
        dcb.DCBlength = sizeof(dcb);

        if (!GetCommState(hSerial, &dcb))
        {
            throw std::system_error(
                GetLastError(),
                std::system_category(),
                "GetCommState failed"
            );
        }

        dcb.BaudRate = baudRate;
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcb))
        {
            throw std::system_error(
                GetLastError(),
                std::system_category(),
                "SetCommState failed"
            );
        }

        COMMTIMEOUTS timeout = {};

        timeout.ReadIntervalTimeout = 50;
        timeout.ReadTotalTimeoutConstant = 5000;
        timeout.ReadTotalTimeoutMultiplier = 0;

        timeout.WriteTotalTimeoutConstant = 500;
        timeout.WriteTotalTimeoutMultiplier = 10;

        if (!SetCommTimeouts(hSerial, &timeout))
        {
            throw std::system_error(
                GetLastError(),
                std::system_category(),
                "SetCommTimeouts failed"
            );
        }
    }

    void WriteBytes(const std::vector<uint8_t>& data)
    {
        if (data.size() > std::numeric_limits<DWORD>::max())
        {
            throw std::length_error("Message is too large");
        }

        DWORD total = static_cast<DWORD>(data.size());
        DWORD offset = 0;

        while (offset < total)
        {
            DWORD written = 0;

            if (!WriteFile(
                hSerial,
                data.data() + offset,
                total - offset,
                &written,
                nullptr
            ))
            {
                throw std::system_error(
                    GetLastError(),
                    std::system_category(),
                    "WriteFile failed"
                );
            }

            if (written == 0)
            {
                throw std::runtime_error("No bytes written");
            }

            offset += written;
        }
    }

protected:

    SerialPort(const std::string& portName, int baudRate)
    {
        open(portName);

        try
        {
            configure(baudRate);
        }
        catch (...)
        {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
            throw;
        }
    }

    void SendBytes(const std::vector<uint8_t>& data)
    {
        WriteBytes(data);
    }

    // ѕолучение заданного количества бинарных данных
    void ReceiveBytes(uint8_t* buffer, size_t size)
    {
        size_t totalReceived = 0;

        while (totalReceived < size)
        {
            DWORD bytesRead = 0;

            size_t remaining = size - totalReceived;

            DWORD bytesToRead = static_cast<DWORD>(
                remaining > std::numeric_limits<DWORD>::max()
                ? std::numeric_limits<DWORD>::max()
                : remaining
                );

            if (!ReadFile(
                hSerial,
                buffer + totalReceived,
                bytesToRead,
                &bytesRead,
                nullptr
            ))
            {
                throw std::system_error(
                    GetLastError(),
                    std::system_category(),
                    "ReadFile failed"
                );
            }

            if (bytesRead == 0)
            {
                throw std::runtime_error("Read timeout");
            }

            totalReceived += bytesRead;
        }
    }

public:

    virtual ~SerialPort()
    {
        if (hSerial != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hSerial);
        }
    }

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
};
