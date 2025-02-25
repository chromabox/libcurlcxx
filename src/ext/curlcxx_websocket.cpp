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


#include "curlcxx_websocket.h"
#include "curlcxx_mime.h"
#include "curlcxx_error.h"
#include "curlcxx_utility.h"

#include "classfname.h"

using libcurlcxx::curl_base_easy;
using libcurlcxx::curl_base_exception;
using libcurlcxx::curl_base_mime;
using libcurlcxx::curl_base_utility;

using libcurlcxx::curl_websocket;
using libcurlcxx::curl_http_request_param;

using std::string;
using std::ostringstream;

#define CURLCXX_WEBSOCK_BUFSIZE		(1024)

// curl_websocket : curl Easyを使用したWebSocketのC++実装
// easyを使用せずに大体これを使えばWebsocketは賄えるようにしている

// 使い方は以下の通り
//
// 1. curl_websocketのコンストラクタでオブジェクトを作成
// 2. URLと共に投げるデータがある場合はGETによってRequestSetupGetを使って設定
// 3. performで接続の実行。接続が成功or失敗したらひとまず帰ってくる
// 4. 結果のHTTPコードをget_responceCode()で受け取る
// 5. is_recvedで受信データがあるか確認。場合によっては5でループするようにすればいい
// 6. 結果をrecv_binaryなどで受け取る
//
// このクラスはHttpReqクラスとは異なっていて、performを呼び出して接続を開始した場合はPerformからすぐに帰ってくる
// performが終わっただけではデータは来ていないので、WebSocketからのデータを受け取る場合はrecv_binaryなどを使って受け取ること
//

// 現在はrecvのみ。sendには対応していない

// コンストラクタ
// ストリームを後から生成する場合やMultiの際などに使う
curl_websocket::curl_websocket() : curl_base_easy()
{
	isconnected = false;
	sendrecv_debug = false;
	internal_rbufsize = CURLCXX_WEBSOCK_BUFSIZE;
}

// デストラクタ
// 通信が接続中だった場合は切断も同時に行われる
curl_websocket::~curl_websocket() noexcept
{
	close();
}

// コンストラクタ。通常はこれを使用する
// url: 対象URLを指定。URLはws:// または wss:// で始めること
curl_websocket::curl_websocket(std::string_view url)
		: curl_base_easy()
{
	RequestSetupGet(url);
}

// コンストラクタ。こちらはGetパラメータを入れる場合
// url: 対象URLを指定。URLはws:// または wss:// で始めること
// params: 設定したいGetパラメータがある場合は指定する
curl_websocket::curl_websocket(std::string_view url, const curl_http_request_param& params)
		: curl_base_easy()
{
	RequestSetupGet(url, params);
}


// ムーブコンストラクタ
curl_websocket::curl_websocket(curl_websocket &&other) noexcept
{
	handle = std::move(other.handle);
	streamer = std::move(other.streamer);
	mime = std::move(other.mime);
	url = std::move(other.url);
	http_header = std::move(other.http_header);
	isconnected = std::move(other.isconnected);
	sendrecv_debug = std::move(other.sendrecv_debug);
	internal_rbufsize = std::move(other.internal_rbufsize);
}

curl_websocket& curl_websocket::operator= (curl_websocket &&other) noexcept
{
	if (this != &other) {
		handle = std::move(other.handle);
		streamer = std::move(other.streamer);
		mime = std::move(other.mime);
		url = std::move(other.url);
		http_header = std::move(other.http_header);
		isconnected = std::move(other.isconnected);
		sendrecv_debug = std::move(other.sendrecv_debug);
		internal_rbufsize = std::move(other.internal_rbufsize);
	}
	return *this;
}

// 接続の切断
// websocketは通常のHTTPリクエストとは異なりずっと接続し続けているため
// 接続を閉じたい場合はこれを実行するかそれともデストラクタまで待つこと
void curl_websocket::close()
{
	if(!isConnection()) return;
	size_t sent;
	curl_ws_send(handle.get(), "", 0, &sent, 0, CURLWS_CLOSE);
}

// HTTP用getパラメータでURLとともに設定する文字列をcurl_http_request_paramから構築して文字列型で返す
std::string curl_websocket::build_get_param(const curl_http_request_param& params)
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

// Performする際のHttpヘッダを設定する
// ヘッダの設定はprePerformかperformが呼び出される前に終わらせておくこと
// data: ヘッダ文字列
void curl_websocket::appendHeader(std::string_view data)
{
	http_header.append(data);
}

// appendHeaderでセットしたヘッダ設定をすべてなかったことにする
void curl_websocket::removeHeader()
{
	http_header.reset();
}


// URLをGetRequestで投げる準備をする
// URL: Get を投げるURL
// return:
//   true : 設定成功 performしても良い
//   false : 設定失敗 performしてはいけない
bool curl_websocket::RequestSetupGet(std::string_view url)
{
	set_option(CURLOPT_HTTPGET, 1L);
	return set_url(url);
}


// URLをGetRequestで投げる準備をする(パラメータつき)
// URLのあとの?は自動で設定される
//
// url: 対象URLを指定。URLはws:// または wss:// で始めること
// params: Getを投げる際のパラメータ。aaa=bbb&ccc=ddd...などのアレをcurl_http_request_paramで作っておくこと
// return:
//   true : 設定成功 performしても良い
//   false : 設定失敗 performしてはいけない
bool curl_websocket::RequestSetupGet(std::string_view url, const curl_http_request_param& params)
{
	std::string xurl(url);
	if(!params.empty()){
		xurl += '?';
		xurl += build_get_param(params);
	}
	return RequestSetupGet(xurl);
}


