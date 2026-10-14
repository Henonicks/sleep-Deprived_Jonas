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

#include "jonas/audio.hpp"

#include "jonas/jonas.hpp"

#include <dpp/unicode_emoji.h>

[[nodiscard]] float* normalise(float const input[], sf_count_t input_size) {
	if (input_size == 0) {
		logger::log("The data is reportedly empty! There is nothing to convert!");
		return nullptr;
	}
	auto [min_ptr, max_ptr] = std::minmax_element(input, input + input_size);
	float const abs_max = std::max(std::abs(*min_ptr), std::abs(*max_ptr));
	logger::log(std::format("Upon normalising, the max absolute value is {}.", abs_max));
	float const markiplier = NORMALISE_TO / abs_max;
	auto* const output = new float[input_size];
	float max{};
	for (int i = 0; i < input_size; i++) {
		max = std::max(max, std::abs(output[i] = input[i] * markiplier));
	}
	return output;
}

[[nodiscard]] int16_t* mono_to_stereo(int16_t const input[], sf_count_t const input_size) {
	if (input_size == 0) {
		logger::log("The data is reportedly empty! There is nothing to convert!");
		return nullptr;
	}
	auto* const output = new int16_t[input_size * 2];
	for (sf_count_t i = 0; i < input_size; i++) {
		for (int j = 0; j < 2; j++) {
			output[i * 2 + j] = input[i];
		}
	}
	return output;
}

[[nodiscard]] int16_t* to_stereo(int16_t const input[], sf_count_t const input_size, int const channels) {
	if (input_size == 0) {
		logger::log("The data is reportedly empty! There is nothing to convert!");
		return nullptr;
	}
	if (channels <= 0) {
		logger::log("There are reportedly no channels! There is nothing to convert!");
		return nullptr;
	}
	if (channels == 2) {
		logger::log("The audio is reportedly already stereo! There is no point in conversion!");
		return nullptr;
	}
	if (input_size % channels != 0) {
		logger::log("The size is indivisible by the amount of channels! Consider this a conversion failure.");
		return nullptr;
	}
	sf_count_t const mono_size = input_size / channels;
	if (channels == 1) {
		return mono_to_stereo(input, mono_size);
	}
	auto* const stereo = new int16_t[mono_size * 2];
	for (sf_count_t i = 0; i < mono_size; i++) {
		int64_t additional_sum{};
		for (int j = 2; j < channels; j++) {
			additional_sum += input[i * channels + j];
		}
		for (int j = 0; j < 2; j++) {
			stereo[i * 2 + j] = input[i * channels + j] + additional_sum / channels;
		}
	}
	return stereo;
}

[[nodiscard]] int16_t* trim_off_silence(int16_t const input[], sf_count_t const input_size, sf_count_t* const output_size) {
	logger::log("Trimming silence off of the audio.");
	if (input_size == 0) {
		logger::log("The audio is reportedly empty! There's nothing to trim off of!");
		*output_size = 0;
		return nullptr;
	}
	if (input_size % TARGET_CHANNELS != 0) {
		logger::log(std::format("The audio size is indivisible by two, impossible for audio with {} channels!", TARGET_CHANNELS));
		*output_size = 0;
		return nullptr;
	}
	sf_count_t l, r;
	for (l = 0; l < input_size; l += TARGET_CHANNELS) {
		bool is_data{};
		for (int j = 0; j < TARGET_CHANNELS; j++) {
			if (input[l + j] != 0) {
				is_data = true;
				break;
			}
		}
		if (is_data) {
			break;
		}
	}
	logger::log("Trimmed " + std::to_string(l) + " samples off from the beginning.");
	for (r = input_size - 1; r > l; r -= TARGET_CHANNELS) {
		bool is_data{};
		for (int j = 0; j < TARGET_CHANNELS; j++) {
			if (input[r - j] != 0) {
				is_data = true;
				break;
			}
		}
		if (is_data) {
			break;
		}
	}
	logger::log("Trimmed " + std::to_string(input_size - r) + " samples off from the end.");
	*output_size = r - l + (input_size % TARGET_CHANNELS == 0);
	auto* const output = new int16_t[*output_size];
	std::copy_n(input + l, *output_size, output);
	return output;
}

void wait_for_another_user() {
	std::mutex mutex;
	std::unique_lock L(mutex);
	join_cv.wait(L, [] { return someone_joined.load(); });
}

