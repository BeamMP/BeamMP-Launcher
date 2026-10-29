/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once

// Background jobs for the client mod ('J' packets on the core socket).
//
// The game sends   J{"id":"<any string>","type":"<job type>", ...job fields}
// and gets back    J{"id":"...","status":"progress","stage":"...","done":n,"total":m}   (optional, any number)
//                  J{"id":"...","status":"done","result":{...}}
//                  J{"id":"...","status":"failed","reason":"..."}
// Jobs run on a small pool of low priority worker threads, never on the network threads.
// "ping" is answered straight away with the protocol version and the job types this launcher knows,
// so the mod can detect support (an older launcher simply never answers a 'J' packet).

#include <nlohmann/json.hpp>
#include <string>

namespace Jobs {

constexpr int ProtocolVersion = 1;

// a game connected to the core socket: drops queued work of the previous connection
void OnGameConnected();
// payload is the packet without the leading 'J'
void Handle(const std::string& payload);

// runs one job synchronously on the calling thread (debug CLI: --run-job <file.json>), returns the reply object
nlohmann::json RunNow(const nlohmann::json& request);
int RunCli(const char* jsonFile);

}
