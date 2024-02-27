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
#include "curlcxx_mime.h"
#include "curlcxx_http_req.h"
#include "curlcxx_utility.h"
#include "curlcxx_error.h"

using libcurlcxx::curl_base_cdtor;
using libcurlcxx::curl_base_stringstream;
using libcurlcxx::curl_base_utility;
using libcurlcxx::curl_base_exception;
using libcurlcxx::curl_base_mime;

using libcurlcxx::curl_http_request;
using libcurlcxx::curl_http_request_param;

using std::string;
using std::ostream;
using std::ostringstream;
using std::map;

// 使用の際はこれの定義が必要
static libcurlcxx::curl_base_cdtor _libcurl;

// 引数付きGETリクエストサンプル
int main()
{
	std::string_view url("https://httpbin.org/post");

	curl_http_request req(std::make_shared<curl_base_stringstream>());

	try {
#if 0
		// POSTとして投げるデータを設定。
		// 文字列以外のデータがある場合はcurl_base_mimeを使用する
		// この場合はEasyが消去されるまでは大丈夫
		auto mime = std::make_shared<curl_base_mime>(req);
		mime->add_part("name", "aaa");
		mime->add_part("pass", "psps");
		req.RequestSetupPost(url, mime);
#else
		// POSTとして投げるデータを設定。
		// 文字列だけならcurl_http_request_paramを使ったほうが楽
		curl_http_request_param para;
		para["name"] = "aaa";
		para["pass"] = "psps";
		req.RequestSetupPost(url, para);
#endif
	} catch (curl_base_exception &error) {
		// エラー内容表示
		std::cerr << error.what() << std::endl;
		return -1;
	}

	// 詳細なデバッグをしたい場合は以下のコメントを外す
//  req.set_debugdump(true);
//  req.set_verbose(true);

	std::cout << "url: " << req.get_url() << std::endl;

	try {
		req.perform();
	} catch (curl_base_exception &error) {
		// エラー内容表示
		std::cerr << error.what() << std::endl;
		return -1;
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "htto code error " << req.get_responceCode() << std::endl;
		return -1;
	}
	std::cout << req.get_ContentString() << std::endl;

	return 0;
}
