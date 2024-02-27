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

// misskeyのノート(タイムライン)取得サンプル

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
using std::ostream;
using std::ostringstream;
using std::map;

// 使用の際はこれの定義が必要
static libcurlcxx::curl_base_cdtor _libcurl;

// misskeyのノート取得用エンドポイント
static constexpr std::string_view misskey_fetch_note_local_endpoint("/api/notes/local-timeline");
static constexpr std::string_view misskey_fetch_note_global_endpoint("/api/notes/global-timeline");

// 接続したいサーバをここに書く
//  static constexpr std::string_view misskey_server("https://co.misskey.io");
static constexpr std::string_view misskey_server("https://misskey.io");


#if 0
/*
  curl https://xxx.misskey.io/api/notes/local-timeline -H "Content-Type: application/json" -d '{"limit": 20}'

*/
#endif

// エラーメッセージ用のJsonを解析する
static bool parseErrorJson(const std::string &src, std::string &r_message)
{
	picojson::value jsonval;
	string json_err;

	r_message.clear();
	// Json解析
	picojson::parse(jsonval, src.begin(), src.end(), &json_err);
	if(!json_err.empty()){
		std::cout << "[JSON] parse err!!! " << std::endl;
		std::cout << json_err << std::endl;
		std::cout << src << std::endl;
		return false;
	}
	if(!jsonval.is<picojson::object>()){
		std::cout << "[JSON] is not object... " << std::endl;
		return false;
	}

	// 取れなかったら嫌なのでTryCatchで囲む
	try{
		r_message = jsonval.get<picojson::object>()["error"].get<picojson::object>()["message"].to_str();
	}catch(std::runtime_error &e){
		return false;
	}catch(...){
		return false;
	}
	// 空かどうか確認
	if((r_message == "null") || r_message.empty()){
		r_message.clear();
		return false;
	}

	return true;
}

// 通常メッセージのJSONの解析をする
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

	// misskeyのnoteはいきなりArrayであるはず
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



// misskeyのAPIはJsonでメッセージを送る必要があるので規定のJson文字列にする
//
// {"i": "YOUR_API_TOKEN","limit": 2}
// みたいなJsonデータを生成
// message: 問いたいメッセージを指定する
// ret:
//  json形式に直したものが出てくる。これをそのまま投げること
std::string create_message_json(int limit)
{
	picojson_helper::writer _root;
	_root.add("limit", limit);
//  _root.add("i", "YOUR_API_KEY");			// Credential必要なエンドポイントの場合はこれが必要

	return _root.serialize();
}

// misskeyのサーバからnote取得
// surl: 対象サーバ
// local: ローカル限定かどうか。
//    true  : ローカルNote(LTL)取得
//    false : 連合Note(GTL)取得
int misskey_fetch_public(std::string_view surl, bool local)
{
	curl_http_request req(std::make_shared<curl_base_stringstream>());

	std::string url(surl);

	if(local)	url += misskey_fetch_note_local_endpoint;
	else		url += misskey_fetch_note_global_endpoint;

	// ヘッダ作る
	req.appendHeader("Content-Type: application/json");		// 設定必須
	req.appendHeader("User-Agent: curl/7.81.0");			// 一応UAを指定
	// misskeyはREST_APIではないらしい。Post＋Jsonで投げる
	req.RequestSetupPost(url, create_message_json(20));

	// 詳細なデバッグをしたい場合は以下のコメントを外す
	req.set_verbose(false);

	std::cout << "url: " << req.get_url() << std::endl;
	try {
		std::cout << "contacting misskey..... " << std::endl;
		req.perform();
	} catch (curl_base_exception &error) {
		// 何らかのHTTPS的な失敗をした場合はここでエラー内容表示して終わり
		std::cerr << error.what() << std::endl;
		return -1;
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "http code error " << req.get_responceCode() << std::endl;
		if(req.get_ContentType().find("application/json") == std::string::npos){
			// jsonがContentTypeにない場合はそのまま出す
			std::cout << "raw message:" << std::endl;
			std::cout << req.get_ContentString() << std::endl;
		}else{
			// jsonだ
			std::string err_message;
			if(parseErrorJson(req.get_ContentString(), err_message)){
				std::cout << "error message:" << std::endl;
				std::cout << err_message << std::endl;
			}else{
				std::cout << "raw message:" << std::endl;
				std::cout << req.get_ContentString() << std::endl;
			}
		}
		return -1;
	}
	if(req.get_ContentType().find("application/json") == std::string::npos){
		// JSONではない。何らかのエラーである
		std::cout << "openai request error : " << req.get_ContentString() << std::endl;
		return -1;
	}
//  std::cout << "content type: " << req.get_ContentType() << std::endl;
//  std::cout << "length: " << req.get_ContentLength() << std::endl;
//  std::cout << req.get_ContentString() << std::endl;

	// JSONでやってくるので解析
	picojson::array note_array;
	if(!parseJson(req.get_ContentString(), note_array)){
		std::cout << "json perse error exiting... " << std::endl;
		return -1;
	}
	// ノートが配列になって入ってるので一件つづ表示
	std::for_each(note_array.begin(), note_array.end(), [&](const picojson::value &pval) {
		std::string display_name;
		std::string acct_name;
		std::string note_text;
		bool renoted = false;
		try{
			picojson::object ob = pval.get<picojson::object>();
			display_name = ob["user"].get<picojson::object>()["name"].to_str();
			acct_name = ob["user"].get<picojson::object>()["username"].to_str();
			if(ob["renote"].is<picojson::object>()){
				note_text = ob["renote"].get<picojson::object>()["text"].to_str();
				renoted = true;
			}else{
				note_text = ob["text"].to_str();
			}
		}catch(std::runtime_error &e){
			std::cout << "[JSON] object not found? API miss ??? reason: " << e.what() << std::endl;
			std::cout << "can not find choice or message or content" << std::endl;
			return;
		}catch(...){
			std::cout << "[JSON] object not found...API miss ??? " << std::endl;
			return;
		}
		if(renoted){
			// [renote] displayname(@xxx): message の形とする
			std::cout << "[renote] " << display_name << "(@" << acct_name << "): " << note_text << std::endl;
		}else{
			// displayname(@xxx): message の形とする
			std::cout << display_name << "(@" << acct_name << "): " << note_text << std::endl;
		}
		std::cout << "---"<< std::endl;
	});
	return 0;
}

int main()
{
	int ret;
	ret = misskey_fetch_public(misskey_server, true);
	return ret;
}
