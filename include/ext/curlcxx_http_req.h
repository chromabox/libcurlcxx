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

#include <map>
#include <string>
#include <memory>
#include "curlcxx_easy.h"
#include "curlcxx_multi.h"
#include "curlcxx_slist.h"

namespace libcurlcxx
{
	class curl_base_mime;

	// HTTP get 用のリクエストデータ定義。Key=Valueの形でマップ型の定義
	using curl_http_request_param = std::map<std::string, std::string>;

	class curl_http_request : public curl_base_easy
	{
	private:
		// proxy
		std::string				proxyurl;				// ProxyのURL
		std::string				proxyuser;				// Proxyのユーザ
		std::string				proxypass;				// Proxyのパスワード
		long int				proxyport;				// Proxyのポート番号

		curl_base_slist			http_header;		// 設定したカスタムヘッダ

		// コピー禁止
		curl_http_request &operator=(curl_http_request const &) = delete;
		curl_http_request(curl_http_request const &) = delete;

		std::string build_get_param(const curl_http_request_param& params);
		bool build_post_param(const curl_http_request_param& params);

	protected:
		void setInternalProxy();

	public:
		curl_http_request();
		explicit curl_http_request(const std::shared_ptr<curl_base_stream_object> &_streamer);

		curl_http_request(curl_http_request &&) noexcept;
		curl_http_request &operator= (curl_http_request &&) noexcept;

		~curl_http_request() noexcept;

		virtual bool RequestSetupGet(std::string_view url);
		virtual bool RequestSetupGet(std::string_view url, const curl_http_request_param& params);

		virtual bool RequestSetupPost(std::string_view url, const std::shared_ptr<curl_base_mime> &mimes);
		virtual bool RequestSetupPost(std::string_view url, const curl_http_request_param& params);
		virtual bool RequestSetupPost(std::string_view url, std::string_view strdata);

		virtual void prePerform();
		virtual void perform();

		virtual void appendHeader(std::string_view data);
		virtual void removeHeader();

		// httpのレスポンスコードを返す
		inline const long get_responceCode() noexcept
		{
			long httpcode = 0;
			get_info(CURLINFO_RESPONSE_CODE, httpcode);
			return httpcode;
		}
		// httpのcontent-typeを返す
		inline std::string get_ContentType()
		{
			std::string rstring;
			get_info(CURLINFO_CONTENT_TYPE, rstring);
			return rstring;
		}

		// httpのcontent-lengthを返す
		inline const curl_off_t get_ContentLength()
		{
			curl_off_t length;
			get_info(CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, length);
			return length;
		}
		// performした結果、サーバから帰ってきたデータをStringで受け取る(streamがstringを返せる場合のみ)
		inline std::string get_ContentString() const &
		{
			return get_streamer()->get_string();
		}

		inline void setProxy(std::string_view url, std::string_view user, long int port, std::string_view passwd)
		{
			proxyurl = url;
			proxyuser = user;
			proxypass = passwd;
			proxyport = port;
		}
	};

	// multi messageからこのリクエストクラスのポインタを返す(multi用)
	//
	// mes: multi.get_next_messageで取ってきたcurl_base_multi_messageのオブジェクト
	// return:
	//  messageに対応したcurl_easyのポインタを返す
	inline curl_http_request * getRequestPtr(const curl_base_multi_message &mes)
	{
		return static_cast<curl_http_request *>(mes.get_easy());
	}
}  // namespace libcurlcxx
