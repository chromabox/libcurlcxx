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
#include <deque>
#include <algorithm>
#include <mutex>
#include <curl/curl.h>

#include "curlcxx_base_object.h"

namespace libcurlcxx
{
	// エラーログ用構造
	class curl_base_error
	{
	private:
		std::string message;            // 詳細なエラーメッセージ
		std::string funcname;           // エラーを起こした関数名
		int         funcline;           // エラーを起こした行番号
		int         errorcode;          // Curlのエラーコード(あれば)

	public:
		curl_base_error(std::string_view mess, std::string_view func, const int line) noexcept;
		curl_base_error(const libcurlcxx::curl_base_object* obj, std::string_view func, const int line) noexcept;

		inline std::string get_message() const noexcept {return message;}
		inline std::string get_funcname() const noexcept {return funcname;}
		inline int get_funcline()         const noexcept {return funcline;}
	};

	// エラー構造ログを貯めるためのキュー定義
	using curl_base_errqueue = std::deque<curl_base_error>;

	// cURLにまつわるエラー例外クラス
	class curl_base_exception : public std::exception
	{
	private:
		static curl_base_errqueue errlog;       // エラーログ。これはStaticで持っているので全共通
		static std::mutex errlog_lk;            // errlog用Mutex。これはStaticで持っているので全共通

		std::string err_mes;                    // 直近のエラーメッセージ

	public:
		curl_base_exception(std::string_view mess, std::string_view func, const int line) noexcept;
		curl_base_exception(const libcurlcxx::curl_base_object* obj, std::string_view func, const int line) noexcept;
		curl_base_exception(const curl_base_exception &) noexcept;

		curl_base_exception & operator=(curl_base_exception const&);

		~curl_base_exception() noexcept override = default;

		virtual const char* what() const noexcept;

		static curl_base_errqueue get_errlog();
		static void print_errlog();
		static void clear_errlog();
		// 直近で発生したエラーメッセージを取得する
		inline std::string get_errmes() const noexcept {return err_mes;}
	};
}  // namespace libcurlcxx
