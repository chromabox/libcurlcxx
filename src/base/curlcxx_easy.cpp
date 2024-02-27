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


#include "curlcxx_easy.h"
#include "curlcxx_mime.h"
#include "curlcxx_error.h"

#include "classfname.h"

using libcurlcxx::curl_base_easy;
using libcurlcxx::curl_base_exception;
using libcurlcxx::curl_easy_unique_handle;
using libcurlcxx::curl_base_mime;

using std::string;


// curl_base_easy : curlのEasyハンドラのC++実装
// Easyハンドルを生で使用せずスマートポインタで包み、可能な限り生ポインタを使わないことによって安全に使用することを念頭においている

// コンストラクタ。
// ストリームを後から生成する場合やMultiの際などに使う
curl_base_easy::curl_base_easy()
{
	CURL *p = curl_easy_init();
	if(p == nullptr){
		throw curl_base_exception("handle return null", __FCNAME, __LINE__);
	}
	handle.reset(p);
	prog_callbk = nullptr;
	prog_data = nullptr;
}

// デストラクタ
curl_base_easy::~curl_base_easy()
{}

// コンストラクタ。通常はこれを使用する
// streamer: curl_base_stream_objectを派生したオブジェクトのポインタ
//           データはこのクラスに貯められる
//           なるべくこのクラスが破棄されるまでstreamerは破棄しないことを推奨
//           std::make_shared<>したものを入れること
curl_base_easy::curl_base_easy(const std::shared_ptr<curl_base_stream_object> &_streamer)
{
	CURL *p = curl_easy_init();
	if(p == nullptr){
		throw curl_base_exception("handle return null", __FCNAME, __LINE__);
	}
	handle.reset(p);
	set_streamer(_streamer);
	prog_callbk = nullptr;
	prog_data = nullptr;
}

// ムーブコンストラクタ
curl_base_easy::curl_base_easy(curl_base_easy &&other) noexcept
{
	handle = std::move(other.handle);
	streamer = std::move(other.streamer);
	mime = std::move(other.mime);
	url = std::move(other.url);
	prog_callbk = other.prog_callbk;
	prog_data = other.prog_data;
}

// ムーブ代入演算子
curl_base_easy & curl_base_easy::operator= (curl_base_easy &&other) noexcept
{
	if (this != &other) {
		handle = std::move(other.handle);
		streamer = std::move(other.streamer);
		mime = std::move(other.mime);
		url = std::move(other.url);
		prog_callbk = other.prog_callbk;
		prog_data = other.prog_data;
	}
	return *this;
}

// 同じオブジェクトか？
// ハンドルが同じかどうかで判定
bool curl_base_easy::operator==(const curl_base_easy &easy) const
{
	return handle == easy.handle;
}

// ストリームを指定したものに設定
// perform中にやるとどうなるかは不定
// streamer: curl_base_stream_objectを派生したオブジェクトのポインタ
//           データはこのクラスに貯められる
//           なるべくこのクラスが破棄されるまでstreamerは破棄しないことを推奨
void curl_base_easy::set_streamer(const std::shared_ptr<curl_base_stream_object> &_streamer)
{
	streamer = _streamer;
	set_write_callback(streamer->get_write_function());
	set_option(CURLOPT_WRITEDATA, streamer.get());
}

// エラーのセット。このクラスの中及び派生クラスでのみ使用
// curl_code: Curlのエラーコード
void curl_base_easy::set_error(const int curl_code) noexcept
{
	error_code = curl_code;
	error_str = curl_easy_strerror((CURLcode)curl_code);
}

// URLをセットする
// 構文チェックはしないので間違っていてもたいていTrueを返す
bool curl_base_easy::set_url(std::string_view _url)
{
	// 大抵はCURLE_OKを返す
	if(set_option(CURLOPT_URL, _url.data()) != CURLE_OK) return false;
	url = _url;
	return true;
}

// mimeをセットしてPOSTとする
// FormPostでPerformするときもこれを使う必要がある
bool curl_base_easy::set_mime(const std::shared_ptr<curl_base_mime> &_mime)
{
	mime = _mime;
	if(set_option(CURLOPT_MIMEPOST, mime->get_mimeptr()) != CURLE_OK) return false;
	return true;
}

// 派生クラス用
// mimeをセットしてPOSTとする。元の_mimeは所有権を移動され空になるので注意
// FormPostでPerformするときもこれを使う必要がある
bool curl_base_easy::move_set_mime(std::shared_ptr<curl_base_mime> &_mime)
{
	mime = move(_mime);		// 所有権を移動する
	if(set_option(CURLOPT_MIMEPOST, mime->get_mimeptr()) != CURLE_OK) return false;
	return true;
}

// libcurlから呼ばれるXferのエントリポイント
int curl_base_easy::_xfer_callback_func(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	if(clientp == nullptr)	return 0;		// clientpがない場合は何事もなかったかのように0を返さなければならない

	return static_cast<curl_base_easy*>(clientp)->internal_xfer_callback(dltotal, dlnow, ultotal, ulnow);
}