// Multi：Easyのperform前に呼び出す関数
// perform前の必要な設定を行う
// このクラスのperformを呼び出すときには特に必要がないが、multiを使用する場合はmulti.addする前に必要
void curl_websocket::prePerform()
{
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
	// このクラスではCURLOPT_CONNECT_ONLYを使用してPerformは接続だけにする。
	// これはPerformしたらすぐに戻ってくる
	// よってデータを受けたり投げたりrecvやsendを使うことになる
	set_option(CURLOPT_CONNECT_ONLY, 2L);
}

// 接続の開始
// WebSocketの場合はすぐに帰ってくる
// 注意：multiを使用するときはこれを使わないこと
//
// なにかエラーがでたら例外を投げるのでtry-catchで囲むこと
void curl_websocket::perform()
{
	// すでに接続が成立していた場合は閉じる
	if(isConnection()){
		close();
	}

	prePerform();
	// 投げる
	const CURLcode code = curl_easy_perform(handle.get());
	if (code != CURLE_OK) {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
	// HTTPコードを取得。101 Switching ProtocolsならWebsocketに切り替わったのでOK
	long httpcode = 0;
	get_info(CURLINFO_RESPONSE_CODE, httpcode);
	if(httpcode != 101){
		set_error(CURLE_UNSUPPORTED_PROTOCOL);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
	isconnected = true;
}


// メタ情報のみを受け取る。これは次に続くデータがTextかBinaryかの判断に必要
//
// istext: 受信したデータにテキストフラグが立っているかどうか
//    true = 受信したデータがテキストである。recv_textで受けられる
//    false = 受信したデータがバイナリである。recv_binaryで受けること推奨
// byteleft: 受取予定のデータサイズ
//
// return:
// CURLE_OK: なにかデータが来ている
// CURLE_AGAIN: データがなにも到達していない(エラーではない)。場合によって再度実行の必要あり
// CURLE_GOT_NOTHING: 切断された可能性があるので切断した
// その他: エラー
CURLcode curl_websocket::recv_meta(bool &istext, curl_off_t &byteleft)
{
	const struct curl_ws_frame *meta;
	size_t recv;

	istext = false;
	byteleft = 0;
	// 0とNULLを指定することによってmeta情報だけを受け取ることができる
	CURLcode res = curl_ws_recv(handle.get(), nullptr, 0, &recv, &meta);
	if(res == CURLE_AGAIN) return res;		// AGAINは特に何もしない(よく帰ってくる。METAもNULLである)

	if(res == CURLE_GOT_NOTHING){
		// 通信切れたっぽいので切断
		if(isDebug()){
			std::cout << "curl_websocket::recv_meta Got Noting!" << std::endl;
		}
		close();
		return res;
	}
	// CURLE_OKじゃないときはMETA取れない
	if(res != CURLE_OK){
		return res;
	}

	byteleft = meta->bytesleft;
	if(meta->flags & CURLWS_TEXT){
		istext = true;
	}else{
		istext = false;
	}
	return res;
}

// データをバイナリとみなして、std::vector<uint8_t>で受け取る
// TEXTかBINARYかどうかの判定は一切行っていないため、それが知りたい場合は前もってrecv_metaを実行すること
// rvecはこの中ではクリアしないので、続け様に受け取ることも可能
bool curl_websocket::recv_binary(std::vector<uint8_t> &rvec)
{
	std::vector<uint8_t> buffer(internal_rbufsize);

	while(1){
		const struct curl_ws_frame *meta;
		size_t recved;

		CURLcode cres = recv_raw(buffer, recved, &meta);
		// AGAIN時はブレークせずもう一度
		if(cres == CURLE_AGAIN) continue;

		// Appendを実行
		if(recved > 0){
			const auto oldsize = rvec.size();
			rvec.resize(oldsize + recved);
			std::copy_n(buffer.data(), recved, std::next(rvec.begin(), oldsize));
		}

		if(cres != CURLE_OK){
			// エラー
			return false;
		}
		// 次に続く受信がなければ完了
		if(meta->bytesleft == 0){
			break;
		}
	}
	return true;
}

// データをテキストとみなして、std::stringstreamで受け取る
// TEXTかBINARYかどうかの判定は一切行っていないため、それが知りたい場合は前もってrecv_metaを実行すること
bool curl_websocket::recv_text(std::stringstream &rtextstr)
{
	std::vector<char> buffer(internal_rbufsize);

	while(1){
		const struct curl_ws_frame *meta;
		size_t recved;

		CURLcode cres = recv_raw(buffer, recved, &meta);
		// AGAIN時はブレークせずもう一度
		if(cres == CURLE_AGAIN) continue;

		// Appendを実行
		if(recved > 0){
			rtextstr.write(buffer.data(), recved);
		}

		if(cres != CURLE_OK){
			// エラー
			return false;
		}
		// 次に続く受信がなければ完了
		if(meta->bytesleft == 0){
			break;
		}
	}
	return true;
}

// データをテキストとみなして、std::stringで受け取る
// TEXTかBINARYかどうかの判定は一切行っていないため、それが知りたい場合は前もってrecv_metaを実行すること
bool curl_websocket::recv_text(std::string &rtext)
{
	std::stringstream strm;

	rtext.clear();
	if(!recv_text(strm)) return false;
	rtext = strm.str();

	return true;
}

