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

#include <memory>
#include "curlcxx_utility.h"
#include "curlcxx_error.h"
#include "curlcxx_version.h"

#include "classfname.h"

using libcurlcxx::curl_base_utility;
using libcurlcxx::curl_base_exception;

using std::string;
using std::string_view;
using std::unique_ptr;

static constexpr std::string_view libcurlcxx_version_str("00.01.00");

// 日時の文字(RFC7231のDate項目類)を解析してtime_t型に変換
// curl_getdataを使用
// 失敗したら例外を投げる
//
// format: 日時文字
// ret: 変換結果
time_t curl_base_utility::get_date(const std::string_view format)
{
	const time_t value = curl_getdate(format.data(), nullptr);
	if (value == -1) {
		throw curl_base_exception("error: fails to parse the date string", __FCNAME, __LINE__);
	}
	return value;
}

// curl_easyを使ったURLescape
// url: url値
// ret: 変換結果
std::string curl_base_utility::url_escape(const std::string_view url)
{
	// unique ptr: <class,delfunc>
	// curl easy escapeを使用。第一引数にnullを指定できる
	std::unique_ptr<char, void(*)(char*)> _url_ptr(curl_easy_escape(nullptr, url.data(), static_cast<int>(url.length())), [](char *ptr)
	{
		curl_free(ptr);
	});

	if(_url_ptr == nullptr)	return "";
	return std::string(_url_ptr.get());
}

// curl_easyを使ったURL unescape
// url: url値
// ret: 変換結果
std::string curl_base_utility::url_unescape(const std::string_view url)
{
	// unique ptr: <class,delfunc>
	// curl easy unescapeを使用。第一引数にnullを指定できる
	std::unique_ptr<char, void(*)(char*)> _url_ptr(curl_easy_escape(nullptr, url.data(), static_cast<int>(url.length())), [](char *ptr)
	{
		curl_free(ptr);
	});

	if(_url_ptr == nullptr)	return "";
	return std::string(_url_ptr.get());
}

// 実行されているlibcurlのバージョン番号を得る
unsigned int  curl_base_utility::get_curl_version_number() noexcept
{
	curl_version_info_data *ver = curl_version_info(CURLVERSION_NOW);
	return ver->version_num;
}

// 実行されているこのライブラリ(libcurlcxx)のバージョン番号を得る
unsigned int  curl_base_utility::get_curlcxx_version_number() noexcept
{
	return LIBCURLCXX_VERSION_NUM;
}

// 実行されているこのライブラリ(libcurlcxx)のバージョン文字列を得る
constexpr std::string_view curl_base_utility::get_curlcxx_version_string() noexcept
{
	return libcurlcxx_version_str;
}
