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
						std::cerr << "Couldn't get the channel to join! " + callback.get_error().human_readable << '\n';
						std::terminate();
					}
					GUILD_ID = callback.get <dpp::channel>().guild_id;
					shard()->connect_voice(GUILD_ID, CHANNEL_ID, false, true, true);
				});
			}
		});

		bot->on_voice_ready([](dpp::voice_ready_t const&) {
			if (dpp::run_once <struct establish_connection>()) {
				init_player();
			}
			std::shared_lock L(shard()->voice_mutex);
			dpp::discord_voice_client* const voice_client = get_voice_client();
			if (voice_client != nullptr) {
				voice_client->stop_audio();
			}
		});

		bot->on_voice_state_update([](dpp::voice_state_update_t const& event) {
			if (event.state.channel_id.empty()) {
				if (event.state.user_id == bot->me.id) {
					shard()->connect_voice(GUILD_ID, CHANNEL_ID, false, true, true);
				}
			}
			else if (event.state.user_id == bot->me.id) {
				if (event.state.channel_id != CHANNEL_ID) {
					if (!SNAP_TO_CHANNEL) {
						CHANNEL_ID = event.state.channel_id;
					}
					std::shared_lock L(shard()->voice_mutex);
					dpp::discord_voice_client* const voice_client = get_voice_client();
					if (voice_client == nullptr) {
						L.unlock();
						shard()->disconnect_voice(GUILD_ID);
						// We need to do this because otherwise the bot won't connect to the channels it joins
					}
				}
			}
			else if (PAUSE_WHEN_ALONE) {
				if (event.state.channel_id == CHANNEL_ID) {
					someone_joined = true;
					join_cv.notify_one();
				}
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

dpp::discord_client* shard() {
	return bot->get_shard(0);
}

dpp::discord_voice_client* get_voice_client() {
	dpp::voiceconn* const voice_connection = shard()->get_voice(GUILD_ID);
	if (voice_connection == nullptr) {
		return nullptr;
	}
	return voice_connection->voiceclient.get();
}
