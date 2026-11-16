#include "Server.h"
#include <iostream>

int main()
{
    setlocale(LC_ALL, "Russian");

    try
    {
        Server server("COM8", 9600); // COM5

        std::cout << "Slave waiting..." << std::endl;

        server.receiveFile("received.txt", 1024);
        std::cout << "File received!" << std::endl;
        
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}