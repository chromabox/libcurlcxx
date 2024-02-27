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

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <memory>
#include <algorithm>

namespace libcurlcxx
{
	namespace writef
	{
		// コールバック関数でのパラメタチェック
		// 0だったら処理しないほうが良い
		// 0以外はバイトサイズ
		inline size_t _check_callback_arg(char *buffer, size_t size, size_t nitems)
		{
			const auto realsize = size*nitems;
			if(realsize == 0)	  return 0;
			if(buffer == nullptr) return 0;
			return realsize;
		}
	}  // namespace writef

	// すべてのCurl Streamの根底となるクラス。easyはこれを使うのでこのクラスを派生して使用すること
	class curl_base_stream_object
	{
	private:
		curl_write_callback _write_callback;		// CURLOPT_WRITEFUNCTIONで設定するコールバック関数
	protected:
		// コールバック関数のセット
		virtual void set_write_callback(curl_write_callback fcallbk) noexcept {
			_write_callback = fcallbk;
		}

	public:
		// コールバック関数の取得。CURLOPT_WRITEFUNCTIONで設定する時に使う
		curl_write_callback get_write_function() const noexcept {
			return _write_callback;
		}
		inline virtual std::string get_string() const & {
			return "";
		}

		curl_base_stream_object(){}
		virtual ~curl_base_stream_object(){}
	};

	// 何か受け取るたびにコンソールにのみ出力するストリームクラス
	// このクラスはオブジェクトを保持していないので、受信動作が終わったあとで文字列を取るということは出来ない
	class curl_base_coutstream : public curl_base_stream_object
	{
	public:
		curl_base_coutstream() noexcept;
		virtual ~curl_base_coutstream(){}
	};

	// クラスオブジェクトTが write(buffer,size) を使用可能な場合のテンプレートクラス
	// クラスオブジェクトの実体はunique_ptrなためこのクラスに格納され続け移譲はできない
	template<class T> class curl_base_unique_stream : public curl_base_stream_object
	{
	private:
		std::unique_ptr<T> _pstream;

		// CURLOPT_WRITEFUNCTIONで設定してCurlから呼ばれるコールバック関数
		// c++とCurlを組み合わせる場合のコールバックはこうやって呼ぶと良いそうな
		// https://curl.se/docs/faq.html#Using_C_non_static_functions_f

		// 実際にデータをストリームに書く関数
		size_t internal_write(char *buffer, size_t realsize)
		{
			get_stream()->write(static_cast<const char *>(buffer), realsize);
			return realsize;
		}
		// 何かサーバからデータが来るとこれがCurlから呼ばれる
		static size_t _callback_func(char *buffer, size_t size, size_t nitems, void *outstream)
		{
			auto realsize = writef::_check_callback_arg(buffer, size, nitems);
			if(realsize == 0) return 0;
			return static_cast<curl_base_unique_stream*>(outstream)->internal_write(buffer, realsize);
		}
	public:
		curl_base_unique_stream()
		{
			_pstream = std::make_unique<T>();
			set_write_callback(_callback_func);
		}
		virtual ~curl_base_unique_stream(){}

		// ストリームへのポインタを返す。これは一時的なものである。解放してはいけない
		T* get_stream() const noexcept {return _pstream.get();}

		// Stringの取得
		inline virtual std::string get_string() const &
		{
			// constexpr_ifを使用してstringを返せそうなテンプレートの場合は返すこととする(c++17)
			if constexpr (std::is_same_v<T, std::stringstream>){
				return _pstream.get()->str();
			}else{
				return "";
			}
		}
	};

	// クラスオブジェクトTが write(buffer,size) を使用可能な場合のテンプレートクラス
	// クラスオブジェクトの実体はweak_ptrなため、ポインタはこのクラスに格納はされない
	// 呼び出し元でshared_ptr<T>のオブジェクトを作ってから、このコンストラクタに指定する
	template<class T> class curl_base_weak_stream : public curl_base_stream_object
	{
	private:
		std::weak_ptr<T> _pstream;

		// 実際にデータをストリームに書く関数
		size_t internal_write(char *buffer, size_t realsize)
		{
			if(auto ptr = _pstream.lock()){									// expiredはする必要なし。expireしたらlockで空を返す(falseとなる)のでこれでいい
				ptr->write(static_cast<const char *>(buffer), realsize);
			}
			return realsize;
		}
		// 何かサーバからデータが来るとこれがCurlから呼ばれる
		static size_t _callback_func(char *buffer, size_t size, size_t nitems, void *outstream)
		{
			auto realsize = writef::_check_callback_arg(buffer, size, nitems);
			if(realsize == 0) return 0;
			return static_cast<curl_base_weak_stream*>(outstream)->internal_write(buffer, realsize);
		}

	public:
		explicit curl_base_weak_stream(std::shared_ptr<T>& pstream)
		{
			_pstream = pstream;
			set_write_callback(_callback_func);
		}
		virtual ~curl_base_weak_stream(){}
		// ストリームのSharedPtrをセットする
		virtual void set_streamptr(std::shared_ptr<T>& pstreame) noexcept { _pstream = pstreame;}

		// Stringの取得
		inline virtual std::string get_string() const &
		{
			// constexpr_ifを使用してstringを返せそうなテンプレートの場合は返すこととする(c++17)
			if constexpr (std::is_same_v<T, std::stringstream>){
				return _pstream.lock()->str();
			}else{
				return "";
			}
		}
	};

