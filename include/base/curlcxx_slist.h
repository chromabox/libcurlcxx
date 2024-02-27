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

#include <curl/curl.h>
#include <algorithm>
#include <memory>
#include <string>

namespace libcurlcxx
{
	struct _curl_slist_handle_deleter
	{
		void operator()(curl_slist *_ceh) const
		{
			if (_ceh == nullptr) return;		// nullptrのときは何もしない
//  std::cout << "clean slist" << std::endl;
			curl_slist_free_all(_ceh);
		}
	};

	// curl_mime のunique_ptr ハンドル構造。ユニークなポインタとする
	typedef std::unique_ptr<curl_slist, _curl_slist_handle_deleter> curl_slist_unique_handle;

	// curl_slistをラッピングしたクラス。主にpostやget時のHTTPヘッダ設定に使われる
	class curl_base_slist
	{
	private:
		curl_slist_unique_handle _slist;		// slist構造のユニークポインタ

		// コピー禁止
		curl_base_slist &operator=(curl_base_slist const &) = delete;
		curl_base_slist(curl_base_slist const &) = delete;

	public:
		curl_base_slist();
		curl_base_slist(curl_base_slist &&) noexcept;
		curl_base_slist &operator= (curl_base_slist &&) noexcept;

		virtual ~curl_base_slist();

		void append(std::string_view value);
		void reset();

		// 外向けにslist構造を返す
		inline const curl_slist* get_slistptr() const {return _slist.get();}

		// 外向けにslist構造を返すが所有権を放棄するため自動でRemoveが掛からなくなる。mimeに渡すためのもの。使うには注意すること
		inline curl_slist* release_slistptr() {return _slist.release();}

		// SList構造が有効かどうか
		inline bool isempty() const noexcept {
			if(_slist) return false;
			return true;		// 空である
		}
	};
}  // namespace libcurlcxx

