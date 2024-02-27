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

#include "curlcxx_stream.h"

using libcurlcxx::curl_base_coutstream;
using libcurlcxx::writef::_check_callback_arg;
using std::cout;

// --------------------------------------------------
// cout stream用クラス
// 標準出力にそのまま出力する

static size_t _coutstream_callback(char *buffer, size_t size, size_t nitems, void *outstream)
{
	const auto realsize = _check_callback_arg(buffer, size, nitems);
	if(realsize == 0) return 0;

	std::cout.write(static_cast<const char *>(buffer), realsize);
	return realsize;
}

// コンストラクタ。curl_base_coutstreamのデフォルト動作はこれになる
// なにかデータを受けるたびにcoutに出す
curl_base_coutstream::curl_base_coutstream() noexcept
{
    set_write_callback(_coutstream_callback);
}

