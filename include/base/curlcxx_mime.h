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
#include <iostream>
#include <string>
#include <memory>
#include "curlcxx_easy.h"

namespace libcurlcxx
{
	class curl_base_easy;
	struct _curl_mime_handle_deleter
	{
		void operator()(curl_mime *_ceh) const
		{
			if (_ceh == nullptr) return;		// nullptrのときは何もしない
//  std::cout << "clean mime" << std::endl;
			curl_mime_free(_ceh);
		}
	};

	// curl_mime のunique_ptr ハンドル構造。ユニークなポインタとする
	typedef std::unique_ptr<curl_mime, _curl_mime_handle_deleter> curl_mime_unique_handle;

	// curl_mimeをラッピングしたクラス。CURLでPOSTするときの引数を渡す目的がほとんど
	class curl_base_mime
	{
	private:
		curl_mime_unique_handle _mime;		// mime構造のユニークポインタ

	private:
		void set_name_part(curl_mimepart *part, std::string_view name);
		curl_mimepart* create_part();

		// コピー禁止
		curl_base_mime &operator=(curl_base_mime const &) = delete;
		curl_base_mime(curl_base_mime const &) = delete;

	public:
		void add_part(std::string_view name, std::string_view str);
		void add_part(std::string_view name, const char* data, size_t size);

		// 外向けにmime構造を返す
		inline const curl_mime* get_mimeptr() const {return _mime.get();}

		explicit curl_base_mime(const libcurlcxx::curl_base_easy &easy);

		curl_base_mime(curl_base_mime &&) noexcept;
		curl_base_mime &operator= (curl_base_mime &&) noexcept;

		virtual ~curl_base_mime();
	};
}  // namespace libcurlcxx
