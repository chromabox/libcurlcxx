# libcurlcxx

クライアントサイドURL転送ライブラリの`cURL(libcURL)`をmodern C++でラッピングしたライブラリです  
あまり深く考えなくても簡単に、しかし軽めにHttp(s)通信を扱うというのをコンセプトにしています  
CURLで使われるハンドルをスマートポインタで包むことによってオブジェクトのスコープor参照から外れると安全に解放できるようにしています。  
つまり、所有権問題を考えなくて済むようにしています  
  
というよりほぼ自分用に作りました  
`libcurl`をスタティックリンクするのでライブラリの依存関係も少なくてすみます。
  
あと、豊富なサンプル付きです  
  
curlについてより詳しくは→ http://curl.haxx.se/
  
> [!NOTE]
> 現状はlinuxのみです。(Ubuntu 22.04で確認済み)  
> windows環境ではビルド含めて未確認です。  

---
  
## ビルド方法:

まず、このプロジェクトをcloneします。
```bash
$ git clone https://github.com/chromabox/libcurlcxx.git --recursive
```
ついうっかり`--recursive`を忘れてcloneしてしまった場合は以下を行います。  
```bash
$ cd libcurlcxx
$ git submodule update --init
```
  
また、ビルドにcmakeを使っているのでcmakeは必須です  
gcc は c++20が通るバージョンが必要になります。  
(c++17でも通るとは思いますがCMakeの設定で20にしているので、c++17でどうしても…な方は`CMakeLists.txt`を適時変更してください)  
  
ubuntu 22.04の場合は次のようになります。(OpenSSL版。こちらを推奨)  
```bash
$ sudo apt install build-essential cmake cpplint libssl-dev 
```
か、もしくは(gnuTLS版)
```bash
$ sudo apt install build-essential cmake cpplint libgnutls28-dev
```
か、あるいは(NSS版)
```bash
$ sudo apt install build-essential cmake cpplint libnss3-dev 
```
を実行してsslの開発用ライブラリを入れて下さい。  
(sslライブラリ系が3つに別れているのはcurl内で使用しているSSL用ライブラリが色々選択可能なためで、基本的に提供される機能に差は無いので好きなのを入れてください…と言いたいところですが、openssl版でしか確認してないのでできればlibssl-devにしてください)  
  
その後
```bash
$ ./build.sh
```
でOKです。  
makeが終わると、まず`curl`のビルドが始まり、build以下に`libcurlcxx.a`が生成されます。  
プロジェクトからはこのファイルを直接指定してリンクしてください。  
  
ビルドはライブラリアーカイブ(.a)のビルドのみ対応しています。  
大元の`curl`もスタティックリンクしています。  
したがって、このライブラリを使用したい側からはスタティックリンクのような感じとなります  
  

---
## サンプルのビルド方法:

サンプルは次のようにしてビルドします
```bash
$ ./build.sh --debugsample 
```
もしくは、`vscode`を使用してこのライブラリのディレクトリを開いた場合は、`ビルドタスクの実行`でもビルドが可能です。  
  
chatgpt APIのサンプルを確かめたい場合は、サンプルの実行前に事前にOpenAIのapikeyを環境変数`CHATGPT_USER_APIKEY`に設定する必要があります。  
(要OpenAPIへの課金)  
1. OpenAIからchagGPTのAPIキーを設定するなりなんなりする  
2. 以下のようにしてサンプル実行 `CHATGPT_USER_APIKEY="APIキー" ./chatgpt_api_sample "Hello world!!"`  
  
Blueskyのタイムライン取得サンプルを確かめたい場合は、サンプルの実行前に事前にアプリパスワードを環境変数`BLUESKY_APP_PASSWD`に設定する必要があります。  
Blueskyのアカウントも必要です。  
1. アプリパスワードはBlueskyアプリの「設定」ー「アプリパスワード」ー「アプリパスワードを追加」で取得する  
2. 以下のようにしてサンプル実行 `BLUESKY_APP_PASSWD="アプリパスワード" ./bluesky_timeline_read "あなたのスクリーンネーム"`  
  
