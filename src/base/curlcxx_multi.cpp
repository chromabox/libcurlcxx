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

#include "curlcxx_error.h"
#include "curlcxx_multi.h"

#include "classfname.h"

using libcurlcxx::curl_base_multi;
using libcurlcxx::curl_base_multi_message;
using libcurlcxx::curl_multi_unique_handle;
using libcurlcxx::curl_base_easy;
using libcurlcxx::curl_base_exception;

using std::unique_ptr;
using std::weak_ptr;
using std::shared_ptr;

// curl_base_multi_message: CURLMsg用のクラス

// コンストラクタ
curl_base_multi_message::curl_base_multi_message(const CURLMsg *msg, const std::weak_ptr<curl_base_easy> &p_easy)
{
	message = msg->msg;
	_easy	= p_easy;
	if(message == CURLMSG_DONE)	code = msg->data.result;
	else						code = CURLE_UNSUPPORTED_PROTOCOL;
}

// -----------------------------------------------------------------------
// curl_base_multi: curl_multi をC++で実装したもの
// 複数同時接続用のMultiハンドルを扱う
// Multiハンドルを生で使用せずスマートポインタで包み、可能な限り生ポインタを使わないことによって安全に使用することを念頭においている
//
// 複数の接続を同時に扱うことができ、この場合はeasyハンドルよりも処理が早い
// easyハンドルをまず生成してからaddで追加してperformをするとよい
//
// できるだけ簡易に扱えるようにはしているがやや特殊なため、test/http_multi_sample.cppの例を見ること
//
// see also:
// https://curl.se/libcurl/c/libcurl-multi.html
//

