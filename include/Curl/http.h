///
/// Created by Anonymous275 on 7/18/2020
///
#pragma once
#include <string>
int Download(const std::string& URL,const std::string& Path,bool close);
std::string PostHTTP(const std::string& IP, const std::string& Fields);
std::string HTTP_REQUEST(const std::string& IP,int port);