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

#include <filesystem>

void logger::open() {
	if (!std::filesystem::is_directory("../logging")) {
		std::filesystem::create_directory("../logging");
	}
	logfile.open("../logging/logfile.log");
	dpp_logfile.open("../logging/dpp_logfile.log");
}

void logger::log(std::string_view const msg) {
	logfile << '[' << dpp::utility::current_date_time() << "]: " << msg << std::endl;
}

void logger::dpp_log(dpp::log_t const& log) {
	dpp_logfile << '[' << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(log.severity) << ": " << log.message << std::endl;
}
