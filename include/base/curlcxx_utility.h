// The MIT License (MIT)
//
// Copyright (c) <2023> chromabox <chromarockjp@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <exception>
#include <utility>
#include <vector>
#include <algorithm>
#include <mutex>
#include <regex>
#include <curl/curl.h>

#include <cstdio>


namespace libcurlcxx
{
	// c++11でFormatを使う sprintf系のような使い心地ができる (gcc13では必要なくなるかも)
	// FormatしたStringを戻り値として返す
	template <typename ... Args>
	std::string format(const std::string& fmt, Args ... args )
	{
		size_t len = std::snprintf(nullptr, 0, fmt.c_str(), args ...);
		std::vector<char> buf(len + 1);
		std::snprintf(&buf[0], len + 1, fmt.c_str(), args ...);
		return std::string(&buf[0], &buf[0] + len);
	}

	class curl_base_utility
	{
	private:
		// regexを用いてURL文字から切り出す
		// indexは範囲超えていても構わないので気にしなくていいがマッチしない場合は空文字を返す
		// index: https://datatracker.ietf.org/doc/html/rfc3986#appendix-B
		// 2 = scheme
		// 4 = authority
		// 5 = path
		// 7 = query
		// 9 = fragment
		// これ以外の値はあまり意味がない
		static inline std::string _url_match_regex(std::string_view url, int index)
		{
			// see also RFC3986 https://datatracker.ietf.org/doc/html/rfc3986#appendix-B
			constexpr std::string_view url_pattern = "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?";
			std::regex pattern(url_pattern.data());
			std::smatch matches;
			std::string fstr(url);      // regex_searchはbasic_string &しかとってくれないのでこうしないといけない...
			if (!std::regex_search(fstr, matches, pattern)) return "";
			// ここは範囲をはみ出しても問題ない(空文字が帰ってくる)
			return matches[index];
		}

	public:
		static time_t get_date(std::string_view format);

		static std::string url_escape(std::string_view url);
		static std::string url_unescape(std::string_view url);

		// RFC3986に基づくURLからの文字切り出し関連
		// urlから"http"の文字を取得
		static std::string url_from_scheme(std::string_view url){
			return _url_match_regex(url, 2);
		}
		// urlから"google.com"のサーバを示す文字を取得
		static std::string url_from_authority(std::string_view url){
			return _url_match_regex(url, 4);
		}
		// urlから"/pathname/filename.jpg"のパス名とファイル名を示す文字を取得
		static std::string url_from_path(std::string_view url){
			return _url_match_regex(url, 5);
		}
		// urlから"?=aaa"のクエリを示す文字を取得
		static std::string url_from_query(std::string_view url){
			return _url_match_regex(url, 7);
		}
		// urlから"#aaa"のフラグメントを取得
		static std::string url_from_fragment(std::string_view url){
			return _url_match_regex(url, 9);
		}
		// urlからパス名を完全に抜いた"filename.jpg"というようなファイル名だけを取得
		static std::string url_from_filename(std::string_view url){
			std::string fpath = url_from_path(url);
			if(fpath.empty()) return fpath;

			size_t found = fpath.find_last_of("/\\");
			if (found != std::string::npos) {
				return fpath.substr(found + 1);
			}
			return fpath;		// セパレータがない場合はファイル名だけを返す。これでよいはず
		}

		static unsigned int get_curl_version_number() noexcept;
		static unsigned int get_curlcxx_version_number() noexcept;
		static constexpr std::string_view get_curlcxx_version_string() noexcept;

	private:
		curl_base_utility() = default;
	};
}  // namespace libcurlcxx
