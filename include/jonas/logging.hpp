/**************************************************************************
 * Copyright 2026 Ramskyi Roman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
***************************************************************************/

#ifndef LOGGING
#define LOGGING

#include <fstream>

#include <dpp/dispatcher.h>

struct logfile_t {
	logfile_t* backup;
	std::ofstream stream{};
	std::string filename{};
	explicit logfile_t(std::string_view const filename, logfile_t* const backup = nullptr) : backup{backup}, filename{filename} {}

	std::stringstream buffer{};
};

class logger {
	static inline logfile_t logfile_backup{"../logging/logfile_backup.log"};
	static inline logfile_t logfile{"../logging/logfile.log", &logfile_backup};
	static inline logfile_t dpp_logfile_backup{"../logging/dpp_logfile_backup.log"};
	static inline logfile_t dpp_logfile{"../logging/dpp_logfile.log", &dpp_logfile_backup};

	static void truncate(logfile_t& file);
public:
	logger() = delete;
	static void open();

	static void log(std::string_view msg);
	static void dpp_log(dpp::log_t const& log);
};

#endif
