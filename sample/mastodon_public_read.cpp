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

// mastodonなサーバのPublicタイムライン取得サンプル
// 認証の必要はない

#include <map>
#include <memory>
#include "curlcxx_cdtor.h"
#include "curlcxx_http_req.h"
#include "curlcxx_utility.h"
#include "curlcxx_error.h"

#include "picojson.h"

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

// mastodonのPublic timeline取得
// これを参照 https://github.com/mastodon/documentation/blob/fe555118d6925fb8b2aea92a55a8d37361d72b50/content/en/client/public.md

// mastodonのPublicタイムライン取得用エンドポイント
static constexpr std::string_view mastodon_fetch_public_endpoint("/api/v1/timelines/public");

// 接続したいサーバをここに書く
static constexpr std::string_view mastodon_server("https://mstdn.jp");
//  static constexpr std::string_view mastodon_server("https://pawoo.net");



// 通常メッセージのJSONの解析をしてJsonArrayを返す
static bool parseJson(const std::string &src, picojson::array &r_array)
{
	picojson::value jsonval;
	string json_err;

	r_array.clear();

	// json解析
	picojson::parse(jsonval, src.begin(), src.end(), &json_err);
	if(!json_err.empty()){
		std::cout << "[JSON] parse err!!! " << std::endl;
		std::cout << json_err << std::endl;
		std::cout << src << std::endl;
		return false;
	}

//  std::cout << "json dump: "  << std::endl;
//  std::cout << jsonval.serialize(true)  << std::endl;

	// mastodonはいきなりArrayであるはず
	if(!jsonval.is<picojson::array>()){
		std::cout << "[JSON] is not array... " << std::endl;
		return false;
	}
	// arrayであるはずなので取得
	try{
		r_array = jsonval.get<picojson::array>();
	}catch(std::runtime_error &e){
		std::cout << "[JSON] object not found? API miss ??? reason: " << e.what() << std::endl;
		std::cout << "can not find choice or message or content" << std::endl;
		return false;
	}catch(...){
		std::cout << "[JSON] object not found...API miss ??? " << std::endl;
		return false;
	}
	return true;
}


// Mastdonのサーバからタイムライン取得
// surl: 対象サーバ
// local: ローカル限定かどうか。
//    true  : ローカルタイムライン(LTL)取得
//    false : 連合タイムライン(GTL)取得
int mastodon_fetch_public(std::string_view surl, bool local)
{
	curl_http_request req(std::make_shared<curl_base_stringstream>());

	std::string url(surl);
	url += mastodon_fetch_public_endpoint;

	curl_http_request_param para;
	if(local){
		para["local"] = "true";
	}else{
		para["local"] = "false";
	}
//  para["limit"] = "2";				// limitをつけると取得数制限できる

	req.appendHeader("User-Agent: curl/7.81.0");		// UAを指定しないと403になる
	req.RequestSetupGet(url, para);

	// 詳細なデバッグをしたい場合は以下のコメントを外す
//  req.set_verbose(true);

	std::cout << "url: " << req.get_url() << std::endl;
	try {
		std::cout << "contacting mastodon..... " << std::endl;
		req.perform();
	} catch (curl_base_exception &error) {
		// 何らかのHTTPS的な失敗をした場合はここでエラー内容表示して終わり
		std::cerr << error.what() << std::endl;
		return -1;
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "http code error " << req.get_responceCode() << std::endl;
		std::cout << "raw message:" << std::endl;
		std::cout << req.get_ContentString() << std::endl;
		return -1;
	}

	if(req.get_ContentType().find("application/json") == std::string::npos){
		// JSONではない。何らかのエラーである
		std::cout << "mastodon request error : " << req.get_ContentString() << std::endl;
		return -1;
	}
//  std::cout << "content type: " << req.get_ContentType() << std::endl;
//  std::cout << "length: " << req.get_ContentLength() << std::endl;
//  std::cout << req.get_ContentString() << std::endl;

	// JSONでやってくるので解析
	picojson::array status_array;
	// まずJsonArrayをとってくる通常処理
	if(!parseJson(req.get_ContentString(), status_array)){
		std::cout << "json perse error exiting... " << std::endl;
		return -1;
	}
//  status_array[0].get<picojson::object>()["account"].get<picojson::object>()["display_name"].to_str();

	std::for_each(status_array.begin(), status_array.end(), [&](const picojson::value &pval) {
		std::string display_name;
		std::string acct_name;
		std::string content_message;
		try{
			picojson::object ob = pval.get<picojson::object>();
			display_name = ob["account"].get<picojson::object>()["display_name"].to_str();
			acct_name = ob["account"].get<picojson::object>()["acct"].to_str();
			content_message = ob["content"].to_str();
//  display_name = pval.get<picojson::object>()["a"].get<picojson::object>()["d"].to_str();
		}catch(std::runtime_error &e){
			std::cout << "[JSON] object not found? API miss ??? reason: " << e.what() << std::endl;
			std::cout << "can not find choice or message or content" << std::endl;
			return;
		}catch(...){
			std::cout << "[JSON] object not found...API miss ??? " << std::endl;
			return;
		}
		// displayname(@xxx.net): message の形とする
		std::cout << display_name << "(@" << acct_name << "): " << content_message << std::endl;
		std::cout << "---"<< std::endl;
	});

	return 0;
}


int main()
{
	int ret;
	ret = mastodon_fetch_public(mastodon_server, true);
	return ret;
}
