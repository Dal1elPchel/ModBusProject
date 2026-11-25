#include "Master.h"
#include <iostream>
#include <iomanip> 

int main()
{
    setlocale(LC_ALL, "Russian");

    try
    {
        Master master("COM7", 9600);

        std::cout << "Успешное подключение к COM7" << std::endl;

        std::cout << "Отправка запроса на чтение датчика температуры..." << std::endl;
        
        auto timeData = master.ReadHoldingRegisters(1, 2, 2);
        uint32_t rawTime = (static_cast<uint32_t>(timeData[1]) << 16) | timeData[0];
        std::cout << "\tСырое значение времени (Unix): " << rawTime << std::endl;

        auto tempData = master.ReadHoldingRegisters(1, 0, 2);
        uint32_t rawTemp = (static_cast<uint32_t>(tempData[1]) << 16) | tempData[0];
        int32_t temp = static_cast<int32_t>(rawTemp);
        std::cout << "\tПоказания температуры: " << temp << std::endl;

        std::cout << "Отправка запроса на чтение датчика масла..." << std::endl;
        auto oilData = master.ReadHoldingRegisters(2, 0, 2);
        uint32_t rawOil = (static_cast<uint32_t>(oilData[1]) << 16) | oilData[0];
        int32_t oil = static_cast<int32_t>(rawOil);
        std::cout << "\tПоказания датчика масла: " << oil << std::endl;

        std::cout << "Отправка запроса на чтение датчика мощности..." << std::endl;
        auto powerData = master.ReadHoldingRegisters(3, 0, 2);
        uint32_t rawPower = (static_cast<uint32_t>(powerData[1]) << 16) | powerData[0];
        int32_t power = static_cast<int32_t>(rawPower);
        std::cout << "\tПоказания датчика мощности: " << power << std::endl;

        std::cout << "Отправка запроса на чтение датчика мотора..." << std::endl;
        auto motorData = master.ReadHoldingRegisters(4, 0, 2);
        uint32_t rawMotor = (static_cast<uint32_t>(motorData[1]) << 16) | motorData[0];
        int32_t motor = static_cast<int32_t>(rawMotor);
        std::cout << "\tПоказания датчика: " << motor << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ОШИБКА: " << e.what() << std::endl;
    }

    return 0;
}