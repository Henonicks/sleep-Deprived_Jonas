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

#include "jonas/logging.hpp"

#include "jonas/jonas.hpp"

#include <filesystem>

void logger::open() {
	if (!std::filesystem::is_directory("../logging")) {
		std::filesystem::create_directory("../logging");
	}
	if (!logfile.stream.is_open()) {
		logfile.stream.open(logfile.filename, std::ofstream::out | std::ofstream::trunc);
	}
	if (!dpp_logfile.stream.is_open()) {
		dpp_logfile.stream.open(dpp_logfile.filename, std::ofstream::out | std::ofstream::trunc);
	}
}

#ifndef CUSTOM_LOG_TRUNCATOR
void logger::truncate(logfile_t& file) {
	if (file.stream.tellp() >= LOGFILE_SIZE_THRESHOLD) {
		file.stream.flush();
		if (file.backup != nullptr) {
			file.backup->stream.close();
			file.backup->stream.clear();
			file.backup->stream.open(file.backup->filename, std::ofstream::out | std::ofstream::trunc);
			file.backup->stream << file.buffer.str() << std::flush;
		}
		file.stream.close();
		file.stream.clear();
		file.buffer.str("");
		file.stream.open(file.filename, std::ofstream::out | std::ofstream::trunc);
	}
}
#else
#include "jonas/user_defined/log_truncator.hpp"
#endif

void logger::log(std::string_view const msg) {
	truncate(logfile);
	std::stringstream log_stream;
	log_stream << '[' << dpp::utility::current_date_time() << "]: " << msg << '\n';
	logfile.stream << log_stream.str() << std::flush;
	logfile.buffer << log_stream.str();
}

void logger::dpp_log(dpp::log_t const& log) {
	truncate(dpp_logfile);
	std::stringstream log_stream;
	log_stream << '[' << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(log.severity) << ": " << log.message << '\n';
	dpp_logfile.stream << log_stream.str() << std::flush;
	dpp_logfile.buffer << log_stream.str();
}
