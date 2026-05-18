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

#include "jonas/cfg.hpp"

#include "jonas/jonas.hpp"

void configure() {
	logger::open();
	std::string critical_whats;
	try {
		henifig::config_t const general_config("../config/behaviour.hfg");
		try {
			TEST_MODE = general_config["TEST_MODE"];
		}
		catch (henifig::retrieval_exception const&) {}
		if (TEST_MODE) {
			std::cout << "Test mode enabled. The files won't be played and the bot won't start.\n";
			return;
		}
		try {
			TRANSITION_DELAY_SECONDS = general_config["TRANSITION_DELAY_SECONDS"];
		}
		catch (henifig::retrieval_exception const& e) {
			std::cerr << "Falling back to the default transition delay, " << TRANSITION_DELAY_SECONDS << ", due to: " << e.what() << '\n';
		}
		try {
			PAUSE_WHEN_ALONE = general_config["PAUSE_WHEN_ALONE"];
		}
		catch (henifig::retrieval_exception const& e) {
			std::cerr << "Falling back to the default behaviour in regard to pausing when alone, " << std::boolalpha << PAUSE_WHEN_ALONE << std::noboolalpha << ", due to: " << e.what() << '\n';
		}
		try {
			DISPLAY_PLAYLIST = general_config["DISPLAY_PLAYLIST"];
		}
		catch (henifig::retrieval_exception const& e) {
			std::cerr << "Falling back to the default behaviour in regard to displaying the playlist, " << std::boolalpha << DISPLAY_PLAYLIST << std::noboolalpha << ", due to: " << e.what() << '\n';
		}
	}
	catch (henifig::parse_exception const& e) {
		critical_whats += std::string("behaviour.hfg: ") + e.what() + '\n';
		if (e.report.get_error_code() == henifig::error_codes::FILE_OPEN_FAILED) {
			std::cerr << "Couldn't open behaviour.hfg! Creating a default config for you to edit...\n";
			if (stdfs::exists("../config")) {
				if (!stdfs::is_directory("../config")) {
					std::cerr << "behaviour.hfg needs to be in a directory \"config\" but it's a file! Remove it!\n";
				}
			}
			else {
				stdfs::create_directory("../config");
			}
			try {
				stdfs::copy_file("../default_configs/behaviour.hfg", "../config/behaviour.hfg");
				std::cerr << "Couldn't copy behaviour.hfg file!\n";
			}
			catch (std::exception const& copy_e) {
				critical_whats += std::string("behaviour.hfg: Couldn't copy the default jonas.hfg: ") + copy_e.what() + '\n';
			}
		}
	}
	if (!TEST_MODE) {
		try {
			henifig::config_t const config("../config/jonas.hfg");
			std::string const& BOT_TOKEN = config["BOT_TOKEN"];
			if (BOT_TOKEN.empty()) {
				critical_whats += "jonas.hfg: No bot token provided! Edit config/jonas.hfg to fix this!\n";
			}
			CHANNEL_ID = config["CHANNEL_ID"];
			if (CHANNEL_ID.empty()) {
				critical_whats += "jonas.hfg: No channel ID provided! Edit config/jonas.hfg to fix this!\n";
			}

			if (critical_whats.empty()) {
				bot = new dpp::cluster(BOT_TOKEN, dpp::i_default_intents | dpp::i_guilds);
				bot->on_log(logger::dpp_log);
			}
		}
		catch (henifig::parse_exception const& e) {
			critical_whats += std::string("jonas.hfg: ") + e.what() + '\n';
			if (e.report.get_error_code() == henifig::error_codes::FILE_OPEN_FAILED) {
				std::cerr << "Couldn't open jonas.hfg! Creating a default config for you to edit...\n";
				if (stdfs::exists("../config")) {
					if (!stdfs::is_directory("../config")) {
						std::cerr << "jonas.hfg needs to be in a directory \"config\" but it's a file! Remove it!\n";
					}
				}
				else {
					stdfs::create_directory("../config");
				}
				try {
					stdfs::copy_file("../default_configs/jonas.hfg", "../config/jonas.hfg");
				}
				catch (std::exception const& copy_e) {
					critical_whats += std::string("jonas.hfg: Couldn't copy the default jonas.hfg! ") + copy_e.what() + '\n';
				}
			}
		}
	}
	if (!critical_whats.empty()) {
		critical_whats.pop_back();
		throw std::logic_error('\n' + critical_whats);
	}
}

void start_player() {
	while (true) {
		file_entries = make_file_list("../resources");
		for (auto it = file_entries.begin(); it != file_entries.end(); ++it) {
			std::string log = "Found " + it->path().filename().string();
			if (it->is_directory()) {
				log += '/';
			}
			logger::log(log);
			if (it->is_regular_file()) {
				std::string_view const extension = it->path().extension().c_str();
				if (false
				|| extension == ".wav"
				|| extension == ".mp3"
				|| extension == ".ogg"
				|| extension == ".opus"
				|| extension == ".flac"
				|| extension == ".pcm" || extension == ".raw"
				) {
					play_file(it - file_entries.begin(), !played_once);
				}
			}
		}
		if (TEST_MODE) {
			break;
		}
	}
}

void init_player() {
	if (!TEST_MODE) {
		auto const detach_audio_player = [] {
			std::thread audio_player_thread(start_player);
			audio_player_thread.detach();
		};
		if (DISPLAY_PLAYLIST) {
			bot->message_create(dpp::message(CHANNEL_ID, "Initialising..."), [detach_audio_player](dpp::confirmation_callback_t const& callback) {
				MESSAGE_ID = callback.get <dpp::message>().id;
				detach_audio_player();
			});
		}
		else {
			detach_audio_player();
		}
	}
	else {
		start_player();
	}
}