// 転送状況が変わるたびに呼び出される
// これを派生先でオーバライドすることもできる
// dltotal: 予想される合計ダウンロードバイト数
// dlnow: これまでにダウンロードしたバイト数
// ultotal: 予想されるアップロードバイト数
// ulnow: これまでにアップロードしたバイト数
// return:
//   0 = 処理継続(普通はこれを返す)
//   CURL_PROGRESSFUNC_CONTINUE = libcurlが持つプログレスメータに処理を移譲する
//   それ以外 = ダウンロード・アップロード処理の中断
int curl_base_easy::internal_xfer_callback(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	// コールバック関数無指定ではここではなにもすることはないので何事もなかったかのように0を返す
	if(prog_callbk == nullptr){
#if 0
		// debug用
		std::cout << "xfer: url " << get_url() << " dltotal " << dltotal <<
					" dlnow " << dlnow << " ultotal " << ultotal << " ulnow " << ulnow << std::endl;
#endif
		return 0;
	}
	return prog_callbk(this, prog_data, dltotal, dlnow, ultotal, ulnow);
}

// プログレス表示用の関数とデータをセットする
// onoff: プログレス機能を使用するかどうか true=使用する false=使用しない
// func:  転送状況が更新されたときに呼び出すユーザ定義関数を指定。nulの場合は内部関数が呼ばれるが何もせず
//        (派生クラスを使うときはinternal_xfer_callbackをオーバライドするだけでいいのでNullを入れると良い)
// param: 転送状況が更新されたときに呼び出すユーザ定義関数に渡すデータ
//        (派生クラスを使うときは自クラスを参照できるはずなのでNullでも良い)
void curl_base_easy::set_progress_function(bool onoff, curl_base_easy_progress_callback func, void *param)
{
	if(!onoff){
		set_option(CURLOPT_NOPROGRESS, 1L);			// プログレスは処理せずにする
		return;
	}
	prog_callbk = func;
	prog_data = param;

	set_option(CURLOPT_XFERINFODATA, this);
	set_xferinfo_callback(_xfer_callback_func);		// まずは内蔵のエントリポイントを登録する。そこから呼び出すため
	set_option(CURLOPT_NOPROGRESS, 0L);				// これをする必要がある
}


// set_debugdumpをTrueにして、performを行い、通信が発生するたびに呼び出されるデバッグ用のコールバック関数
int curl_base_easy::_debug_callback_func(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr)
{
	if(userptr == nullptr) return 0;
	return static_cast<curl_base_easy*>(userptr)->internal_debug_callback(handle, type, data, size);
}

// set_debugdumpをTrueにした場合のデフォルトのコールバック関数
int curl_base_easy::internal_debug_callback(CURL *handle, curl_infotype type, char *data, size_t size)
{
	const char *text;
	(void)handle;		// warning避け

	switch(type)
	{
		case CURLINFO_TEXT:
			text = "TEXT";
			break;
		case CURLINFO_HEADER_IN:
			text = "<= Recv header";
			break;
		case CURLINFO_HEADER_OUT:
			text = "=> Send header";
			break;
		case CURLINFO_DATA_IN:
			text = "<= Recv data";
			break;
		case CURLINFO_DATA_OUT:
			text = "=> Send data";
			break;
		case CURLINFO_SSL_DATA_IN:
			text = "<= Recv SSL data";
			break;
		case CURLINFO_SSL_DATA_OUT:
			text = "=> Send SSL data";
			break;
		default:
			// unknown は何もしないこと
			return 0;
	}
	std::cout << "debug: url " << get_url() <<  " status " << text << " size " << size << std::endl;
	return 0;
}

// Perform時に詳細なDump情報を出すかどうかを設定
void curl_base_easy::set_debugdump(bool onoff)
{
	if(onoff){
		set_verbose(true);			// これをしないと出てこない
		set_debug_callback(_debug_callback_func);
		set_option(CURLOPT_DEBUGDATA, this);
	}else{
		// libcurlのコードを見た感じでは大丈夫(curl/lib/curl_log.c)
		set_debug_callback(nullptr);
		clear_option(CURLOPT_DEBUGDATA);
	}
}

// Perform時に簡易的なDump表示をする
void curl_base_easy::set_verbose(bool onoff)
{
	if(onoff)		set_option(CURLOPT_VERBOSE, 1L);
	else			set_option(CURLOPT_VERBOSE, 0L);
}



// Easyハンドルでファイル転送を開始
// 転送が終わるまで帰ってこない。ブロッキングする
// 注意：multiを使用するときはこれを使わないこと
//
// なにかエラーがでたら例外を投げるのでtry-catchで囲むこと
void curl_base_easy::perform()
{
	// 投げる
	const CURLcode code = curl_easy_perform(handle.get());
	if (code != CURLE_OK) {
		set_error(code);
		throw curl_base_exception(this, __FCNAME, __LINE__);
	}
}
