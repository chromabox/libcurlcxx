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

#include "curlcxx_slist.h"
#include "curlcxx_error.h"

#include "classfname.h"

using libcurlcxx::curl_base_slist;
using libcurlcxx::curl_base_exception;

using std::string_view;


// ----------------------------------------
// curl_base_slist: curl_slistをC++で表現したもの
// header情報を設定する場合に使う
// appendを使って設定していき、easy.set_option(CURLOPT_HTTPHEADER,slist.get_slistptr()); などどして引き渡すことができる
// このオブジェクトはperformが終わり結果が帰ってくるまで消してはいけない

// コンストラクタ。普通はこれを使う
curl_base_slist::curl_base_slist()
{
}

curl_base_slist::~curl_base_slist()
{
}

// ムーブコンストラクタ
curl_base_slist::curl_base_slist(curl_base_slist &&other) noexcept
{
	_slist = std::move(other._slist);
}

// ムーブ代入演算子
curl_base_slist & curl_base_slist::operator= (curl_base_slist &&other) noexcept
{
	if (this != &other) {
		_slist = std::move(other._slist);
	}
	return *this;
}


// データをSlistに追記する
//
// 失敗したら例外を返すので注意
// value: slistに格納したいデータ
void curl_base_slist::append(std::string_view value)
{
	curl_slist * p_slist = curl_slist_append(_slist.get(), value.data());
	if(p_slist == nullptr){
		throw curl_base_exception("error: curl_slist_append", __FCNAME, __LINE__);
	}
	// p_slistは更新される(Returns the new list, after appending.)
	// よって、ここでresetだけをしてしまうとDeleteが走ってまずいのでreleaseして所有権を放棄してからResetをする
	_slist.release();			// 所有権を外して…
	_slist.reset(p_slist);		// 純粋にセットする
}

// slistの内容を全部破棄する
void curl_base_slist::reset()
{
	_slist.reset();
}
