// XParser.cpp
#include <iostream>
#include <stack>
#define BOOST_SPIRIT_USE_OLD_NAMESPACE
//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/classic.hpp>
#include "XParser.h"

using namespace std;
using namespace boost::spirit;

////////////////////////////////////////////////////////////////////////////////
// 
// struct XDataListFactory
// 
// @brief:  XDataListの書き出し器
// @update: 2011/10/11 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
class XDataListFactory
{
	XDataList &data_list_;
	stack<bool> flag_stack_;
	
	XDataListFactory(const XDataListFactory &);
	
	// テンプレートの種類をIDに変換
	short ToId(const string &name)
	{
		if (name == "Mesh")             return XDataList::ID_MESH;
		if (name == "MeshMaterialList") return XDataList::ID_MESH_MATERIAL_LIST;
		if (name == "MeshNormals")      return XDataList::ID_MESH_NORMALS;
		if (name == "Material")         return XDataList::ID_MATERIAL;
		
		// 知らないテンプレート
		return -1;
	}
	
public:
	XDataListFactory(XDataList &data_list)
	: data_list_(data_list)
	{}
	
	// データセットの始め
	void Begin(const string &name)
	{
		short id;
		
		if (flag_stack_.empty())
			// スタックが空なら新しいデータセットは有効
			id = ToId(name);
		else if (flag_stack_.top())
			// 親データセットが有効なら新しいデータセットも有効
			id = ToId(name);
		else
			// 親データセットが無効なので新しいデータセットは強制的に無効
			id = -1;
		
		if (id != -1)
		{
			// データセットは有効
			data_list_.id.push_back(id);
			flag_stack_.push(true);
		}
		else
			// データセットは無効
			flag_stack_.push(false);
	}
	
	// データセットの終わり
	void End()
	{
		if (flag_stack_.top())
			data_list_.id.push_back(XDataList::ID_DATASET_END);
		
		flag_stack_.pop();
	}
	
	// 整数値を追加
	void AddInt(short val)
	{
		// 現在のデータセットが有効なら追加
		if (flag_stack_.top())
			data_list_.int_val.push_back(val);
	}
	
	// 実数値を追加
	void AddReal(double val)
	{
		// 現在のデータセットが有効なら追加
		if (flag_stack_.top())
			data_list_.real_val.push_back(val);
	}
};

////////////////////////////////////////////////////////////////////////////////
// 
// ファンクタ
// 
////////////////////////////////////////////////////////////////////////////////
struct add_begin
{
	XDataListFactory &factory_;
	add_begin(XDataListFactory &factory) : factory_(factory) {}
	void operator()(char const *first, char const *last) const
	{
		factory_.Begin(string(first, last));
	}
};
struct add_end
{
	XDataListFactory &factory_;
	add_end(XDataListFactory &factory) : factory_(factory) {}
	void operator()(char const *first, char const *last) const
	{
		factory_.End();
	}
};
struct add_int
{
	XDataListFactory &factory_;
	add_int(XDataListFactory &factory) : factory_(factory) {}
	void operator()(int val) const
	{
		factory_.AddInt(val);
	}
};
struct add_real
{
	XDataListFactory &factory_;
	add_real(XDataListFactory &factory) : factory_(factory) {}
	void operator()(double val) const
	{
		factory_.AddReal(val);
	}
};

////////////////////////////////////////////////////////////////////////////////
// 
// struct XSkip
// 
// @brief:  Xのスキップパーサ（コメント、空白文字）
// @update: 2011/10/12 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
struct XSkip : public grammar<XSkip>
{
	template<typename ScannarT>
	struct definition
	{
		rule<ScannarT> skip_p;
		
		// 構文定義
		definition(const XSkip &self)
		{
			skip_p = +space_p | comment_p("//") | comment_p('#');
			
			// デバッグ出力（出力しない）
			BOOST_SPIRIT_DEBUG_TRACE_NODE(self, false);
			BOOST_SPIRIT_DEBUG_TRACE_NODE(skip_p, false);
		};
		
		// 開始記号
		const rule<ScannarT> &start() const
		{
			return skip_p;
		}
	};
};

////////////////////////////////////////////////////////////////////////////////
// 
// struct XGrammar
// 
// @brief:  Xの構文定義
// @update: 2011/10/11 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
struct XGrammar : public grammar<XGrammar>
{
	XDataListFactory &factory_;
	
	XGrammar(XDataListFactory &factory)
	: factory_(factory)
	{}

	template<typename ScannarT>
	struct definition
	{
		rule<ScannarT> keyword, uuid, ident, document, templ, member, opening, dataset;
	
		// 構文定義
		definition(const XGrammar &self)
		{
			keyword
				= str_p("array") | "template";
			uuid
				= lexeme_d[
					   '<' >> repeat_p(8)[xdigit_p]
					>> '-' >> repeat_p(4)[xdigit_p]
					>> '-' >> repeat_p(4)[xdigit_p]
					>> '-' >> repeat_p(4)[xdigit_p]
					>> '-' >> repeat_p(12)[xdigit_p]
					>> '>'
				];
			ident
				= lexeme_d[alpha_p >> *alnum_p] - keyword;
			document
				= *(templ | dataset);
			templ
				= "template" >> ident >> '{' >> uuid >> *member >> !opening >> '}';
			member
				= ((ident >> ident) | ("array" >> ident >> ident >> '[' >> (ident | int_p) >> ']')) >> ';';
			opening
				= '[' >> (("...")  | ((ident >> !uuid) % ',')) >> ']';
			dataset
				=  ident[add_begin(self.factory_)]
				>> !ident
				>> '{'
				>> !uuid
				// strict_real_pを先に判定
				>> *(strict_real_p[add_real(self.factory_)] | int_p[add_int(self.factory_)] | ';' | ',' | dataset)
				>> '}'
				>> eps_p[add_end(self.factory_)];

			// デバッグ出力
			BOOST_SPIRIT_DEBUG_RULE(keyword);
			BOOST_SPIRIT_DEBUG_RULE(uuid);
			BOOST_SPIRIT_DEBUG_RULE(ident);
			BOOST_SPIRIT_DEBUG_RULE(document);
			BOOST_SPIRIT_DEBUG_RULE(templ);
			BOOST_SPIRIT_DEBUG_RULE(member);
			BOOST_SPIRIT_DEBUG_RULE(opening);
			BOOST_SPIRIT_DEBUG_RULE(dataset);
		}
		
		// 開始記号
		const rule<ScannarT> &start() const
		{
			return document;
		}
	};
};

//------------------------------------------------------------------------------
// 
// function XParser::Parse()
// 
// @brief:  XファイルからXDataListを生成
// @update: 2011/10/12 by Odashi
// @args:
//     const char *data:     Xフォーマットを格納した'\0'で終わる文字列
//     size_t avail:         '\0'を含まない文字列の長さ
//     XDataList &data_list: データの格納先
// @ret:
//     void: 
// @note:
//     テキスト版Xのみ対応。
//     template構文などは無視します。
// 
//------------------------------------------------------------------------------
void XParser::Parse(const char *data, size_t avail, XDataList &data_list)
{
	// 最初の16バイトはヘッダなので無視（サイズだけ確認しておく）
	if (avail < 16)
		throw Exception("Header mismatched.");
	
	// 書き出し器
	XDataListFactory factory(data_list);
	
	// 文法
	XGrammar g(factory);
	XSkip    skip_p;
	
	// 解析の開始
	parse_info<> r = parse(data+16, g, skip_p);
	
	if (!r.hit)
		throw Exception("Unsupported format.");
}
