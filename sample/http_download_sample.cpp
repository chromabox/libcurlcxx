// The MIT License (MIT)
//
// Copyright (c) <2024> chromabox <chromarockjp@gmail.com>
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

#include <map>
#include <memory>
#include "curlcxx_cdtor.h"
#include "curlcxx_http_req.h"
#include "curlcxx_utility.h"
#include "curlcxx_error.h"

using libcurlcxx::curl_base_cdtor;
using libcurlcxx::curl_base_easy;
using libcurlcxx::curl_base_stringstream;
using libcurlcxx::curl_base_bytestream;
using libcurlcxx::curl_base_utility;
using libcurlcxx::curl_base_exception;

using libcurlcxx::curl_http_request;
using libcurlcxx::curl_http_request_param;

using std::string;
using std::ostream;
using std::ostringstream;
using std::map;

// 使用の際はこれの定義が必要
static libcurlcxx::curl_base_cdtor _libcurl;

// sample URLS

// get abe-hiroshi jpeg (light)
// static constexpr std::string_view download_url_object("http://abehiroshi.la.coocan.jp/abe-top-20190328-2.jpg");
// get linux-kernel-source (135M)
static constexpr std::string_view download_url_object("https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.7.4.tar.xz");
// get huggingface stable diffusion official SDXL model (heavy heavy 6GB)
// static constexpr std::string_view download_url_object("https://huggingface.co/stabilityai/stable-diffusion-xl-base-1.0/resolve/main/sd_xl_base_1.0.safetensors");


// プログレス表示用のコールバック関数。ダウンロード中に定期的に呼び出される
static int download_progress_callback(curl_base_easy *easy, void *pdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	std::cout << "\r\e[1m\e[7m --- downloading: " << easy->get_url() << " " << dlnow <<" / " << dltotal << " ---\e[0m" << std::flush;
	return 0;					// 継続したいので0を返す。もし途中で止める場合は0以外を返すこと
}

// 単にダウンロードとプログレス表示をするサンプル
int main()
{
	curl_http_request req(std::make_shared<curl_base_bytestream>());		// 対象がバイナリの可能性がある場合は、bytestreamを使います
	req.appendHeader("User-Agent: curl/7.81.0");		// UAを指定
	req.appendHeader("Accept-Language: *");
	req.RequestSetupGet(download_url_object);
	req.set_progress_function(true, download_progress_callback);			// ダウンロード中にユーザ定義のコールバック関数で進捗表示させる場合はこのようにする
	// URLからファイル名を抽出
	std::string save_fname = curl_base_utility::url_from_filename(req.get_url());
	if(save_fname.empty()){
		save_fname = "data.dat";		// わからんのでこうなります
	}

	// 詳細なデバッグをしたい場合は以下のコメントを外す
//  req.set_debugdump(false);
//  req.set_verbose(true);

	std::cout << "url: " << req.get_url() << std::endl;
	std::cout << "save_filename: " << save_fname << std::endl;

	try {
		std::cout << "start downloading.....\n" << std::endl;
		req.perform();
	} catch (curl_base_exception &derror) {
		// エラー内容表示
		std::cout << derror.what() << std::endl;
		return -1;
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "error : http code error " << req.get_responceCode() << std::endl;
		return -1;
	}
	std::cout << "\ndownload done!" << std::endl;
	std::cout << "  content type: " << req.get_ContentType() << std::endl;
	std::cout << "  length: " << req.get_ContentLength() << std::endl;

	std::vector<uint8_t> *datvec;
	datvec = (static_cast<curl_base_bytestream* >(req.get_streamer()))->get_stream();
	// からじゃないときは処理する
	if(datvec->empty()){
		std::cout << "content is empty... " << std::endl;
		return 0;
	}

	std::cout << "saveing.... " << save_fname << std::endl;
	std::ofstream file(save_fname, std::ios::out|std::ios::binary|std::ios::trunc);
	// ファイルが正常に開かれたか確認
	if (!file.is_open()) {
		std::cerr << "error :can not open " << save_fname << std::endl;
		return -1;
	}
	file.write(reinterpret_cast<const char* >(datvec->data()), datvec->size());
	file.flush();				// flushしてからじゃないと正確なことはわからない(buffering)
	if (file.fail()) {
		std::cout << "error: file save missing !! " << save_fname << std::endl;
		return -1;
	}
	std::cout << "file saved " << save_fname << std::endl;
	return 0;
}
