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

#include <map>
#include <memory>
#include "curlcxx_cdtor.h"
#include "curlcxx_easy.h"
#include "curlcxx_utility.h"
#include "curlcxx_error.h"

using libcurlcxx::curl_base_cdtor;
using libcurlcxx::curl_base_stringstream;
using libcurlcxx::curl_base_easy;
using libcurlcxx::curl_base_utility;
using libcurlcxx::curl_base_exception;


using std::string;
using std::ostream;
using std::ostringstream;
using std::map;

// 使用の際はこれの定義が必要
static libcurlcxx::curl_base_cdtor _libcurl;

// GETリクエスト（引数なし）サンプル
int main()
{
	// 直接StringStreamを入れる場合はこれ。ただしStreamerから取り出すときが若干面倒になる
	curl_base_easy easy(std::make_shared<curl_base_stringstream>());
	std::string_view url = "https://www.gaitameonline.com/rateaj/getrate";

	easy.set_option(CURLOPT_FOLLOWLOCATION, 1L);
	easy.set_option(CURLOPT_HTTPGET, 1L);
	easy.set_url(url);
	std::cout << "url: " << url << std::endl;

	try {
		easy.perform();
	} catch (curl_base_exception &error) {
		// エラー内容表示
		std::cerr << error.what() << std::endl;
		return -1;
	}
	// httpコード取得
	long httpcode = 0;
	easy.get_info(CURLINFO_RESPONSE_CODE, httpcode);
	if(httpcode != 200){
		std::cout << "htto code error " << httpcode << std::endl;
		return -1;
	}
	std::cout << easy.get_streamer()->get_string() << std::endl;

	return 0;
}
