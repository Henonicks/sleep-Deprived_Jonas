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

#include "jonas/jonas.hpp"

void run() {
	if (!TEST_MODE) {
		bot->on_ready([](dpp::ready_t const& ready) {
			if (dpp::run_once <struct init_audio_player>()) {
				bot->channel_get(CHANNEL_ID, [ready](dpp::confirmation_callback_t const& callback) {
					if (callback.is_error()) {
						std::cerr << "Couldn't get the channel to join to! " + callback.get_error().human_readable << '\n';
						std::terminate();
					}
					GUILD_ID = callback.get <dpp::channel>().guild_id;
					ready.from()->connect_voice(GUILD_ID, CHANNEL_ID, false, true, true);
				});
			}
		});

		bot->on_voice_ready([](dpp::voice_ready_t const& event) {
			if (dpp::run_once <struct establish_connection>()) {
				voice_client = event.from()->get_voice(GUILD_ID)->voiceclient.get();
				init_player();
			}
		});

		bot->on_voice_state_update([](dpp::voice_state_update_t const& event) {
			if (event.state.channel_id.empty()) {
				if (event.state.user_id == bot->me.id) {
					bot->shutdown();
				}
			}
			else if (PAUSE_WHEN_ALONE) {
				someone_joined = true;
				join_cv.notify_one();
			}
		});

		bot->start(dpp::st_wait);
	}
	else {
		init_player();
		std::cout << "Passed: " << passed_files << "\nFailed: " << failed_files << '\n';
		logger::log("Passed: " + std::to_string(passed_files) + "\nFailed: " + std::to_string(failed_files));
	}
}
