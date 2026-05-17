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

#ifndef FILE_LIST
#define FILE_LIST

#include <vector>
#include <filesystem>

namespace stdfs = std::filesystem;

inline std::vector <stdfs::directory_entry> file_entries;
inline std::string file_list_str;

constexpr std::string insert_tabs(int tabs);
[[nodiscard]] std::vector <stdfs::directory_entry> make_file_list_impl(std::string_view path, int depth);
[[nodiscard]] std::vector <stdfs::directory_entry> make_file_list(std::string_view path);
[[nodiscard]] std::string trim_file_list(size_t line);

#endif
