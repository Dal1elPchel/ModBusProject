#include "Client.h"
#include <iostream>

int main()
{
    setlocale(LC_ALL, "Russian");

    try
    {
        Client master("COM7", 9600); //COM4

        master.SendFile("test.txt", 1024);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}