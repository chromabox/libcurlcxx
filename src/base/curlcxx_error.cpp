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

#include "curlcxx_error.h"
#include "curlcxx_utility.h"

using libcurlcxx::curl_base_object;
using libcurlcxx::curl_base_error;
using libcurlcxx::curl_base_errqueue;
using libcurlcxx::curl_base_exception;
using libcurlcxx::format;

// 以下2つはStaticであるのでここに実体を定義する必要がある
libcurlcxx::curl_base_errqueue libcurlcxx::curl_base_exception::errlog;			// エラーログ（全共通）
std::mutex libcurlcxx::curl_base_exception::errlog_lk;							// エラーログロック（全共通）

using std::deque;
using std::string;
using std::string_view;

// curl_base_error : curlの例外オブジェクト(1要素)のC++実装

// コンストラクタ
// mess: エラーメッセージ詳細
// func: エラーを起こした関数
// line: エラーを起こした行番号
curl_base_error::curl_base_error(std::string_view mess, std::string_view func, const int line) noexcept
{
	message = mess;
	errorcode = 0;
	funcname = func;
	funcline = line;
}

// コンストラクタ
// obj: エラーを起こしたCurlオブジェクト(curl_base_objectの派生のみ)
// func: エラーを起こした関数
// line: エラーを起こした行番号
curl_base_error::curl_base_error(const libcurlcxx::curl_base_object* obj, std::string_view func, const int line) noexcept
{
	message = obj->get_errorstr();
	errorcode = obj->get_errorcode();
	funcname = func;
	funcline = line;
}

// -----------------------------------------------------------------------------------------------------------------------
// curl_base_exception : curlの例外オブジェクトのC++実装。なにかエラーが発生したら基本的にこれをthrowで投げる

// コンストラクタ。これをthrowで投げること
// mess: エラーメッセージ詳細
// func: エラーを起こした関数
// line: エラーを起こした行番号
curl_base_exception::curl_base_exception(std::string_view mess, std::string_view func, const int line) noexcept
{
	std::scoped_lock lk{errlog_lk};			// ScopedLockでなくていいんだけどとりあえず
	curl_base_error e(mess, func, line);

	errlog.push_front(e);
	// 直近エラーの文字列化をする。what()でこれを返すためである
	err_mes = libcurlcxx::format("error: %s,func %s line %d", e.get_message().c_str(), e.get_funcname().c_str(), e.get_funcline());
}

// コンストラクタ(easyやmultiなどのcurlオブジェクト用)。これをthrowで投げること
// obj: エラーを起こしたCurlオブジェクト(curl_base_objectの派生のみ)
// func: エラーを起こした関数
// line: エラーを起こした行番号
curl_base_exception::curl_base_exception(const libcurlcxx::curl_base_object* obj, std::string_view func, const int line) noexcept
{
	std::scoped_lock lk{errlog_lk};			// ScopedLockでなくていいんだけどとりあえず
	curl_base_error e(obj, func, line);
	errlog.push_front(e);
	// 直近エラーの文字列化をする。what()でこれを返すためである
	err_mes = libcurlcxx::format("error: %s, func %s line %d", e.get_message().c_str(), e.get_funcname().c_str(), e.get_funcline());
}

// コピーコンストラクタ
curl_base_exception::curl_base_exception(const curl_base_exception &object) noexcept
{
	errlog = object.get_errlog();
	err_mes = object.get_errmes();
}

// ただの代入
curl_base_exception& curl_base_exception::operator=(curl_base_exception const &object)
{
	if (&object != this) {
		errlog = object.get_errlog();
		err_mes = object.get_errmes();
	}
	return *this;
}

// エラーログ本体キューを外部から取得
curl_base_errqueue curl_base_exception::get_errlog()
{
	std::scoped_lock lk{errlog_lk};
	curl_base_errqueue tmp = errlog;
	return tmp;
}

// エラーログをSTDOUTに出力する
// 最後に発生したエラーから表示される。デバッグ用として使用できる
void curl_base_exception::print_errlog()
{
	std::scoped_lock lk{errlog_lk};
	std::for_each(errlog.begin(), errlog.end(), [](const curl_base_error &value) {
		std::cout << "ERROR: "<< value.get_message() <<" FUNCLINE: "<< value.get_funcline() << std::endl;
	});
}

// エラーログの全クリア
void curl_base_exception::clear_errlog()
{
	std::scoped_lock lk{errlog_lk};
	errlog.clear();
}

// 直近のエラーメッセージ類の出力
const char* curl_base_exception::what() const noexcept
{
	return err_mes.c_str();
}
