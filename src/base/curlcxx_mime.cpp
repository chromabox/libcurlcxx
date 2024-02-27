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

#include "curlcxx_mime.h"
#include "curlcxx_error.h"

#include "classfname.h"

using libcurlcxx::curl_base_easy;
using libcurlcxx::curl_base_mime;
using libcurlcxx::curl_base_exception;

using std::string;
using std::string_view;

// ----------------------------------------
// curl_base_mime:curl_mimeをC++で表現したもの
// POSTでフォーム情報を投げるときに使う
// add_partを使って設定していき、easy.set_option(CURLOPT_MIMEPOST,mime.get_mimeptr()); などどして引き渡すことができる
// このオブジェクトはperformが終わり結果が帰ってくるまで消してはいけない


// コンストラクタ。普通はこれを使う
// 失敗したら例外を返すので注意
// easy: このMimeを使用するeasyオブジェクト
curl_base_mime::curl_base_mime(const libcurlcxx::curl_base_easy &easy)
{
	curl_mime* p = curl_mime_init(easy.get_chandle());
	if(p == nullptr){
		throw curl_base_exception("error: curl_mime_init", __FCNAME, __LINE__);
	}
	_mime.reset(p);
}

curl_base_mime::~curl_base_mime()
{
}

// ムーブコンストラクタ
curl_base_mime::curl_base_mime(curl_base_mime &&other) noexcept
{
	_mime = std::move(other._mime);
}

// ムーブ代入演算子
curl_base_mime & curl_base_mime::operator= (curl_base_mime &&other) noexcept
{
	if (this != &other) {
		_mime = std::move(other._mime);
	}
	return *this;
}

// 名前のみをmime partオブジェクトに追加する
// クラス外からは呼ばれることはない
//
// 失敗したら例外を返すので注意
// part: create_partなどで作ったpartオブジェクトのポインタ
// name: 名前
void curl_base_mime::set_name_part(curl_mimepart *part, std::string_view name)
{
	CURLcode ret = curl_mime_name(part, name.data());
	if(ret != CURLE_OK){
		throw curl_base_exception("error: curl_mime_name", __FCNAME, __LINE__);
	}
}

// 空のmime partを作って返す。自動的にPartオプジェクトに登録される
// クラス外からは呼ばれることはない
// 失敗したら例外を返すので注意
curl_mimepart* curl_base_mime::create_part()
{
	curl_mimepart* h_part = curl_mime_addpart(_mime.get());
	if(h_part == nullptr){
		throw curl_base_exception("error: curl_mime_addpart", __FCNAME, __LINE__);
	}
	return h_part;
}

// 名前と何らかの文字列のmime_partを追加する
// 失敗したら例外を返すので注意
// name: 名前
// str: なんらかの文字列
void curl_base_mime::add_part(std::string_view name, std::string_view str)
{
	curl_mimepart* h_part = create_part();
	// お名前を設定
	set_name_part(h_part, name);
	// データを設定。CURL_ZERO_TERMINATEDを渡すとlibcurl内でNullターミネーションを観てくれる
	CURLcode ret = curl_mime_data(h_part, str.data(), CURL_ZERO_TERMINATED);
	if(ret != CURLE_OK){
		throw curl_base_exception("error: curl_mime_data", __FCNAME, __LINE__);
	}
}

// 名前と何らかのデータ列のmime_partを追加する
// 失敗したら例外を返すので注意
// name: 名前
// data: データ列へのポインタ
// size: データ列のByteサイズ
void curl_base_mime::add_part(std::string_view name, const char* data, size_t size)
{
	curl_mimepart* h_part = create_part();
	// お名前を設定
	set_name_part(h_part, name);
	// データを設定
	CURLcode ret = curl_mime_data(h_part, data, size);
	if(ret != CURLE_OK){
		throw curl_base_exception("error: curl_mime_data", __FCNAME, __LINE__);
	}
}
