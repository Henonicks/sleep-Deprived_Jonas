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

/* Ripped straight from https://github.com/Henonicks/bump-watcher, thanks J!
 * Will only return a meaningful value on Linux.
 */
int64_t proc_self_value(const std::string_view find_token) {
	int64_t ret{0};
	std::ifstream self_status{"/proc/self/status"};
	while (self_status) {
		std::string token;
		self_status >> token;
		if (token == find_token) {
			self_status >> ret;
			break;
		}
	}
	self_status.close();
	return ret;
}

int64_t get_ram_usage_bytes() {
	return proc_self_value("VmRSS:") * 1024;
}

dpp::embed make_status_embed() {
	std::string const uptime = std::to_string(bot->uptime().days)  + 'd' +
	                     ' ' + std::to_string(bot->uptime().hours) + 'h' +
	                     ' ' + std::to_string(bot->uptime().mins)  + 'm' +
	                     ' ' + std::to_string(bot->uptime().secs)  + 's';
	int64_t const ram_usage = get_ram_usage_bytes() / 1024 / 1024;
	dpp::embed res = dpp::embed()
		.set_colour(STATUS_EMBED_COLOUR)
		.set_thumbnail(bot->me.get_avatar_url())
		.set_title("Status")
		.add_field("Uptime", uptime);
	if (ram_usage > 0) {
		res.add_field("Memory Usage", std::to_string(ram_usage) + "MiB");
	}
	res.add_field("Currently playing", curr_file_path.empty() ? "None" : curr_file_path);
	return res;
}

void run() {
	if (!TEST_MODE) {
		bot->on_ready([](dpp::ready_t const& ready) {
			if (dpp::run_once <struct init_audio_player>()) {
				if (CREATE_STATUS_SLASHCOMMAND) {
					STATUS_SLASHCOMMAND.application_id = bot->me.id;
					bot->global_bulk_command_create({STATUS_SLASHCOMMAND}, [](dpp::confirmation_callback_t const& callback) {
						if (callback.is_error()) {
							logger::log("Failed to create the slashcommands: " + callback.get_error().message);
						}
					});
				}
				bot->channel_get(CHANNEL_ID, [ready](dpp::confirmation_callback_t const& callback) {
					if (callback.is_error()) {
						std::cerr << "Couldn't get the channel to join! " + callback.get_error().human_readable << '\n';
						std::terminate();
					}
					GUILD_ID = callback.get <dpp::channel>().guild_id;
					prev_shard = ready.from();
					prev_shard->connect_voice(GUILD_ID, CHANNEL_ID, false, true, true);
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

		bot->on_slashcommand([](dpp::slashcommand_t const& event) {
			if (event.command.get_command_name() == STATUS_SLASHCOMMAND.name) {
				dpp::message response = dpp::message().add_embed(make_status_embed());
				if (STATUS_RESPONSE_EPHEMERAL) {
					response.set_flags(dpp::m_ephemeral);
				}
				event.reply(response);
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
	dpp::discord_client* const curr_shard = bot->get_shard(0);
	if (curr_shard != prev_shard) {
		prev_shard = curr_shard;
		if (REJOIN_ON_DISCONNECT) {
			curr_shard->disconnect_voice(GUILD_ID);
			// This will get picked up by the voice state update handler and the bot will automatically reconnect
		}
	}
	return bot->get_shard(0);
}

dpp::discord_voice_client* get_voice_client() {
	dpp::voiceconn* const voice_connection = shard()->get_voice(GUILD_ID);
	if (voice_connection == nullptr) {
		return nullptr;
	}
	return voice_connection->voiceclient.get();
}
