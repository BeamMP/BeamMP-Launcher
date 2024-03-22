// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <Logger.h>
#include <fstream>
#include "Http.h"
#include <mutex>
#include <cmath>
#include <filesystem>
#include <httplib.h>
#include <nlohmann/json.hpp>

void WriteHttpDebug(const httplib::Client& client, const std::string& method, const std::string& target, const httplib::Result& result) try {
	const std::filesystem::path folder = ".https_debug";
	std::filesystem::create_directories(folder);
	if (!std::filesystem::exists(folder / "WHAT IS THIS FOLDER.txt")) {
		std::ofstream ignore { folder / "WHAT IS THIS FOLDER.txt" };
		ignore << "This folder exists to help debug current issues with the backend. Do not share this folder with anyone but BeamMP staff. It contains detailed logs of any failed http requests." << std::endl;
	}
	const auto file = folder / (method + ".json");
	// 1 MB limit
	if (std::filesystem::exists(file) && std::filesystem::file_size(file) > 1'000'000) {
		std::filesystem::rename(file, file.generic_string() + ".bak");
	}

	std::ofstream of { file, std::ios::app };
	nlohmann::json js {
		{ "utc", std::chrono::system_clock::now().time_since_epoch().count() },
		{ "target", target },
		{ "client_info", {
				{ "openssl_verify_result", client.get_openssl_verify_result() },
				{ "host", client.host()},
				{ "port", client.port()},
				{ "socket_open", client.is_socket_open()},
				{"valid", client.is_valid()},
			}
		},
	};
	if (result) {
		auto value = result.value();
		js["result"] = {};
		js["result"]["body"] = value.body;
		js["result"]["status"] = value.status;
		js["result"]["headers"] = value.headers;
		js["result"]["version"] = value.version;
		js["result"]["location"] = value.location;
		js["result"]["reason"] = value.reason;
	}
	of << js.dump();
}
catch (const std::exception& e) {
	error(e.what());
}

bool HTTP::isDownload = false;
std::string HTTP::Get(const std::string& IP) {
	static std::mutex Lock;
	std::scoped_lock Guard(Lock);

	auto pos = IP.find('/', 10);

	httplib::Client cli(IP.substr(0, pos).c_str());
	cli.set_connection_timeout(std::chrono::seconds(10));
	cli.set_follow_location(true);
	auto res = cli.Get(IP.substr(pos).c_str(), ProgressBar);
	std::string Ret;

	if (res) {
		if (res->status == 200) {
			Ret = res->body;
		}
		else {
			WriteHttpDebug(cli, "GET", IP, res);
			error("Failed to GET '" + IP + "': " + res->reason + ", ssl verify = " + std::to_string(cli.get_openssl_verify_result()));
			
		}
	}
	else {
		if (isDownload) {
			std::cout << "\n";
		}
		WriteHttpDebug(cli, "GET", IP, res);
		error("HTTP Get failed on " + to_string(res.error()) + ", ssl verify = " + std::to_string(cli.get_openssl_verify_result()));
		
	}

	return Ret;
}

std::string HTTP::Post(const std::string& IP, const std::string& Fields) {
	static std::mutex Lock;
	std::scoped_lock Guard(Lock);

	auto pos = IP.find('/', 10);

	httplib::Client cli(IP.substr(0, pos).c_str());
	cli.set_connection_timeout(std::chrono::seconds(10));
	std::string Ret;

	if (!Fields.empty()) {
		httplib::Result res = cli.Post(IP.substr(pos).c_str(), Fields, "application/json");

		if (res) {
			if (res->status != 200) {
				error(res->reason);
			}
			Ret = res->body;
		}
		else {
			WriteHttpDebug(cli, "POST", IP, res);
			error("HTTP Post failed on " + to_string(res.error()) + ", ssl verify = " + std::to_string(cli.get_openssl_verify_result()));
			
		}
	}
	else {
		httplib::Result res = cli.Post(IP.substr(pos).c_str());
		if (res) {
			if (res->status != 200) {
				error(res->reason);
			}
			Ret = res->body;
		}
		else {
			WriteHttpDebug(cli, "POST", IP, res);
			error("HTTP Post failed on " + to_string(res.error()) + ", ssl verify = " + std::to_string(cli.get_openssl_verify_result()));
			
		}
	}

	if (Ret.empty())return "-1";
	else return Ret;
}

bool HTTP::ProgressBar(size_t c, size_t t) {
	if (isDownload) {
		static double last_progress, progress_bar_adv;
		progress_bar_adv = round(c / double(t) * 25);
		std::cout << "\r";
		std::cout << "Progress : [ ";
		std::cout << round(c / double(t) * 100);
		std::cout << "% ] [";
		int i;
		for (i = 0; i <= progress_bar_adv; i++)std::cout << "#";
		for (i = 0; i < 25 - progress_bar_adv; i++)std::cout << ".";
		std::cout << "]";
		last_progress = round(c / double(t) * 100);
	}
	return true;
}

bool HTTP::Download(const std::string& IP, const std::string& Path) {
	static std::mutex Lock;
	std::scoped_lock Guard(Lock);

	isDownload = true;
	std::string Ret = Get(IP);
	isDownload = false;

	if (Ret.empty())return false;

	std::ofstream File(Path, std::ios::binary);
	if (File.is_open()) {
		File << Ret;
		File.close();
		std::cout << "\n";
		info("Download Complete!");
	}
	else {
		error("Failed to open file directory: " + Path);
		return false;
	}

	return true;
}
