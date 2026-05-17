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

#ifndef AUDIO_PLAYER
#define AUDIO_PLAYER

#include <condition_variable>

#include <sndfile.h>

inline std::condition_variable join_cv;
inline std::atomic <bool> someone_joined;

inline size_t passed_files, failed_files;

[[nodiscard]] int16_t* mono_to_stereo(int16_t const input[], sf_count_t input_size);
[[nodiscard]] int16_t* to_stereo(int16_t const input[], sf_count_t input_size, int channels);
[[nodiscard]] int16_t* trim_off_silence(int16_t const input[], sf_count_t input_size, sf_count_t* output_size);
void wait_for_another_user();
void send_audio(int16_t const input[], size_t input_size);
void play_file(size_t file_num, bool to_prepend_silence);

#endif
