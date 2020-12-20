// Copyright (c) 2020 Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///
#pragma once
#include <string>
int Download(const std::string& URL,const std::string& Path,bool close);
std::string PostHTTP(const std::string& IP, const std::string& Fields);
std::string HTTP_REQUEST(const std::string& IP,int port);