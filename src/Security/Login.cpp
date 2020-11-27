///
/// Created by Anonymous275 on 11/26/2020
///

#include "Curl/http.h"
#include <iostream>
#include <thread>

//check file if not present flag for login to the core network
//to then get user and pass
//if present use to contact the backend to refresh and get a public key for servers
//public keys are one time use for a random server

/// "username":"password"
/// "Guest":"Name"
/// "pk":"private_key"


///TODO: test with no internet connection
void CheckLocalKey(){
    for(int C = 1; C <= 10; C++) {
        std::cout << PostHTTP("https://auth.beammp.com/userlogin", R"({"username":"Anonymous275", "password":"SimonAS1482001"})") << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    system("pause");
}
