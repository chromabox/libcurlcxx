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

#include <string>
#include <curl/curl.h>

namespace libcurlcxx
{
	// easy や multi などのCurlオブジェクトの基底クラス。これを派生すること。

	class curl_base_object
	{
	public:
		curl_base_object(){
			error_code = 0;
		}
		~curl_base_object() noexcept{}

		// エラー原因をStringで返す
		inline std::string get_errorstr() const noexcept
		{return error_str;}

		// エラー原因をコード(CURLのエラー番号)で返す
		inline int get_errorcode() const noexcept
		{return error_code;}

		// エラー状態のクリア
		inline void clear_error() noexcept
		{error_str.clear();}

	protected:
		virtual void set_error(const int curl_code) noexcept  = 0;

		int error_code;						// CURLM_CODEなど
		std::string error_str;				// for CURLOPT_ERRORBUFFER
	};
}  // namespace libcurlcxx