// コンストラクタ
curl_base_multi::curl_base_multi()
{
	active_transfers = 0;
	CURLM *p = curl_multi_init();
	if(p == nullptr){
		throw curl_base_exception("handle return null", __FCNAME, __LINE__);
	}
	// スマートポインタに登録。以後は直接使用せず、これ経由で使う。
	_multi.reset(p);
	// これだけはデフォルトでやっておく
	set_option(CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
}

curl_base_multi::~curl_base_multi() noexcept
{
	// multi_cleanupをしたあとでやってしまうとまずいため呼んでおく
	clear();
	// curl_multi_cleanupはここで呼ぶ必要はなし。
	// multiハンドル本体はスマートポインタで管理しているため。
	// (_curl_multi_handle_deleterを見ること)
}

// ムーブコンストラクタ
curl_base_multi::curl_base_multi(curl_base_multi &&other) noexcept
{
	_multi = std::move(other._multi);
	handles = std::move(other.handles);
	active_transfers = other.active_transfers;
}

// ムーブコンストラクタ
curl_base_multi &curl_base_multi::operator=(curl_base_multi&& other) noexcept
{
	if (this != &other) {
		_multi = std::move(other._multi);
		handles = std::move(other.handles);
		active_transfers = other.active_transfers;
	}
	return *this;
}

// エラーのセット。このクラスの中で使用
void curl_base_multi::set_error(const int curl_code) noexcept
{
	error_code = curl_code;
	error_str = curl_multi_strerror((CURLMcode)curl_code);
}

// EasyハンドルをMultiへ登録する
// multiでのperform対象になる
// 注意：addしたハンドルは個別にperformすることはできない
//       個別にperformしたいときはremoveした後ならできる
//
// easy: 登録したいEasyオブジェクトを指定する
void curl_base_multi::add(const std::shared_ptr<curl_base_easy> &easy)
{
	// 登録済みか見る。登録済みだったら駄目
	auto it = handles.find(easy->get_chandle());
	if (it != this->handles.end()){
		// 見つかってしまった。まずいので例外を投げる
		throw curl_base_exception("error: handle already registered", __FCNAME, __LINE__);
	}
	// 実際に登録
	const CURLMcode code = curl_multi_add_handle(_multi.get(), easy->get_chandle());
	if (code == CURLM_OK) {
		handles[easy->get_chandle()] = easy;
	} else {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
}

// 指定したEasyハンドルをMultiから登録解除する
// multiでのperform対象外になる
// addしたものの、後で個別にperformしたい場合はこれを呼んで登録を解除すること
// easyの参照が他にどこにもない場合はeasyそのものが消える可能性があるので注意
//
// easy: 消したいEasyオブジェクトを指定する
//
void curl_base_multi::remove(const std::shared_ptr<curl_base_easy> &easy)
{
	// 登録済みか見る。登録していないものは駄目
	auto it = handles.find(easy->get_chandle());
	if (it == this->handles.end()){
		// 未登録ハンドル。例外は投げない
		return;
	}
	// 登録解除処理
	const CURLMcode code = curl_multi_remove_handle(_multi.get(), it->first);
	if (code == CURLM_OK) {
		handles.erase(it->first);
	} else {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
}

// メッセージ型を渡せるRemove。メッセージを受けてもう処理しない場合とかに適用できる
// easyの参照が他にどこにもない場合は消える可能性があるので注意
//
// msg: get_next_message()で取得したメッセージを入れる。
//      対応したEasyオブジェクトの登録を解除する
void curl_base_multi::remove(const libcurlcxx::curl_base_multi_message &msg)
{
	// 登録済みか見る。登録していないものは駄目
	auto it = handles.find(msg.get_easy()->get_chandle());
	if (it == this->handles.end()){
		// 未登録ハンドル。例外は投げない
		return;
	}
	// 登録解除処理
	const CURLMcode code = curl_multi_remove_handle(_multi.get(), it->first);
	if (code == CURLM_OK) {
		handles.erase(it->first);
	} else {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
}

// 登録済みのEasyハンドルをMultiからすべて登録解除する
// 呼んだ段階でcurl_base_easyのオブジェクトがどこからも所有されていない場合は、この段階で適切に削除される
// なので、使用には注意すること
void curl_base_multi::clear()
{
	// ハンドルを順に探して登録解除
	std::for_each(handles.begin(), handles.end(), [this](const std::pair<CURL*, std::shared_ptr<curl_base_easy>> &p) {
		curl_multi_remove_handle(_multi.get(), p.second->get_chandle());
	});
	// マップも当然全消し
	handles.clear();
}


// perform後にperform結果をmessageとして取得する
// 使い方は難しいのでtest/multi_sample.cppの例を参照すること
//
// rmsg: 空のメッセージオブジェクトを設定。取得できたら結果を格納する
// msg_in_queue: 残メッセージキュー数が入る
// return:
//   true メッセージを取得した
//   false メッセージがないなどで取得できなかった
bool curl_base_multi::get_next_message(curl_base_multi_message &rmsg, int &msg_in_queue)
{
	CURLMsg *message = curl_multi_info_read(_multi.get(), &msg_in_queue);

	if (message == nullptr) return false;				// 取得できるメッセージは無い
	if (message->msg != CURLMSG_DONE) return false;		// 現状のCurlではCURLMSG_DONEしか意味がない

	// 登録してあるはずなので、そのEasyクラスのポインタを返してメッセージを作る
	auto it = handles.find(message->easy_handle);
	if (it == this->handles.end()) return false;				// CURLのEASYハンドルとEasyオブジェクト結びついてない場合はおかしいので何もせず

	rmsg = curl_base_multi_message(message, it->second);		// 対応したメッセージを作って返す
	return true;
}

// 取得処理の開始
// easyと異なり、この関数を実行してもすぐに帰ってくる
// 内部状況を更新するために、すべての受信が終わるまで定期的に呼ぶ必要がある
//
// return: false: すでに取得処理を実行中
//         true: 取得処理を開始した
bool curl_base_multi::perform()
{
	const CURLMcode code = curl_multi_perform(_multi.get(), &active_transfers);
	if (code == CURLM_CALL_MULTI_PERFORM) {
		// すでに読んでいるときはfalseを返す
		return false;
	}else if (code != CURLM_OK) {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
	return true;
}

// 指定時間待つ関数
void curl_base_multi::wait(struct curl_waitfd extra_fds[], const unsigned int extra_nfds, const int timeout_ms, int *numfds)
{
	const CURLMcode code = curl_multi_wait(_multi.get(), extra_fds, extra_nfds, timeout_ms, numfds);
	if (code != CURLM_OK) {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
}

// 指定時間待つ関数。waitと異なるのは別スレッドからwakeup()が呼ばれると復帰する
void curl_base_multi::poll(struct curl_waitfd extra_fds[], unsigned int extra_nfds, int timeout_ms, int *numfds)
{
	const CURLMcode code = curl_multi_poll(_multi.get(), extra_fds, extra_nfds, timeout_ms, numfds);
	if (code != CURLM_OK) {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
}

// pollで待っているときに他のスレッドから呼び出すとpoll状態から抜ける
// waitで待っている場合は全く影響を及ぼさないので注意
void curl_base_multi::wakeup()
{
	const CURLMcode code = curl_multi_wakeup(_multi.get());
	if (code != CURLM_OK) {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
}

// perform後のタイムアウト値を設定
void curl_base_multi::timeout(long *timeout)
{
	const CURLMcode code = curl_multi_timeout(_multi.get(), timeout);
	if (code != CURLM_OK) {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
}
