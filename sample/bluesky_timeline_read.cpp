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

// blueskyのタイムライン取得サンプル
//
// タイムライン取得だけでも認証必須なので、実行する前に事前にアプリパスワードが必要
// (アプリパスワードはBlueskyアプリの「設定」ー「アプリパスワード」ー「アプリパスワードを追加」で取得可)
// 取得したアプリパスワードを環境変数「BLUESKY_APP_PASSWD」に設定すること
//
// blueskyではPOSTとGET、おまけにJsonで作ったリクエストを投げる必要もあるため
// 本当は curl_http_request をオーバライドしたほうが野暮ったくなくて良いですが
// 簡易的なサンプルであるので、curl_http_requestをそのまま使っています
//

#include <map>
#include <memory>
#include "curlcxx_cdtor.h"
#include "curlcxx_http_req.h"
#include "curlcxx_utility.h"
#include "curlcxx_error.h"

#include "picojson.h"
#include "picojson_writer.h"

using libcurlcxx::curl_base_cdtor;
using libcurlcxx::curl_base_stringstream;
using libcurlcxx::curl_base_utility;
using libcurlcxx::curl_base_exception;
using libcurlcxx::curl_base_mime;

using libcurlcxx::curl_http_request;
using libcurlcxx::curl_http_request_param;

using std::string;
using std::string_view;
using std::ostream;
using std::ostringstream;
using std::map;

// 使用の際はこれの定義が必要
static libcurlcxx::curl_base_cdtor _libcurl;

// エンドポイント
static constexpr string_view bluesky_resolve_handle_endp("com.atproto.identity.resolveHandle");
static constexpr string_view bluesky_create_session_endp("com.atproto.server.createSession");
static constexpr string_view bluesky_feed_get_timeline_endp("app.bsky.feed.getTimeline");

// Blueskyサーバ
static constexpr string_view bluesky_server("https://bsky.social/xrpc/");


// Blueskyのサーバにユーザハンドル名からdidを問い合わせる
// 空が帰ってきたら失敗
static std::string resolveHandletoDid(string_view server_url, string_view user_handle)
{
	curl_http_request req(std::make_shared<curl_base_stringstream>());
	curl_http_request_param para;
	std::string url(server_url);

	url += bluesky_resolve_handle_endp;
	// ヘッダ作る
	req.appendHeader("Content-Type: application/json");		// 一応設定
	req.appendHeader("User-Agent: curl/7.81.0");			// 一応UAを指定
	// getで投げる要素の設定
	para["handle"] = user_handle;
	req.RequestSetupGet(url, para);

//  std::cout << "url: " << req.get_url() << std::endl;
	try {
		std::cout << "resolve handle to bluesky..... " << std::endl;
		req.perform();
	} catch (curl_base_exception &error) {
		// 何らかのHTTPS的な失敗をした場合はここでエラー内容表示して終わり
		std::cerr << error.what() << std::endl;
		return "";
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "http code error " << req.get_responceCode() << std::endl;
		std::cout << "raw message:" << std::endl;
		std::cout << req.get_ContentString() << std::endl;
		return "";
	}

	if(req.get_ContentType().find("application/json") == std::string::npos){
		// JSONではない。何らかのエラーである
		std::cout << "request error : " << req.get_ContentString() << std::endl;
		return "";
	}

	picojson::value jsonval;
	string json_err;

	// Json解析
	json_err = picojson::parse(jsonval, req.get_ContentString());
	if(!json_err.empty()){
		std::cout << "[JSON] parse err!!! " << std::endl;
		std::cout << json_err << std::endl;
		return "";
	}
	if(!jsonval.is<picojson::object>()){
		std::cout << "[JSON] is not object... " << std::endl;
		return "";
	}

	std::string did;
	// 取れなかったら嫌なのでTryCatchで囲む
	try{
		did = jsonval.get<picojson::object>()["did"].to_str();
	}catch(...){
		std::cout << "[JSON] cannot find did entry... " << std::endl;
		return did;
	}
	// 空かどうか確認
	if(did == "null"){
		did.clear();
	}
	return did;
}

// セッションの作成
// blueskyでは何をするにも認証が必要なのでこれをする必要がある
// 成功したらaccessJwtを返す
static std::string createSession(string_view server_url, string_view did, string_view ap_pass)
{
	curl_http_request req(std::make_shared<curl_base_stringstream>());
	std::string url(server_url);

	url += bluesky_create_session_endp;
	// ヘッダ作る
	req.appendHeader("Content-Type: application/json");		// 一応設定
	req.appendHeader("User-Agent: curl/7.81.0");			// 一応UAを指定

	// postで投げる要素の設定(JSONでないといけない)
	picojson_helper::writer _root;
	string poststr;
	_root.add("identifier", did);
	_root.add("password", ap_pass);
	poststr = _root.serialize();

	req.RequestSetupPost(url, poststr);

//  std::cout << "url: " << req.get_url() << std::endl;
	try {
		std::cout << "create session to bluesky..... " << std::endl;
		req.perform();
	} catch (curl_base_exception &error) {
		// 何らかのHTTPS的な失敗をした場合はここでエラー内容表示して終わり
		std::cerr << error.what() << std::endl;
		return "";
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "http code error " << req.get_responceCode() << std::endl;
		std::cout << "raw message:" << std::endl;
		std::cout << req.get_ContentString() << std::endl;
		return "";
	}

	if(req.get_ContentType().find("application/json") == std::string::npos){
		// JSONではない。何らかのエラーである
		std::cout << "request error : " << req.get_ContentString() << std::endl;
		return "";
	}

	picojson::value jsonval;
	string json_err;

	// Json解析
	json_err = picojson::parse(jsonval, req.get_ContentString());
	if(!json_err.empty()){
		std::cout << "[JSON] parse err!!! " << std::endl;
		std::cout << json_err << std::endl;
		return "";
	}
	if(!jsonval.is<picojson::object>()){
		std::cout << "[JSON] is not object... " << std::endl;
		return "";
	}

	// 取れなかったら嫌なのでTryCatchで囲む
	std::string jwt;
	try{
		jwt = jsonval.get<picojson::object>()["accessJwt"].to_str();
	}catch(...){
		std::cout << "[JSON] cannot find accessJwt entry... " << std::endl;
		return jwt;
	}
	// 空かどうか確認
	if(jwt == "null"){
		jwt.clear();
	}
	return jwt;
}

