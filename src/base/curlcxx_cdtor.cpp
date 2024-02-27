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

#include "curlcxx_cdtor.h"
#include "curlcxx_error.h"
#include "curlcxx_utility.h"

#include "classfname.h"

using libcurlcxx::curl_base_cdtor;
using libcurlcxx::curl_base_error;
using libcurlcxx::curl_base_utility;

// curlのサポートバージョン
// TODO(chromabox): websocketが7.86.0以降なので今後そのようにするかもしれない
#define SUPPORTED_LIBCURL_VERSION_NUM 	CURL_VERSION_BITS(7, 81, 0)

// ビルドされるときにとりあえずバージョンを観ておく
#if (LIBCURL_VERSION_NUM < SUPPORTED_LIBCURL_VERSION_NUM)
#error "unsupport curl version!!!! check you libcurl version..."
#endif


// curl_base_cdtor:
// curl_global_initアプリの最初に呼び出す必要があり
// curl_global_cleanupはアプリの最後に呼び出す必要があるためクラス化している
// 通常の場合は単純にグローバル変数としてこれを定義すれば自動的にInitとCleanupが呼ばれるが
// WINDOWSのDLLの場合はDllMainから呼ぶとまずい

curl_base_cdtor::curl_base_cdtor()
{
	const CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if (code != CURLE_OK) {
		std::cerr << "FAIL; curl global init..STOP! " << std::endl;
		throw curl_base_exception("FAIL; global init..STOP! ", __FCNAME, __LINE__);
	}
}

curl_base_cdtor::~curl_base_cdtor()
{
	curl_global_cleanup();
}
