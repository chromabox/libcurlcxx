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

#include <algorithm>
#include <memory>
#include <typeinfo>
#include <string>

#include <curl/curl.h>
#include <curl/easy.h>

#include "curlcxx_base_object.h"
#include "curlcxx_stream.h"
#include "curlcxx_mime.h"

namespace libcurlcxx
{
	class curl_base_mime;
	class curl_base_easy;

	// プログレスを使うときのコールバック関数
	// この形式で外部から定義すること
	typedef int (*curl_base_easy_progress_callback)(
		curl_base_easy *easy,
		void *prog_udata,
		curl_off_t dltotal,
		curl_off_t dlnow,
		curl_off_t ultotal,
		curl_off_t ulnow
	);

	// EasyハンドルのDeleter専用
	// Easyハンドルがどこからも参照がなくなった場合に呼ばれて安全に解放される
	struct _curl_easy_handle_deleter
	{
		void operator()(CURL *_ceh) const
		{
			if (_ceh == nullptr) return;  // nullptrのときは何もしない
//  std::cout << "clean easy" << std::endl;
			curl_easy_cleanup(_ceh);
		}
	};
	// CURLのunique_ptr ハンドル構造。ユニークなポインタとする
	typedef std::unique_ptr<CURL, _curl_easy_handle_deleter> curl_easy_unique_handle;

	// CURLのEasyハンドルをラッピングしているクラス
	class curl_base_easy : public curl_base_object
	{
	protected:
		curl_easy_unique_handle	handle;		// Easyハンドルそのものを管理
		std::string				url;		// 設定したURL
		std::shared_ptr<curl_base_mime>	mime;  // 設定したmime
		std::shared_ptr<curl_base_stream_object>	streamer;  // 設定したstreamer
		long int				connect_timeout;		// 接続タイムアウト秒数(デフォルトは300秒＝CURLのデフォルトと同じ)

		void									*prog_data;	   // progress用データ
		curl_base_easy_progress_callback         prog_callbk;  // progressを使うときに呼ばれるコールバック関数
		// コピー禁止
		curl_base_easy &operator=(curl_base_easy const &) = delete;
		curl_base_easy(curl_base_easy const &) = delete;

	protected:
		virtual void set_error(const int curl_code) noexcept;

		static int _debug_callback_func(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr);
		virtual int internal_debug_callback(CURL *handle, curl_infotype type, char *data, size_t size);

		static int _xfer_callback_func(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
		virtual int internal_xfer_callback(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

		bool move_set_mime(std::shared_ptr<curl_base_mime> &_mime);

	public:
		curl_base_easy();
		explicit curl_base_easy(const std::shared_ptr<curl_base_stream_object> &_streamer);

		curl_base_easy(curl_base_easy &&) noexcept;
		curl_base_easy &operator= (curl_base_easy &&) noexcept;

		bool operator==(const curl_base_easy &easy) const;

		~curl_base_easy() noexcept;

		virtual void perform();

		// Streamerを設定する
		void set_streamer(const std::shared_ptr<curl_base_stream_object> &_streamer);
		// Streamerを取得する
		inline curl_base_stream_object* get_streamer() const { return streamer.get();}

		bool set_url(std::string_view _url);
		// 設定したURLを取得
		inline std::string get_url() const noexcept { return url;}

		bool set_mime(const std::shared_ptr<curl_base_mime> &_mime);

		void set_progress_function(bool onoff, curl_base_easy_progress_callback func = nullptr, void *param = nullptr);

		void set_verbose(bool onoff);
		void set_debugdump(bool onoff);

		// curl_easy_setopt直接呼び出し
		inline CURLcode set_option(CURLoption option, long param) noexcept 			{return curl_easy_setopt(handle.get(), option, param);};
		inline CURLcode set_option(CURLoption option, char* param) noexcept 			{return curl_easy_setopt(handle.get(), option, param);};
		inline CURLcode set_option(CURLoption option, const char* param) noexcept 	{return curl_easy_setopt(handle.get(), option, param);};
		inline CURLcode set_option(CURLoption option, void *param) noexcept 	{return curl_easy_setopt(handle.get(), option, param);};
		inline CURLcode set_option(CURLoption option, const curl_base_stream_object *param) noexcept 	{return curl_easy_setopt(handle.get(), option, param);};
		inline CURLcode set_option(CURLoption option, const curl_mime* param) noexcept 	{return curl_easy_setopt(handle.get(), option, param);};
		inline CURLcode set_option(CURLoption option, const curl_slist* param) noexcept 	{return curl_easy_setopt(handle.get(), option, param);};
		inline CURLcode set_option(CURLoption option, std::string_view param) noexcept 	{return curl_easy_setopt(handle.get(), option, param.data());};

		inline CURLcode set_write_callback(curl_write_callback param) noexcept 			{return curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, param);};
		inline CURLcode set_xferinfo_callback(curl_xferinfo_callback param) noexcept 	{return curl_easy_setopt(handle.get(), CURLOPT_XFERINFOFUNCTION, param);};

		inline CURLcode set_debug_callback(curl_debug_callback param) noexcept 			{return curl_easy_setopt(handle.get(), CURLOPT_DEBUGFUNCTION, param);};

		// 接続タイムアウト時間を設定(接続が失敗とみなされるまでの秒数を設定)
		// タイムアウトまでの時間は、名前解決 (DNS) と、リモート側との接続が確立されるまでのすべてのプロトコルハンドシェイクとネゴシエーションの時間が含まれる
		// 実データの送受信時間は含まれない
		// touttimeを0に設定するとデフォルトの300秒となる
		inline void set_connect_timeout(long int touttime)
		{
			if(touttime == 0) touttime = 300;
			connect_timeout = touttime;
			set_option(CURLOPT_CONNECTTIMEOUT, connect_timeout);
		}

		// curl_easy_setoptで設定したオプションのクリア。実際にはnullptrをセットする
		inline CURLcode clear_option(CURLoption option) noexcept 				{return curl_easy_setopt(handle.get(), option, nullptr);};

		// curl_easy_getinfo直接呼び出し
		inline CURLcode get_info(CURLINFO info, long &rparam) noexcept			{return curl_easy_getinfo(handle.get(), info, &rparam);}
		inline CURLcode get_info(CURLINFO info, std::string &rparam)	{
			char *ptr;
			rparam.erase();
			CURLcode ret = curl_easy_getinfo(handle.get(), info, &ptr);
			if(ptr != nullptr) rparam = ptr;
			return ret;
		}

		// 接続タイムアウト時間を返す(接続が失敗とみなされるまでの秒数)
		inline long int get_connect_timeout() const {
			return connect_timeout;
		}

		// CURLの生ハンドルを取得する
		inline CURL *get_chandle() const noexcept	{ return handle.get();}
	};
}  // namespace libcurlcxx
