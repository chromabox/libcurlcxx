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
#include "curlcxx_cdtor.h"
#include "curlcxx_easy.h"
#include "curlcxx_multi.h"
#include "curlcxx_utility.h"
#include "curlcxx_error.h"

using libcurlcxx::curl_base_cdtor;
using libcurlcxx::curl_base_stringstream;
using libcurlcxx::curl_base_easy;
using libcurlcxx::curl_base_multi;
using libcurlcxx::curl_base_multi_message;
using libcurlcxx::curl_base_utility;
using libcurlcxx::curl_base_exception;

using std::string;
using std::string_view;
using std::vector;

// 使用の際はこれの定義が必要
static libcurlcxx::curl_base_cdtor _libcurl;

// VectorでEasyハンドルを後々管理する場合は定義。Multiに任せてしまう場合は定義しない
// #define USE_VECTOR_EASY


// multiリクエストサンプル
int main()
{
	std::vector<std::string_view> urls;
	urls.push_back("http://abehiroshi.la.coocan.jp/"); // やはりこれは必要でしょう
	urls.push_back("https://www.yahoo.com");
	urls.push_back("https://www.wikipedia.org");

	curl_base_multi multi;

#ifdef USE_VECTOR_EASY
	// 所有権をこちらで管理する場合
	std::vector<std::shared_ptr<curl_base_stringstream> > ioss;
	std::vector<std::shared_ptr<libcurlcxx::curl_base_easy> > easyes;
	easyes.reserve(urls.size());
	ioss.reserve(urls.size());
#endif

	// URLリストに従い登録（ラムダ式使ってる）
	std::for_each(urls.begin(), urls.end(), [&](const std::string_view &str) {
#ifdef	USE_VECTOR_EASY
		ioss.emplace_back(std::make_shared<curl_base_stringstream>());
		auto _s = ioss.back();
		easyes.emplace_back(std::make_shared<curl_base_easy>(_s));
		auto _e = easyes.back();
#else
		// 所有権を持たず、完全にEasyクラスに任せる場合はこのようになる
		auto _e = std::make_shared<curl_base_easy>(std::make_shared<curl_base_stringstream>());
#endif
		_e->set_option(CURLOPT_HTTPGET, 1L);
		// HTTP 2 over TLS (HTTPS) のみを試し、ダメな場合はHTTP1.1で通信
		// こうしないと多重化の恩恵が受けられない
		_e->set_option(CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
		_e->set_url(str);
#if (CURLPIPE_MULTIPLEX > 0)
		// 多重化を待ち、確実なものにするために1にする
		_e->set_option(CURLOPT_PIPEWAIT, 1L);
#endif
		// EasyハンドルをMultiハンドルに教える
		multi.add(_e);
	});

	while(1){
		try{
			multi.perform();
			int msgs_left = 0;
			// メッセージ取得ループ
			while(1){
				curl_base_multi_message mes;
				if(!multi.get_next_message(mes, msgs_left)) break;
				if(mes.get_code() != CURLE_OK) {
					std::cout << "CURL error code:" << mes.get_code() << std::endl;
					continue;
				}
				long httpcode = 0;
				mes.get_easy()->get_info(CURLINFO_RESPONSE_CODE, httpcode);
				if(httpcode == 200) {
					std::cout << "200 OK " << mes.get_easy()->get_url() << std::endl;
				} else {
					std::cout << "GET returned http status code " << httpcode << " " << mes.get_easy()->get_url() << std::endl;
				}
				// remove test
				// メッセージ完了してその都度Easyそのものを消す場合はremoveを呼ぶ（ただし、他から参照が残ってる場合は消えない）
				//  multi.remove(mes);
			}
			// 転送の残りが0になったら終わる
			if(multi.get_active_transfers() == 0) break;
		}catch (curl_base_exception &error){
			// エラー内容表示
			std::cerr << error.what() << std::endl;
			return -1;
		}
	}
	// ハンドルをすべて登録解除
	multi.clear();

	std::cerr << "end of program" << std::endl;
	return 0;
}