---
## コードの書き方:
特殊なことをする場合以外は、`curl_http_request`クラスを使えば大体実装できるようにしています。  
`curl_http_request`クラスでは事足りない場合は、`curl_base_easy`クラスを派生させるか、`curl_http_request`クラスを派生します。  

一つだけ決まりがありまして、以下を使用するアプリのソースコード(ヘッダではなくcpp)のどこか一箇所に書いてください  
この中でcURLのグローバルな初期化とアプリが終了したときの破棄を確実に行うためです  
```c++
static libcurlcxx::curl_base_cdtor _libcurl;
```
簡単なGetリクエストなら以下のように記述できます。  
```c++
// 使用の際はこれの定義が必要
static libcurlcxx::curl_base_cdtor _libcurl;

int main()
{
	std::string_view url("https://httpbin.org/get");

	curl_http_request req(std::make_shared<curl_base_stringstream>());
	req.appendHeader("User-Agent: testman/0.1");
	req.appendHeader("Accept-Language: *");
	req.RequestSetupGet(url);

	std::cout << "url: " << req.get_url() << std::endl;
	
	try {
		req.perform();
	} catch (curl_base_exception &error) {
		// エラー内容表示
		std::cerr<<error.what()<<std::endl;
		return -1;
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "http code error " << req.get_responceCode() << std::endl;
		return -1;
	}
	std::cout << req.get_ContentString() << std::endl;
	return 0;
}
```
POSTの場合は以下のようになります  
```c++
static libcurlcxx::curl_base_cdtor _libcurl;

int main()
{
	std::string_view url("https://httpbin.org/post");
	curl_http_request req(std::make_shared<curl_base_stringstream>());

	try {
		// POSTとして投げるデータを設定。
		// 文字列だけならcurl_http_request_paramを使ったほうが楽
		curl_http_request_param para;
		para["name"] = "aaa";
		para["pass"] = "psps";
		req.RequestSetupPost(url, para);
	} catch (curl_base_exception &error) {
		// エラー内容表示
		std::cerr<<error.what()<<std::endl;
		return -1;
	}
	std::cout << "url: " << req.get_url() << std::endl;
	
	try {
		req.perform();
	} catch (curl_base_exception &error) {
		// エラー内容表示
		std::cerr << error.what() << std::endl;
		return -1;
	}
	// httpコード取得
	if(req.get_responceCode() != 200){
		std::cout << "htto code error " << req.get_responceCode() << std::endl;
		return -1;
	}
	std::cout << req.get_ContentString() << std::endl;

	return 0;
}
```
他、実践的な使用方法はsample以下のディレクトリのサンプルを観てください  
  
サンプルは以下のとおりです  
* get_sample --- 単純なGetリクエストのサンプルです。  
Getリクエストでサーバに引き渡すパラメータが無い場合のサンプルになります  
* multi_sample --- 単純なマルチハンドルのサンプルです。  
3つのサイトに対して並列にGetリクエストを発行します  
* http_get_sample --- Getリクエストのサンプルです。  
リクエスト時にサーバに引き渡すパラメータの渡し方のサンプルです。  
* http_post_sample --- Getリクエストのサンプルです。  
リクエスト時にサーバに引き渡すパラメータの渡し方のサンプルです。  
* http_multi_sample --- マルチハンドルのサンプルです。  
multi_sampleと似ていますが若干手数が少なくなっています。  
* http_download_sample --- Getリクエストで何らかのファイルをダウンロードして保存するサンプルです。  
デフォルトではLinuxカーネルソースのダウンロードをします(100MB程度)。  
ダウンロード進捗状況の表示方法、URLからファイル名の抜き出しかたなどの応用サンプルにもなっています。  
* chatgpt_api_sample --- chatgptのAPIにメッセージを送って結果を得るサンプルです。  
事前にOpenAIのAPIキーが必要になります。 
* mastodon_public_read --- mastodonインスタンスからタイムラインを取得します。アカウントは必要ありません。  
ローカルタイムラインかグローバルタイムラインかのどちらかを選べます
* misskey_public_read --- misskeyサーバからノート（タイムライン）を取得します。アカウントは必要ありません。  
ローカルかグローバルかのどちらかを選べます  
(注意：misskeyのAPIは仕様が変わりやすいので動かなくなっているかもしれません)
* bluesky_timeline_read --- blueskyのサーバからタイムラインを取得します  
事前にアプリパスワードを取得して、環境変数「BLUESKY_APP_PASSWD」に設定してから実行してください。
  