// タイムラインの取得。Jsonの解析まではしない
// 取得失敗したら空文字を返す
static std::string getTimeline(string_view server_url, string_view bearer, int limits)
{
	curl_http_request req(std::make_shared<curl_base_stringstream>());
	curl_http_request_param para;
	std::string url(server_url);

	url += bluesky_feed_get_timeline_endp;
	// ヘッダ作る
	req.appendHeader("Content-Type: application/json");		// 一応設定
	req.appendHeader("User-Agent: curl/7.81.0");			// 一応UAを指定
	req.appendHeader(libcurlcxx::format("Authorization: Bearer %s", bearer.data()));			// Bearer Token

	para["limit"] = libcurlcxx::format("%d", limits);

	req.RequestSetupGet(url, para);

//  std::cout << "url: " << req.get_url() << std::endl;
	try {
		std::cout << "get timeline to bluesky..... " << std::endl;
		req.perform();
	} catch (curl_base_exception &error) {
		// 何らかのHTTPS的な失敗をした場合はここでエラー内容表示して終わり
		std::cerr << error.what() << std::endl;
		return "";
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "http code error " << req.get_responceCode() << std::endl;
		std::cout << "raw message:" << std::endl;
		std::cout << req.get_ContentString() << std::endl;
		return "";
	}

	if(req.get_ContentType().find("application/json") == std::string::npos){
		// JSONではない。何らかのエラーである
		std::cout << "request error : " << req.get_ContentString() << std::endl;
		return "";
	}
	return req.get_ContentString();
}



int bluesky_main(string_view server_url, string_view user_handle, string_view ap_pass)
{
	// まずは何をするにもdidが必要なのでdidを解決する
	string did = resolveHandletoDid(server_url, user_handle);
	if(did.empty()){
		std::cout << "error: can not get user did... " << std::endl;
		return -1;
	}
//  std::cout << "user did: " << did << std::endl;

	// セッションキー(bearerToken)を取得
	string bearer = createSession(server_url, did, ap_pass);
	if(bearer.empty()){
		std::cout << "error: can not get user bearer... " << std::endl;
		return -1;
	}
	// JWTは出さないほうが良い
//  std::cout << "session accessJwt: " << bearer << std::endl;

	// ようやっとタイムラインを得ることが出来る
	string timelinestr = getTimeline(server_url, bearer, 5);
	if(timelinestr.empty()){
		std::cout << "error: can not get user timeline... " << std::endl;
		return -1;
	}
	std::cout << "timeline: " << std::endl;

	picojson::value jsonval;
	string json_err;
	std::string jwt;

	// Json解析
	json_err = picojson::parse(jsonval, timelinestr);
	if(!json_err.empty()){
		std::cout << "[JSON] parse err!!! " << std::endl;
		std::cout << json_err << std::endl;
		return -1;
	}
	if(!jsonval.is<picojson::object>()){
		std::cout << "[JSON] is not object... " << std::endl;
		return -1;
	}
	// そのまま整形したJsonを吐き出します
	std::cout <<  jsonval.serialize(true) << std::endl;
	return 0;
}

// 環境変数BLUESKY_APP_PASSWDから設定したアプリパスを得る
bool getAppPasswd(string &ap_pass)
{
	ap_pass.clear();
	// getenv関数を使用して、環境変数の値を取得
	const char* ap_char = std::getenv("BLUESKY_APP_PASSWD");

	if (ap_char == nullptr) {
		std::cout << "enviroment variable BLUESKY_APP_PASSWD isnot setting..." << std::endl;
		return false;
	}
	ap_pass = ap_char;
#if 0
	std::cout << "APIKEY: " << apikeystr << std::endl;
#endif
	return true;
}



int main(int argc, char *argv[])
{
	int ret;
	string ap_pass;
	string user_handle;
	string blsk_server(bluesky_server);

	if (argc <= 1) {
		std::cout << "username isnot setting. ./bluesky_timeline_read yourhandlename" << std::endl;
		return -1;
	}

	if(!getAppPasswd(ap_pass))		return -1;

	user_handle = argv[1];
	ret = bluesky_main(blsk_server, user_handle, ap_pass);
	return ret;
}
