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

#ifndef JONAS
#define JONAS

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include <sndfile.hh>
#include <samplerate.h>

#include "strnatcmp/strnatcmp.hpp"

#include "henifig/henifig.hpp"

#include "jonas/audio.hpp"
#include "jonas/cfg.hpp"
#include "jonas/exception.hpp"
#include "jonas/file_list.hpp"
#include "jonas/logging.hpp"

enum string_comparisons : int8_t {
	COMP_LESS    = -1,
	COMP_EQUAL   =  0,
	COMP_GREATER =  1,
};

constexpr int MAX_MSG_CHAR_CNT = 2000;
constexpr int TARGET_SAMPLE_RATE = 48'000;
constexpr int TARGET_CHANNELS = 2;
inline int TRANSITION_DELAY_SECONDS = 1;
inline bool PAUSE_WHEN_ALONE = false;
inline bool DISPLAY_PLAYLIST = true;
inline bool SNAP_TO_CHANNEL = true;
inline bool TEST_MODE = false;

namespace stdfs = std::filesystem;

inline std::atomic <dpp::snowflake> CHANNEL_ID;
inline dpp::snowflake GUILD_ID, MESSAGE_ID;
inline dpp::cluster* bot;
inline dpp::discord_client* shard;

inline bool played_once;

void run();

dpp::discord_voice_client* get_voice_client();

#endif
