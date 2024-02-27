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


#include "curlcxx_http_req.h"
#include "curlcxx_mime.h"
#include "curlcxx_error.h"
#include "curlcxx_utility.h"

#include "classfname.h"

using libcurlcxx::curl_base_easy;
using libcurlcxx::curl_base_exception;
using libcurlcxx::curl_base_mime;
using libcurlcxx::curl_base_utility;

using libcurlcxx::curl_http_request;
using libcurlcxx::curl_http_request_param;

using std::string;
using std::ostringstream;

// curl_http_request : curl Easyを使用したHTTPリクエストのC++実装
// easyを使用せずに大体これを使えばHTTP(s)は賄えるようにしている

// 使い方は以下の通り
//
// 1. curl_http_requestのコンストラクタでstd::make_shared<T>()を設定してオブジェクトとストリームを作成
// 2. URLと共に投げるデータがある場合は、POSTかGETかによって、RequestSetupPostかRequestSetupGetを使って設定
// 3. performで通信実行
// 4. 結果のHTTPコードをget_responceCode()で受け取る
// 5. ストリームがcurl_base_stringstreamなどのString系ならget_ContentString()で受信した文字列を受け取る
//
// このクラスのperformを呼び出して通信開始した場合は、基本的に通信が終わるまでは帰ってこない
// 通信途中でもいいから受け取ったデータで何かをしたい場合はcurl_base_stream_objectを派生させたクラスを作ること
//

// コンストラクタ
// ストリームを後から生成する場合やMultiの際などに使う
curl_http_request::curl_http_request()
{
	proxyport = 0;
}

// デストラクタ
curl_http_request::~curl_http_request()
{}

// コンストラクタ。通常はこれを使用する
// streamer: curl_base_stream_objectを派生したオブジェクトのポインタ
//           データはこのクラスに貯められる
//           なるべくこのクラスが破棄されるまでstreamerは破棄しないことを推奨
curl_http_request::curl_http_request(const std::shared_ptr<curl_base_stream_object> &_streamer)
		: curl_base_easy(_streamer)
{
	proxyport = 0;
}

// ムーブコンストラクタ
curl_http_request::curl_http_request(curl_http_request &&other) noexcept
{
	handle = std::move(other.handle);
	streamer = std::move(other.streamer);
	mime = std::move(other.mime);
	url = std::move(other.url);
	proxyurl = std::move(other.proxyurl);
	proxyuser = std::move(other.proxyuser);
	proxypass = std::move(other.proxypass);
	proxyport = other.proxyport;
	http_header = std::move(other.http_header);
}

// ムーブ代入演算子
curl_http_request & curl_http_request::operator= (curl_http_request &&other) noexcept
{
	if (this != &other) {
		handle = std::move(other.handle);
		streamer = std::move(other.streamer);
		mime = std::move(other.mime);
		url = std::move(other.url);
		proxyurl = std::move(other.proxyurl);
		proxyuser = std::move(other.proxyuser);
		proxypass = std::move(other.proxypass);
		proxyport = other.proxyport;
		http_header = std::move(other.http_header);
	}
	return *this;
}


// Proxyを実際にセットする。内部用。
void curl_http_request::setInternalProxy()
{
	// とりあえずクリア
	clear_option(CURLOPT_PROXY);
	clear_option(CURLOPT_PROXYUSERPWD);
	set_option(CURLOPT_PROXYAUTH, static_cast<long>(CURLAUTH_ANY));

	// 実際にcURLにセット
	if(!proxyurl.empty())		set_option(CURLOPT_PROXY, proxyurl);
	if(proxyport != 0)			set_option(CURLOPT_PROXYPORT, proxyport);
	if(!proxyuser.empty())		set_option(CURLOPT_PROXYUSERNAME, proxyuser);
	if(!proxypass.empty())		set_option(CURLOPT_PROXYPASSWORD, proxypass);
}

// HTTP用getパラメータでURLとともに設定する文字列をcurl_http_request_paramから構築して文字列型で返す
std::string curl_http_request::build_get_param(const curl_http_request_param& params)
{
    if (params.empty()) return "";

    std::ostringstream oss;
	// URLエンコードをしながら=&が使われる型に変換する
    for (auto p = params.begin(); p != params.end(); ++p) {
        oss << curl_base_utility::url_escape(p->first) << "=" << curl_base_utility::url_escape(p->second);
        if (std::next(p) != params.end()) {
            oss << "&";
        }
    }
    return oss.str();
}

// HTTP用POSTパラメータでPOSTするときに投げるMIME情報をcurl_http_request_paramから構築し内部的にセットする
// 注意：前に設定してあったMime情報は削除される
bool curl_http_request::build_post_param(const curl_http_request_param& params)
{
    if (params.empty()) return false;

	auto src_mime = std::make_shared<curl_base_mime>(*this);

	// POSTはURLEscape不要
    for (auto p = params.begin(); p != params.end(); ++p) {
		src_mime->add_part(p->first, p->second);
    }
	// 所有権移動セット
	return move_set_mime(src_mime);
}


