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

// OpenAIのAPIを使用してChatGPTに質問するサンプル
// 事前にOpenAIにAPI使用の申請が必要です

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

// chatgpt の json messages配列の再現のためのクラス構造
class chatgpt_send_messages_json
{
	public:
		std::string role;
		std::string content;
};

// 使用の際はこれの定義が必要
static libcurlcxx::curl_base_cdtor _libcurl;

// OpenAIのAPIエンドポイント
static constexpr std::string_view chatgpt_endpoint("https://api.openai.com/v1/chat/completions");

#if 0
/*
 curl https://api.openai.com/v1/chat/completions \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer APIKEY" \
  -d '{"model": "gpt-3.5-turbo","messages": [{"role": "user", "content": "Hello!"}]}'
*/
#endif

bool getAPIKey(string &apikeystr)
{
	apikeystr.clear();

	// getenv関数を使用して、環境変数の値を取得
	const char* apikey = std::getenv("CHATGPT_USER_APIKEY");

	if (apikey == nullptr) {
		std::cout << "enviroment variable CHATGPT_USER_APIKEY isnot setting..." << std::endl;
		return false;
	}
	apikeystr = apikey;
	std::cout << "APIKEY: " << apikeystr << std::endl;
	return true;
}


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
static bool parseJson(const std::string &src, std::string &r_message)
{
	picojson::value jsonval;
	string json_err;

	r_message.clear();
	// json解析
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

//  std::cout << "json dump: "  << std::endl;
//  std::cout << jsonval.serialize(true)  << std::endl;

	// choices は配列になっている。ここでは最初の項目のみを対象とする。
	// 取れなかったら嫌なのでTryCatchで囲む
	try{
		// choices[0].message.content の中が返したメッセージである
		r_message = jsonval.get<picojson::object>()["choices"].get<picojson::array>()[0].get<picojson::object>()["message"].get<picojson::object>()["content"].to_str();
	}catch(std::runtime_error &e){
		std::cout << "[JSON] object not found? API miss ??? reason: " << e.what() << std::endl;
		std::cout << "can not find choice or message or content" << std::endl;
		return false;
	}catch(...){
		std::cout << "[JSON] object not found...API miss ??? " << std::endl;
		return false;
	}
	// 空かどうか確認
	if((r_message == "null") || r_message.empty()){
		std::cout << "[JSON] object not found. or message empty. API miss ??? " << std::endl;
		r_message.clear();
		return false;
	}

	return true;
}



// OpenAIのAPIはJsonでメッセージを送る必要があるので規定のJson文字列にする
// {"model": "gpt-3.5-turbo","messages": [{"role": "user", "content": "Hello!"}]}
// みたいなJsonデータを生成
// message: 問いたいメッセージを指定する
// ret:
//  json形式に直したものが出てくる。これをそのまま投げること
std::string create_message_json(std::string_view message)
{
	picojson_helper::writer _root;
	_root.add("model", "gpt-3.5-turbo-1106");		// 現在は1106

	std::vector<chatgpt_send_messages_json> vec;
	chatgpt_send_messages_json sh;
	sh.role = "user";
	sh.content = message;
	vec.push_back(sh);

	_root.add<chatgpt_send_messages_json>("messages", vec, [](chatgpt_send_messages_json _sh)
	{
		picojson_helper::writer j;
		j.add("role", _sh.role);
		j.add("content", _sh.content);
		return j;
	});

	return _root.serialize();
}

// OpenAIのAPIを使ってChatGPTに問いあわせメイン
// json形式で問い合わせ項目を作ってPOSTで投げると良い
int chat(const string &user_message)
{
	string api_kerstr;
	if(!getAPIKey(api_kerstr)) return -1;

	curl_http_request req(std::make_shared<curl_base_stringstream>());
	// ヘッダ作る。APIキーもヘッダに含める
	req.appendHeader("Content-Type: application/json");
	req.appendHeader(libcurlcxx::format("Authorization: Bearer %s", api_kerstr.data()));
	// 送信データはJsonで投げる
	req.RequestSetupPost(chatgpt_endpoint, create_message_json(user_message));

//  req.RequestSetupPost(chatgpt_endpoint, create_message_json("Hello!"));
//  req.RequestSetupPost(chatgpt_endpoint, create_message_json("こんにちは"));

	// 詳細なデバッグをしたい場合は以下のコメントを外す
	req.set_verbose(true);

	// いよいよOpenAIに投げる
	std::cout << "url: " << req.get_url() << std::endl;
	try {
		std::cout << "contacting openai..... " << std::endl;
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
	std::string message;
	// まずエラーがないかどうか見る
	if(parseErrorJson(req.get_ContentString(), message)){
		std::cout << "error responced" << std::endl;
		std::cout << "message:" << message << std::endl;
		return -1;
	}
	// なかったら通常処理
	if(!parseJson(req.get_ContentString(), message)){
		std::cout << "json perse error exiting... " << std::endl;
		return -1;
	}
	std::cout << "responce: " << std::endl;
	std::cout << message << std::endl;

	return 0;
}

// chatgpt の choice配列の再現のためのクラス構造
class chatgpt_choice
{
	public:
		int index = 0;
		std::string content;
};

// レスポンステスト用。いちいち問い合わせていたら課金されるので……
int jsontest()
{
	picojson_helper::writer _root;
	_root.add("id", "xxxxxxxxx");
	_root.add("object", "chat.completion");
	_root.add("created", 12345678);
	_root.add("model", "gpt-3.5-turbo-0301");

	picojson_helper::writer c_usage;
	c_usage.add("prompt_tokens", 10);
	c_usage.add("completion_tokens", 9);
	c_usage.add("total_tokens", 19);
	_root.add("usage", c_usage);

	std::vector<chatgpt_choice> vec;
	chatgpt_choice ch;
	ch.index = 0;
	ch.content = "Hello, how can I assist you today?";
	vec.push_back(ch);

	_root.add<chatgpt_choice>("choices", vec, [](chatgpt_choice _ch)
	{
		picojson_helper::writer j;
		j.add("finish_reason", "stop");
		j.add("index", _ch.index);

		picojson_helper::writer c_message;
		c_message.add("role", "assistant");
		c_message.add("content", _ch.content);
		j.add("message", c_message);
		return j;
	});

	std::string jstr = _root.serialize();
//  std::cout << jstr << std::endl;

	std::string message;
	if(!parseJson(jstr, message)){
		std::cout << "json perse error exiting... " << std::endl;
		return -1;
	}
	std::cout << "responce: " << std::endl;
	std::cout << message << std::endl;

	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	string message;
#if 1
	if (argc <= 1) {
		message = "hello!!";
	}else{
		message	= argv[1];
	}
	ret = chat(message);
#else
	ret = jsontest();		// TEST
#endif
	return ret;
}