void send_audio(int16_t const input[], sf_count_t const input_size) {
	for (int i = 0; i < input_size; i += dpp::send_audio_raw_max_length / 2) {
		auto const samples_to_send = std::min <sf_count_t>(dpp::send_audio_raw_max_length / 2, input_size - i);
		if (dpp::find_channel(CHANNEL_ID)->get_voice_members().size() > 1) {
			dpp::discord_client* curr_shard = shard();
			if (curr_shard == nullptr) {
				std::this_thread::sleep_for(std::chrono::milliseconds(samples_to_send / TARGET_CHANNELS / (TARGET_SAMPLE_RATE / 1000)));
				continue;
			}
			std::shared_lock L(curr_shard->voice_mutex);
			dpp::discord_voice_client* voice_client = get_voice_client();
			if (voice_client == nullptr) {
				std::this_thread::sleep_for(std::chrono::milliseconds(samples_to_send / TARGET_CHANNELS / (TARGET_SAMPLE_RATE / 1000)));
			}
			else {
				voice_client->send_audio_raw(
					reinterpret_cast <uint16_t*>(
						const_cast <int16_t*>(input + i)
					),
					samples_to_send * 2
				);
				L.unlock();
				while (true) {
					curr_shard = shard();
					if (curr_shard == nullptr) {
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						continue;
					}
					std::shared_lock L2(curr_shard->voice_mutex);
					voice_client = get_voice_client();
					if (voice_client != nullptr) {
						if (voice_client->get_secs_remaining() > 1.0f) {
							L2.unlock();
							std::this_thread::sleep_for(std::chrono::milliseconds(20));
						}
						else {
							break;
						}
					}
					else {
						L2.unlock();
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
					}
				}
			}
		}
		else if (PAUSE_WHEN_ALONE) {
			someone_joined = false;
			wait_for_another_user();
			i -= dpp::send_audio_raw_max_length / 2;
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(samples_to_send / TARGET_CHANNELS / (TARGET_SAMPLE_RATE / 1000)));
		}
	}
	dpp::discord_client* const curr_shard = shard();
	if (curr_shard == nullptr) {
		return;
	}
	std::shared_lock L(curr_shard->voice_mutex);
	dpp::discord_voice_client* const voice_client = get_voice_client();
	if (voice_client != nullptr) {
		std::chrono::milliseconds const sleep_time{static_cast <int>(voice_client->get_secs_remaining() * 1000)};
		L.unlock();
		std::this_thread::sleep_for(sleep_time);
	}
}

int16_t* prepend_silence(int16_t const input[], sf_count_t const input_size, sf_count_t const silence_samples) {
	auto* const output = new int16_t[silence_samples + input_size];
	std::fill_n(output, silence_samples, 0);
	std::copy_n(input, input_size, output + silence_samples);
	return output;
}

