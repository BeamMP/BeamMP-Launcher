////
//// Created by Anonymous275 on 3/3/2020.
////

#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> Check();

void Exit(const std::string& Msg){
    std::cout << Msg << std::endl;
    std::cout << "Press Enter to continue . . .";
    std::cin.ignore();
    exit(-1);
}

void ProxyStart();
std::string HTTP_REQUEST();
std::vector<std::string> Discord_Main();
int main()
{
    //Security
    std::vector<std::string> Data = Check();
    std::cout << "You own BeamNG on this machine!" << std::endl;

    //std::cout << Data.at(2) << "\\BeamNG.drive.exe";

    std::cout << "Name : " << Discord_Main().at(0) << std::endl;
    std::cout << "Discriminator : " << Discord_Main().at(1) << std::endl;
    std::cout << "Unique ID : " << Discord_Main().at(2) << std::endl;


    //std::cout << "\nHTTP TEST :\n\n";
    //std::cout << HTTP_REQUEST();

    /// Update, Mods ect...

    //Start(); //Proxy main start

    Exit("");
    return 0;
}