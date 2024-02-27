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

#include <memory>
#include <vector>
#include <unordered_map>

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>

#include "curlcxx_easy.h"


namespace libcurlcxx
{
	// multiハンドルのDeleter専用
	// multiハンドルの参照がどこからもなくなったときに呼ばれて安全に解放される
	struct _curl_multi_handle_deleter
	{
		void operator()(CURLM *_ceh) const
		{
			if (_ceh == nullptr) return;  // nullptrのときは何もしない
			curl_multi_cleanup(_ceh);
		}
	};

	// CURLMのunique_ptr ハンドル構造。ユニークなポインタとする
	using curl_multi_unique_handle = std::unique_ptr<CURLM, _curl_multi_handle_deleter>;

	// curl_multi_info_readで取ってこれるCURLMsg用クラス
	// CURLMsg.msgは現状CURLMSG_DONEしかないのでそれしか考えないとする
	// 現状は転送が終わったものが入ることになる
	class curl_base_multi_message
	{
	private:
		CURLMSG message;			// メッセージコード。現状はCURLMSG_DONEだけ
		// void *whatever;			// unused
		CURLcode code;				// CURLMSG_DONE時のResponceCode
		std::weak_ptr<curl_base_easy> _easy;  // 対象のEasyオブジェクト(WeakPointerにしている);

	public:
		curl_base_multi_message(){}
		explicit curl_base_multi_message(const CURLMsg *msg, const std::weak_ptr<curl_base_easy> &p_easy);

		inline CURLMSG get_message() const 				{return message;}
		inline CURLcode get_code() const 				{return code;}
		inline curl_base_easy* get_easy() const 		{return _easy.lock().get();}
	};

	// cURLのmultiハンドルをラッピングしたクラス。Easyとは異なりスレッドを使用すること無く複数同時リクエストに対応している
	class curl_base_multi : public curl_base_object
	{
	private:
		curl_multi_unique_handle _multi;										// multiハンドル実体
		int active_transfers;													// 残り転送数

		std::unordered_map<CURL*, std::shared_ptr<curl_base_easy>> handles;		// Addで登録しているEasyハンドルとオブジェクトのマップ
																				// 所有権は保持しないといけないのでSharedPtrである
		// コピー禁止
		curl_base_multi &operator=(curl_base_multi const &) = delete;
		curl_base_multi(curl_base_multi const &) = delete;

	protected:
		virtual void set_error(const int curl_code) noexcept;

	public:
		curl_base_multi();

		curl_base_multi(curl_base_multi&&) noexcept;
		curl_base_multi& operator=(curl_base_multi&&) noexcept;

		~curl_base_multi() noexcept;

		void add(const std::shared_ptr<curl_base_easy> &easy);
		void remove(const std::shared_ptr<curl_base_easy> &easy);
		void remove(const libcurlcxx::curl_base_multi_message &msg);
		void clear();

		bool get_next_message(curl_base_multi_message &rmsg, int &msg_in_queue);
		bool perform();

		void wait(struct curl_waitfd [], unsigned int, int, int *);
		void poll(struct curl_waitfd extra_fds[], unsigned int extra_nfds, int timeout_ms, int *numfds);
		void wakeup();

		void timeout(long *);

		// curl_multi_setoptを直接呼び出しするためのもの
		inline CURLMcode set_option(CURLMoption option, long param) noexcept	{return curl_multi_setopt(_multi.get(), option, param);};

		// perform後の残り転送数を取得
		inline int get_active_transfers() const noexcept	{ return active_transfers;}

		// 生ハンドルを取得する
		inline CURLM *get_chandle() const noexcept			{ return _multi.get();}
	};
}  // namespace libcurlcxx
