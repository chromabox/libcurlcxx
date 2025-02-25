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

#pragma once

#include <map>
#include <string>
#include <memory>
#include <vector>
#include "curlcxx_easy.h"
#include "curlcxx_multi.h"
#include "curlcxx_slist.h"
#include "curlcxx_utility.h"
#include "curlcxx_http_req.h"

namespace libcurlcxx
{
	class curl_base_mime;

	class curl_websocket : public curl_base_easy
	{
	private:
		// proxy
		// コピー禁止
		curl_websocket &operator=(curl_websocket const &) = delete;
		curl_websocket(curl_websocket const &) = delete;

		curl_base_slist			http_header;		// 設定したカスタムヘッダ

		std::string build_get_param(const curl_http_request_param& params);

		bool	isconnected;					// 接続中かどうか
		bool	sendrecv_debug;					// sendとrecvのデバッグフラグ
		size_t	internal_rbufsize;				// 内部受取バッファサイズ

	public:
		curl_websocket();
		explicit curl_websocket(std::string_view url);
		curl_websocket(std::string_view url, const curl_http_request_param& params);

		curl_websocket(curl_websocket &&other) noexcept;
		curl_websocket &operator= (curl_websocket &&other) noexcept;

		~curl_websocket() noexcept;

		virtual bool RequestSetupGet(std::string_view url);
		virtual bool RequestSetupGet(std::string_view url, const curl_http_request_param& params);

		virtual void prePerform();
		virtual void perform();

		void close();

		virtual void appendHeader(std::string_view data);
		virtual void removeHeader();

		inline void setDebug(bool onoff = true) 	{sendrecv_debug = onoff;}
		inline bool isDebug() 					{return sendrecv_debug;}

		// Recv生関数
		// libcurlをよく理解している人用
		//
		// vec: 受信バッファのベクタ
		// recv: 実際に受信したバイト数
		// meta: 追加情報。ここにTEXTやBINARYを受け取ったとかいろんな情報が返される
		//
		// return
		// CURLE_OK: なにかデータが来ている
		// CURLE_AGAIN: データがなにも到達していない(エラーではない)。場合によって再度実行の必要あり
		// CURLE_GOT_NOTHING: 切断された可能性があるので切断した
		// その他: エラー
		template<typename T>
		CURLcode recv_raw(std::vector<T> &vec, size_t &recv, const struct curl_ws_frame **meta)
		{
			recv = 0;
			CURLcode res = curl_ws_recv(handle.get(), vec.data(), vec.size(), &recv, meta);
			if(res == CURLE_AGAIN) return res;		// AGAINは特に何もしない(よく帰ってくる)

			if(isDebug()){
				std::cout << "curl_websocket::recv_raw: res " << res << std::endl;
				std::cout << "curl_websocket::recv_raw: readed byte " << recv << std::endl;
				// CURLE_OKじゃないとMETAはNullであるので注意せよ
				if(res == CURLE_OK){
					const struct curl_ws_frame *metap = *meta;

					std::cout << "curl_websocket::recv_raw: bytesleft " <<  metap->bytesleft << std::endl;
					if(metap->flags & CURLWS_PONG) std::cout << "flag:: PONG" << std::endl;
					if(metap->flags & CURLWS_PING) std::cout << "flag:: PING" << std::endl;
					if(metap->flags & CURLWS_TEXT) std::cout << "flag:: TEXT" << std::endl;
					if(metap->flags & CURLWS_BINARY) std::cout << "flag:: BIN" << std::endl;
					if(metap->flags & CURLWS_CLOSE) std::cout << "flag:: CLOSE" << std::endl;
					if(metap->flags & CURLWS_CONT) std::cout << "flag:: CONTINUE" << std::endl;
				}
			}

			if(res == CURLE_GOT_NOTHING){
				// 通信切れたっぽいので切断
				if(isDebug()){
					std::cout << "curl_websocket::recv_raw Got Noting!" << std::endl;
				}
				close();
			}
			return res;
		}


		CURLcode recv_meta(bool &istext, curl_off_t &byteleft);

		bool recv_binary(std::vector<uint8_t> &rvec);
		bool recv_text(std::string &rtext);
		bool recv_text(std::stringstream &rtextstr);

		// 何か受信したかを返す
		// これは次に続くデータがTextかBinaryかの判断にも必要
		// なにも受信がなければブロッキングせずに帰ってくる(iserr=false, return=false)
		//
		// istext: 受信したデータにテキストフラグが立っているかどうか
		//    true = 受信したデータがテキストである。recv_textで受けられる
		//    false = 受信したデータがバイナリである。recv_binaryで受けること推奨
		// iserr: エラー、もしくは切断が発生した場合はtrue
		//
		// return:
		//    true = 何かデータを受け取った。recv_xxxで受取可能
		//    false = データなしかエラー。エラーはiserrを見ること。iserrがfalseな場合は単にデータ来てないだけ
		inline bool is_recved(bool &istext, bool &iserr)
		{
			CURLcode res;
			curl_off_t dummy;

			iserr = false;
			istext = false;

			res = recv_meta(istext, dummy);
			if(res == CURLE_AGAIN) return false;		// AGAINは何も来ていないという意味合い
			if(res == CURLE_OK) return true;			// なんか来ている
			// ここに来ているときはエラーか接続断発生
			iserr = true;
			return false;
		}

		// recvで一度に受け取る内部バッファのサイズを設定する
		inline void set_recvbufsize(size_t bufsize) noexcept { internal_rbufsize = bufsize;}
		// Connectが成立しているかどうかを返す。falseならまだ接続できていない。
		inline bool isConnection()		{return isconnected;}
	};

}  // namespace libcurlcxx