	// クラスオブジェクトTyが std::vector<Ty> 場合のテンプレートクラス
	// クラスオブジェクトの実体はunique_ptrなためこのクラスに格納され続け移譲はできない

	template <typename Ty> class curl_base_unique_vec_stream : public curl_base_stream_object
	{
	private:
		std::unique_ptr<std::vector<Ty>> _pvec;

		// Vectorをストリームに見立ててデータを書き込む
		size_t internal_write(char *buffer, size_t realsize)
		{
			std::vector<Ty>* const pvec = get_stream();
			const auto oldsize = pvec->size();

			pvec->resize(oldsize + realsize);
			std::copy(buffer, buffer+realsize, pvec->begin()+oldsize);

			return realsize;
		}
		// 何かサーバからデータが来るとこれがCurlから呼ばれる
		static size_t _callback_func(char *buffer, size_t size, size_t nitems, void *outstream)
		{
			auto realsize = writef::_check_callback_arg(buffer, size, nitems);
			if(realsize == 0) return 0;
			return static_cast<curl_base_unique_vec_stream*>(outstream)->internal_write(buffer, realsize);
		}

	public:
		curl_base_unique_vec_stream()
		{
			_pvec = std::make_unique<std::vector<Ty>>();
			set_write_callback(_callback_func);
		}

		virtual ~curl_base_unique_vec_stream(){}
		// 一時的なベクタへのポインタを返す
		std::vector<Ty>* get_stream() const noexcept {return _pvec.get();}
	};


	// クラスオブジェクトTyが std::vector系 場合のテンプレートクラス
	// クラスオブジェクトの実体はweak_ptrなため、ポインタはこのクラスに格納はされない

	template <typename Ty> class curl_base_weak_vec_stream : public curl_base_stream_object
	{
	private:
		std::weak_ptr<std::vector<Ty>> _pvec;

		// Vectorをストリームに見立ててデータを書き込む
		size_t internal_write(char *buffer, size_t realsize)
		{
			if(auto pvec = _pvec.lock()){
				const auto oldsize = pvec->size();

				pvec->resize(oldsize + realsize);
				std::copy_n(buffer, realsize, std::next(pvec->begin(), oldsize));
//  std::copy(buffer, buffer+realsize, pvec->begin()+oldsize);
			}
			return realsize;
		}
		// 何かサーバからデータが来るとこれがCurlから呼ばれる
		static size_t _callback_func(char *buffer, size_t size, size_t nitems, void *outstream)
		{
			auto realsize = writef::_check_callback_arg(buffer, size, nitems);
			if(realsize == 0) return 0;
			return static_cast<curl_base_weak_vec_stream*>(outstream)->internal_write(buffer, realsize);
		}

	public:
		explicit curl_base_weak_vec_stream(std::shared_ptr<std::vector<Ty>>& vec)
		{
			_pvec = vec;
			set_write_callback(_callback_func);
		}

		virtual ~curl_base_weak_vec_stream(){}
		// ストリームのSharedPtrをセットする
		virtual void set_streamptr(std::shared_ptr<std::vector<Ty>>& vec) noexcept { _pvec = vec;}
	};

	// 基本的に使用されると思われるストリームの型宣言
	// curl_base_unique_streamで宣言されているものはオブジェクトが内包されていて、初期化時に指定の必要はない。
	// オブジェクトの消滅もクラス内で完結し何もする必要はない。
	// curl_base_weak_streamで宣言されているものはオブジェクトは内包されていないので、初期化時にそのshard_ptr型変数を作って引き渡す必要がある
	// オブジェクトの消滅時には何もしないのでオブジェクトの管理は呼び出し元で行う必要がある

	// 結果を文字列(String)で受けたいときはこれを使う
	typedef curl_base_unique_stream<std::stringstream> 	curl_base_stringstream;

	// 初期化時にString型のshard_ptr変数を外からセットしたい場合はこれを使う
	typedef curl_base_weak_stream<std::stringstream>	curl_base_stringstr_ptr;
	// 初期化時にfstream型のshard_ptr変数を外からセットしたい場合はこれを使う
	typedef curl_base_weak_stream<std::fstream>			curl_base_fstr_ptr;

	// 結果をバイト列(std::vector<uint8_t>)で受けたい場合に使う
	typedef curl_base_unique_vec_stream<uint8_t>		curl_base_bytestream;
	// 結果をバイト列(std::vector<uint8_t>)で受けたい場合に使うが、初期化時にshard_ptr<std::vector<uint8_t>>の変数を外からセットしたい場合に使う
	typedef curl_base_weak_vec_stream<uint8_t>			curl_base_bytestr_ptr;
}  // namespace libcurlcxx