## 外部プロジェクトからのライブラリ使用方法:
  
このライブラリを外部プロジェクトからスタティックリンクするには、次のようにします  
例では external_libs に、この libcurlcxx があるとします。  
```bash
g++ -std=gnu++20 example.cpp -Iexternal_libs/libcurlcxx/extlibs/curl/include -Iexternal_libs/libcurlcxx/include/base -Iexternal_libs/libcurlcxx/include/ext -Iexternal_libs/libcurlcxx/include/http external_libs/libcurlcxx/build/libcurlcxx.a -l:libcurlcxx.a  -Lexternal_libs/libcurlcxx/build 
```
この記述はなかなか大変なので`cmake`を使う方法を推奨します。  
上記記述を使わずとも簡単に導入できます。  

`cmake`を使用している場合は、以下のようにプロジェクトの`CMakeLists.txt`に追記するだけでOKです。  
例では プロジェクト名は sample として、external_libs にこの libcurlcxx があるとします。  
```
add_subdirectory(external_libs/libcurlcxx)
.....
target_include_directories(sample PUBLIC ${LIBCURLCXX_INC_DIRS})
.....
target_link_libraries(sample PRIVATE curlcxx )

```
あとは普通にcmakeでのビルドをすれば適用されます。  
  
あなたのプロジェクトがvscodeを使用している場合は、以下のように`c_cpp_properties.json`の`includePath`を設定するとエディタでエラーがでなくなりコード補完も有効になります。  
例では external_libs に、この libcurlcxx があるとします。  
```json
    "configurations": [
        {
         //...
            "includePath": [
                "external_libs/libcurlcxx/extlibs/curl/include",
                "external_libs/libcurlcxx/include/base",
                "external_libs/libcurlcxx/include/http",
                "external_libs/libcurlcxx/include/ext",
                "external_libs/libcurlcxx/include"
         //...
            ],
         //...
        }
    ],
```
この辺りの一連の外部プロジェクトから`libcurlcxx`を使用するサンプルは[libcurlcxx_sample](https://github.com/chromabox/libcurlcxx_sample)を参照してください  

---
## TODO:  
* web socket対応したい  
* cookie関連の実装をする  
* windows対応
* 完全には無理だけどできるだけヘッダに実装を持ってくる(リファクタリング)
* Delete/Put/Patchリクエストの対応  
* 他、カスタムリクエストの対応  
---
## 改善したいのでプルリク(PR)したい、問題をみつけたのでIssueを建てたい:  

ありがとうございます。  
まずはこちらを一読お願いします。  
https://github.com/chromabox/libcurlcxx/blob/master/.github/CONTRIBUTING.md


---
## 更新履歴:
* 2024/02/27 : 0.1.0  
初リリースバージョン

* 2024/02/XX : 0.0.9  
プレバージョン

---
## 謝辞、参考元:
このプロジェクトは以下のプロジェクトや研究の影響を受けています。  
これらのプロジェクトの開発者および貢献者に敬意と感謝の意を表します  
    
このプロジェクトはjosephp91氏のlibcurlcppをかなり参考にしました。この場を借りて御礼申し上げます。  
https://josephp91.github.io/curlcpp  
このプロジェクトはcurlを利用しております。Cではcurlが無ければHttp通信一つすら大変です。この場を借りて御礼申し上げます。  
https://curl.se/  
サンプル(jsonの解析)にはkazuho氏picojsonを使用しています。いつもお世話になっております。    
https://github.com/kazuho/picojson  
サンプル(jsonの書き出し)にはTakap氏のコードを使用しています。いつもお世話になっております。    
https://takap-tech.com/entry/2019/10/24/235423  