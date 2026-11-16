#include "Server.h"
#include <iostream>

int main()
{
    setlocale(LC_ALL, "Russian");
    Server server("COM5", 9600);

    if (!server.openPort())
        return 1;

    std::cout << "Slave waiting..." << std::endl;

    if (server.receiveFile("received.txt", 1024))
        std::cout << "File received!" << std::endl;

    return 0;
}