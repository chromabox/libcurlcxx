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

// このコードは以下を参照してnamespaceをつける、一部変更などしました。お礼申し上げます
//
// PicojsonのJSON書き出しをもっと便利にする
// @Takap
// https://takap-tech.com/entry/2019/10/24/235423

#pragma once

#include <vector>
#include <utility>
#include <string>
#include <string_view>

#include "picojson.h"


namespace picojson_helper
{
	// picojsonでJsonデータを書くためのヘルパクラス
	class writer
	{
	private:
		picojson::object obj;

	public:
		operator picojson::object() { return obj; }
		operator picojson::value() { return picojson::value(obj); }

		picojson::object getObject() { return *this; }
		picojson::value toValue() { return *this; }

		// オブジェクトに値を追加する
		inline static void add(picojson::object& sobj, std::string_view key, double value)
		{
			sobj.insert(std::make_pair(key.data(), value));
		}

		inline static void add(picojson::object& sobj, std::string_view key, int64_t value)
		{
			sobj.insert(std::make_pair(key.data(), value));
		}

		inline void add(std::string_view key, const int value)
		{
			add(obj, key, static_cast<int64_t>(value));
		}
		inline void add(std::string_view key, const unsigned int value)
		{
			add(obj, key, static_cast<int64_t>(value));
		}
		inline void add(std::string_view key, const float value)
		{
			add(obj, key, static_cast<double>(value));
		}
		inline void add(std::string_view key, const bool &value)
		{
			obj.insert(std::make_pair(key.data(), picojson::value(value)));
		}
		inline void add(std::string_view key, const char* value)
		{
			obj.insert(std::make_pair(key.data(), picojson::value(value)));
		}
		inline void add(std::string_view key, const std::string& value)
		{
			obj.insert(std::make_pair(key.data(), picojson::value(value)));
		}
		inline void add(std::string_view key, std::string_view value)
		{
			obj.insert(std::make_pair(key.data(), picojson::value(value.data())));
		}
		inline void add(std::string_view key, picojson_helper::writer child)
		{
			obj.insert(std::make_pair(key.data(), child.toValue()));
		}

		template<class T>
		void add(std::string_view key, T value, const std::function<picojson_helper::writer(T)>& convert)
		{
			obj.insert(std::make_pair(key.data(), convert(value).getValue()));
		}

		template<class T>
		void add(std::string_view key, const std::vector<T>& list, const std::function<picojson_helper::writer(T)>& convert)
		{
			picojson::array jsonList;
			for (const auto& item : list)
			{
				jsonList.push_back(convert(item).toValue());
			}

			obj.insert(std::make_pair(key.data(), picojson::value(jsonList)));
		}

		std::string serialize(bool prettify = false)
		{
			return toValue().serialize(prettify);
		}
	};
}  // namespace picojson_helper