void play_file(size_t const file_num, bool const to_prepend_silence) {
	bool failed{};
	try {
		SndfileHandle audio_file;
		std::fstream input_file;
		// TODO: fast open
		auto const& path = file_entries[file_num].path();
		if (path.extension() == ".pcm" || path.extension() == ".raw") {
			audio_file = SndfileHandle(path, SFM_READ, SF_FORMAT_RAW | SF_FORMAT_PCM_16, TARGET_CHANNELS, TARGET_SAMPLE_RATE);
		}
		else {
			audio_file = SndfileHandle(path, SFM_READ);
		}
		int const channels = audio_file.channels();
		sf_count_t const frames = audio_file.frames();
		sf_count_t const samplerate = audio_file.samplerate();
		sf_count_t samples = frames * channels;

		logger::log("Original frames:       " + std::to_string(frames));
		logger::log("Original sample count: " + std::to_string(samples));
		logger::log("Original sample rate:  " + std::to_string(samplerate));
		logger::log("Original channels:     " + std::to_string(channels));

		if (frames == 0) {
			logger::log("Couldn't read `" + file_entries[file_num].path().filename().string() + "`: empty/non-decodable file");
			throw self_invoked_exception{};
		}

		std::unique_ptr <float[]> float_sample_buffer(new float[samples]);
		audio_file.read(float_sample_buffer.get(), samples);

		if (samplerate != TARGET_SAMPLE_RATE) {
			auto const samplerate_ratio = static_cast <float>(TARGET_SAMPLE_RATE) / static_cast <float>(samplerate);
			sf_count_t const new_frames = frames * samplerate_ratio;
			sf_count_t const new_samples = new_frames * channels;

			logger::log("Sample rate ratio: " + std::to_string(samplerate_ratio));
			logger::log("New frames:        " + std::to_string(new_frames));
			logger::log("New sample count:  " + std::to_string(new_samples));

			std::unique_ptr <float[]> float_sample_buffer_out(new float[new_samples]);
			SRC_DATA src_data = {
				.data_in = float_sample_buffer.get(),
				.data_out = float_sample_buffer_out.get(),
				.input_frames = frames,
				.output_frames = new_frames,
				.src_ratio = samplerate_ratio,
			};
			src_simple(&src_data, SRC_SINC_BEST_QUALITY, channels);
			float_sample_buffer.swap(float_sample_buffer_out);
			samples = new_samples;
		}

		auto [min_ptr, max_ptr] = std::minmax_element(float_sample_buffer.get(), float_sample_buffer.get() + samples);
		float const abs_max = std::max(std::abs(*min_ptr), std::abs(*max_ptr));
		if (NORMALISE_NON_CLIPPING || abs_max > std::abs(NORMALISE_TO)) {
			float_sample_buffer.reset(normalise(float_sample_buffer.get(), samples));
		}

		std::unique_ptr <int16_t[]> int16_sample_buffer(new int16_t[samples]);
		src_float_to_short_array(float_sample_buffer.get(), int16_sample_buffer.get(), samples);
		if (channels != TARGET_CHANNELS) {
			logger::log("Non-stereo audio is unplayable by D++. Converting.");
			int16_t* const stereo_sample_buffer = to_stereo(int16_sample_buffer.get(), samples, channels);
			if (stereo_sample_buffer == nullptr) {
				throw self_invoked_exception{};
			}
			logger::log("Converted!");
			int16_sample_buffer.reset(stereo_sample_buffer);
			samples = samples / channels * TARGET_CHANNELS;
			logger::log("New sample count: " + std::to_string(samples));
		}
		if (TRIM_SILENCE) {
			int16_t* const silence_less = trim_off_silence(int16_sample_buffer.get(), samples, &samples);
			if (silence_less == nullptr) {
				throw self_invoked_exception{};
			}
			int16_sample_buffer.reset(silence_less);
		}
		if (to_prepend_silence) {
			logger::log("No song has been sent yet. Prepending silence.");
			int16_sample_buffer.reset(prepend_silence(int16_sample_buffer.get(), samples, TARGET_SAMPLE_RATE));
			samples += TARGET_SAMPLE_RATE;
		}
		size_t const int16_buffer_bytes = samples * sizeof(int16_t);
		logger::log("To be played size (in bytes): " + std::to_string(int16_buffer_bytes));
		if (int16_buffer_bytes % 4 != 0) {
			logger::log("The size isn't divisible by 4 and the audio is therefore unplayable!");
			throw std::exception{};
		}
		played_once = true;
		if (!TEST_MODE) {
			if (DISPLAY_PLAYLIST) {
				dpp::message msg(CHANNEL_ID, trim_file_list(file_num));
				msg.id = MESSAGE_ID;
				bot->message_edit(msg, [msg](dpp::confirmation_callback_t const& edit_callback) {
					if (edit_callback.is_error() && edit_callback.get_error().code == dpp::err_unknown_message) {
						logger::log("Error editing the playlist message! Creating a new one.");
						bot->message_create(msg, [](dpp::confirmation_callback_t const& create_callback) {
							if (create_callback.is_error()) {
								logger::log("Error creating the playlist message! Giving up.");
								return;
							}
							MESSAGE_ID = create_callback.get <dpp::message>().id;
							bot->message_add_reaction(MESSAGE_ID, CHANNEL_ID, dpp::unicode_emoji::white_check_mark);
						});
					}
				});
			}
			logger::log("Playing now!");
			curr_file_path = '`' + file_entries[file_num].path().relative_path().string().substr(RESOURCES_PATH.size()) + '`';
			send_audio(int16_sample_buffer.get(), samples);
			std::this_thread::sleep_for(std::chrono::seconds(TRANSITION_DELAY_SECONDS));
			curr_file_path = "*Preparing to play next*";
		}
		else {
			logger::log("Audio file PASSED!");
			++passed_files;
		}
	}
	catch (self_invoked_exception const&) {
		failed = true;
	}
	catch (std::exception const& e) {
		failed = true;
		logger::log(std::string("Uncaught exception: ") + e.what());
	}
	if (TEST_MODE && failed) {
		logger::log("Audio file FAILED!");
		++failed_files;
	}
}
