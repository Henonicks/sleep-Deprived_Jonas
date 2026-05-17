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

#include "jonas/file_list.hpp"

#include "jonas/jonas.hpp"

constexpr std::string insert_tabs(int const tabs) {
	return std::string(2 * tabs, ' ');
}

[[nodiscard]] std::vector <stdfs::directory_entry> make_file_list_impl(std::string_view const path, int const depth) {
	std::vector <stdfs::directory_entry> buffer;
	std::ranges::transform(stdfs::directory_iterator(path), std::back_inserter(buffer), [](auto const& x) {
		return x;
	});
	std::ranges::sort(buffer, [](auto const& e1, auto const& e2) {
		if (e1.is_directory() == e2.is_directory()) {
			return strnatcasecmp(e1.path().string().c_str(), e2.path().string().c_str()) == COMP_LESS;
		}
		return e1.is_directory() > e2.is_directory();
	});
	std::vector <stdfs::directory_entry> res;
	for (stdfs::directory_entry const& x : buffer) {
		res.push_back(x);
		file_list_str += insert_tabs(depth) + x.path().filename().string() + '\n';
		if (x.is_directory()) {
			file_list_str.insert(file_list_str.size() - 1, "/");
			std::vector <stdfs::directory_entry> const deeper_list = make_file_list_impl(x.path().string(), depth + 1);
			res.insert(res.end(), deeper_list.begin(), deeper_list.end());
		}
	}
	return res;
}

[[nodiscard]] std::vector <stdfs::directory_entry> make_file_list(std::string_view const path) {
	file_list_str.clear();
	return make_file_list_impl(path, 0);
}

[[nodiscard]] std::string trim_file_list(size_t line) {
	std::string res = file_list_str;
	size_t n = std::ranges::count(res, '\n');
	size_t cnt{};
	auto const curr_it = std::ranges::find_if(res, [&cnt, line](char const x) {
		return x == '\n' && cnt++ == line;
	});
	auto const prev_rit = std::find(res.rend() - (curr_it - res.begin()), res.rend(), '\n');
	size_t const prev_pos = res.rend() - prev_rit;
	res.insert(curr_it - res.begin(), "**");
	res.insert(prev_pos, "**");
	while (n > 0 && res.size() > MAX_MSG_CHAR_CNT) {
		if (line <= n / 2) {
			res.erase(res.rend() - std::find(res.rbegin(), res.rend(), '\n') - 1 + res.begin(), res.end());
		}
		else {
			res.erase(0, res.find('\n') + 1);
			--line;
		}
		--n;
	}
	return res;
}