// Performする際のHttpヘッダを設定する
// ヘッダの設定はprePerformかperformが呼び出される前に終わらせておくこと
// data: ヘッダ文字列
void curl_http_request::appendHeader(std::string_view data)
{
	http_header.append(data);
}

// appendHeaderでセットしたヘッダ設定をすべてなかったことにする
void curl_http_request::removeHeader()
{
	http_header.reset();
}


// Multi：Easyのperform前に呼び出す関数
// perform前の必要な設定を行う
// このクラスのperformを呼び出すときには特に必要がないが、multiを使用する場合はmulti.addする前に必要
void curl_http_request::prePerform()
{
	// ProxYセット
	setInternalProxy();

	// HTTP 2 over TLS (HTTPS) のみを試し、ダメな場合はHTTP1.1で通信
	// こうしないと多重化の恩恵が受けられない
	set_option(CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
	// ロケーションはデフォルトでCurlに転送してもらう
	set_option(CURLOPT_FOLLOWLOCATION, 1L);

#if (CURLPIPE_MULTIPLEX > 0)
	// 多重化を待ち、確実なものにするために1にする
	set_option(CURLOPT_PIPEWAIT, 1);
#endif
	// ACCEPT_ENCODINGの設定
	// ここはむやみに設定すると403を返される場合がある
#if 0
	std::string_view _empty("");
	set_option(CURLOPT_ACCEPT_ENCODING, _empty.data());
#endif
	// ヘッダを実際にセット
	if(!http_header.isempty()){
		set_option(CURLOPT_HTTPHEADER, http_header.get_slistptr());
	}
}

// URLをGetRequestで投げる準備をする
// URL: Get を投げるURL
// return:
//   true : 設定成功 performしても良い
//   false : 設定失敗 performしてはいけない
bool curl_http_request::RequestSetupGet(std::string_view url)
{
	set_option(CURLOPT_HTTPGET, 1L);
	return set_url(url);
}


// URLをGetRequestで投げる準備をする(パラメータつき)
// URLのあとの?は自動で設定される
//
// URL: Get を投げるURL
// params: Getを投げる際のパラメータ。aaa=bbb&ccc=ddd...などのアレをcurl_http_request_paramで作っておくこと
// return:
//   true : 設定成功 performしても良い
//   false : 設定失敗 performしてはいけない
bool curl_http_request::RequestSetupGet(std::string_view url, const curl_http_request_param& params)
{
	std::string xurl(url);
	if(!params.empty()){
		xurl += '?';
		xurl += build_get_param(params);
	}
	return RequestSetupGet(xurl);
}


// URLをPostRequestで投げる準備をする
// 文字列以外も投げる必要がある場合はこれ
//
// URL: Post を投げるURL
// mimes: Postを投げる際のmime情報。
// return:
//   true : 設定成功 performしても良い
//   false : 設定失敗 performしてはいけない
bool curl_http_request::RequestSetupPost(std::string_view url, const std::shared_ptr<curl_base_mime> &mimes)
{
	if(!set_url(url)) return false;
	return set_mime(mimes);
}

// URLをPostRequestで投げる準備をする
// 内容が文字列で複数要素がある場合に使うと良い
//
// URL: Post を投げるURL
// params: Postを投げる際のパラメータ。複数の要素がある文字列を送ることが出来る
// return:
//   true : 設定成功 performしても良い
//   false : 設定失敗 performしてはいけない
bool curl_http_request::RequestSetupPost(std::string_view url, const curl_http_request_param& params)
{
	if(!set_url(url)) return false;
	return build_post_param(params);
}

// URLをPostRequestで投げる準備をする
// 一つしか文字列のパラメータしかない場合や、JSONをそのまま投げる場合に使うと楽
//
// URL: Post を投げるURL
// strdata: Postを投げる際のパラメータ。文字列。これはこのクラスでは何もせずそのままCurlに渡される
// return:
//   true : 設定成功 performしても良い
//   false : 設定失敗 performしてはいけない
bool curl_http_request::RequestSetupPost(std::string_view url, std::string_view strdata)
{
	if(!set_url(url)) return false;
	// 文字列はlibcurl内部へコピーするようにする
	set_option(CURLOPT_COPYPOSTFIELDS, strdata);
	return true;
}

// ファイル転送を開始
// 転送が終わるまで帰ってこない。ブロッキングする
// 注意：multiを使用するときはこれを使わないこと
//
// なにかエラーがでたら例外を投げるのでtry-catchで囲むこと
void curl_http_request::perform()
{
	prePerform();
	curl_base_easy::perform();
}
