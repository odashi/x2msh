// XParser.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#define BOOST_SPIRIT_USE_OLD_NAMESPACE
//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/classic.hpp>
#include "XParser.h"

using namespace std;
using namespace boost::spirit;

////////////////////////////////////////////////////////////////////////////////
// 
// struct MshWriter
// 
// @brief:  mshの書き出し器
// @update: 2011/10/08 by Odashi
// @note:
//     Mesh, MeshMaterialList, Materialのみ認識する
// 
////////////////////////////////////////////////////////////////////////////////
class MshWriter
{
	ofstream fout;
	stack<bool> flag_stack;
	
	MshWriter(const MshWriter &);
public:
	enum Id
	{
		ID_TEMPLATE_END       = 0,
		ID_MESH               = 1,
		ID_MESH_MATERIAL_LIST = 2,
		ID_MATERIAL           = 3
	};

	MshWriter(const char *filename)
	: fout(filename, ios::binary)
	{}
	
	// ヘッダの書き出し
	void Header()
	{
		fout.write("MSH ", 4);
	}
	
	// データセットの始め
	void Begin(const string &name)
	{
		bool flag = true;
		int id = 0;
		
		if (name == "Mesh")                  id = ID_MESH;
		else if (name == "MeshMaterialList") id = ID_MESH_MATERIAL_LIST;
		else if (name == "Material")         id = ID_MATERIAL;
		else flag = false;
		
		if (flag)
			fout.write((const char *)&id, 4);
		
		flag_stack.push(flag);
	}
	
	// データセットの終わり
	void End()
	{
		if (flag_stack.top())
		{
			int id = ID_TEMPLATE_END;
			fout.write((const char *)&id, 4);
		}
		flag_stack.pop();
	}
	
	// 整数値を書き出す
	void WriteInt(int val)
	{
		if (flag_stack.top())
			fout.write((const char *)&val, 4);
	}
	
	// 実数値を書き出す
	// 実際の値の10000倍を整数として書き出す。
	void WriteReal(double val)
	{
		WriteInt((int)(val * 10000.0));
	}
};

////////////////////////////////////////////////////////////////////////////////
// 
// ファンクタ
// 
////////////////////////////////////////////////////////////////////////////////
struct match_begin
{
	MshWriter &writer;
	match_begin(MshWriter &w) : writer(w) {}
	void operator()(char const *first, char const *last) const
	{
		writer.Begin(string(first, last));
	}
};
struct match_end
{
	MshWriter &writer;
	match_end(MshWriter &w) : writer(w) {}
	void operator()(char const *first, char const *last) const
	{
		writer.End();
	}
};
struct match_int
{
	MshWriter &writer;
	match_int(MshWriter &w) : writer(w) {}
	void operator()(int val) const
	{
		writer.WriteInt(val);
	}
};
struct match_real
{
	MshWriter &writer;
	match_real(MshWriter &w) : writer(w) {}
	void operator()(double val) const
	{
		writer.WriteReal(val);
	}
};

////////////////////////////////////////////////////////////////////////////////
// 
// struct XSkipParser
// 
// @brief:  Xのスキップパーサ（コメント、空白文字）
// @update: 2011/10/08 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
struct XSkipParser : public grammar<XSkipParser>
{
	template<typename ScannarT>
	struct definition
	{
		rule<ScannarT> skip_p;
		
		// 構文定義
		definition(const XSkipParser &self)
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
// struct XParser
// 
// @brief:  Xの構文定義
// @update: 2011/10/08 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
struct XParser : public grammar<XParser>
{
	MshWriter &writer;
	XParser(MshWriter &w) : writer(w) {}

	template<typename ScannarT>
	struct definition
	{
		rule<ScannarT> keyword, uuid, ident, document, templ, member, opening, dataset;
	
		// 構文定義
		definition(const XParser &self)
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
				=  ident[match_begin(self.writer)]
				>> !ident
				>> '{'
				>> !uuid
				// strict_real_pを先に判定
				>> *(strict_real_p[match_real(self.writer)] | int_p[match_int(self.writer)] | ';' | ',' | dataset)
				>> '}'
				>> eps_p[match_end(self.writer)];

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
// function x2msh()
// 
// @brief:  Xからmshへの変換
// @update: 2011/10/08 by Odashi
// @args:
//     const char *filename: 出力先ファイル名
//     const char *data:     Xフォーマットを格納した文字列（終端'\0'）
//     int avail:            終端の'\0'を除く文字列長
// @ret:
//     void: 
// @note:
//     テキスト版Xのみ対応。
//     template構文などは無視します。
//     認識可能なテンプレートはMshWriterを参照。
// 
//------------------------------------------------------------------------------
void x2msh(const char *filename, const char *data, int avail)
{
	// 最初の16バイトはヘッダなので無視（サイズだけ確認しておく）
	if (avail < 16)
	{
		cerr << "Header mismatched." << endl;
		throw X2MshException();
	}
	
	// 書き出し器
	MshWriter writer(filename);
	writer.Header();
	
	// 文法
	XParser     g(writer);
	XSkipParser skip_p;
	
	// 解析の開始
	parse_info<> r = parse(data+16, g, skip_p);
	
	if (!r.hit)
	{
		cerr << "Parsing failed." << endl;
		throw X2MshException();
	}

	cerr << "Succeeded." << endl;
}
