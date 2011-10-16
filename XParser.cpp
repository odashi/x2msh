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
// @brief:  XDataList�̏����o����
// @update: 2011/10/11 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
class XDataListFactory
{
	XDataList &data_list_;
	stack<bool> flag_stack_;
	
	XDataListFactory(const XDataListFactory &);
	
	// �e���v���[�g�̎�ނ�ID�ɕϊ�
	short ToId(const string &name)
	{
		if (name == "Mesh")             return XDataList::ID_MESH;
		if (name == "MeshMaterialList") return XDataList::ID_MESH_MATERIAL_LIST;
		if (name == "MeshNormals")      return XDataList::ID_MESH_NORMALS;
		if (name == "Material")         return XDataList::ID_MATERIAL;
		
		// �m��Ȃ��e���v���[�g
		return -1;
	}
	
public:
	XDataListFactory(XDataList &data_list)
	: data_list_(data_list)
	{}
	
	// �f�[�^�Z�b�g�̎n��
	void Begin(const string &name)
	{
		short id;
		
		if (flag_stack_.empty())
			// �X�^�b�N����Ȃ�V�����f�[�^�Z�b�g�͗L��
			id = ToId(name);
		else if (flag_stack_.top())
			// �e�f�[�^�Z�b�g���L���Ȃ�V�����f�[�^�Z�b�g���L��
			id = ToId(name);
		else
			// �e�f�[�^�Z�b�g�������Ȃ̂ŐV�����f�[�^�Z�b�g�͋����I�ɖ���
			id = -1;
		
		if (id != -1)
		{
			// �f�[�^�Z�b�g�͗L��
			data_list_.id.push_back(id);
			flag_stack_.push(true);
		}
		else
			// �f�[�^�Z�b�g�͖���
			flag_stack_.push(false);
	}
	
	// �f�[�^�Z�b�g�̏I���
	void End()
	{
		if (flag_stack_.top())
			data_list_.id.push_back(XDataList::ID_DATASET_END);
		
		flag_stack_.pop();
	}
	
	// �����l��ǉ�
	void AddInt(short val)
	{
		// ���݂̃f�[�^�Z�b�g���L���Ȃ�ǉ�
		if (flag_stack_.top())
			data_list_.int_val.push_back(val);
	}
	
	// �����l��ǉ�
	void AddReal(double val)
	{
		// ���݂̃f�[�^�Z�b�g���L���Ȃ�ǉ�
		if (flag_stack_.top())
			data_list_.real_val.push_back(val);
	}
};

////////////////////////////////////////////////////////////////////////////////
// 
// �t�@���N�^
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
// @brief:  X�̃X�L�b�v�p�[�T�i�R�����g�A�󔒕����j
// @update: 2011/10/12 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
struct XSkip : public grammar<XSkip>
{
	template<typename ScannarT>
	struct definition
	{
		rule<ScannarT> skip_p;
		
		// �\����`
		definition(const XSkip &self)
		{
			skip_p = +space_p | comment_p("//") | comment_p('#');
			
			// �f�o�b�O�o�́i�o�͂��Ȃ��j
			BOOST_SPIRIT_DEBUG_TRACE_NODE(self, false);
			BOOST_SPIRIT_DEBUG_TRACE_NODE(skip_p, false);
		};
		
		// �J�n�L��
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
// @brief:  X�̍\����`
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
	
		// �\����`
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
				// strict_real_p���ɔ���
				>> *(strict_real_p[add_real(self.factory_)] | int_p[add_int(self.factory_)] | ';' | ',' | dataset)
				>> '}'
				>> eps_p[add_end(self.factory_)];

			// �f�o�b�O�o��
			BOOST_SPIRIT_DEBUG_RULE(keyword);
			BOOST_SPIRIT_DEBUG_RULE(uuid);
			BOOST_SPIRIT_DEBUG_RULE(ident);
			BOOST_SPIRIT_DEBUG_RULE(document);
			BOOST_SPIRIT_DEBUG_RULE(templ);
			BOOST_SPIRIT_DEBUG_RULE(member);
			BOOST_SPIRIT_DEBUG_RULE(opening);
			BOOST_SPIRIT_DEBUG_RULE(dataset);
		}
		
		// �J�n�L��
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
// @brief:  X�t�@�C������XDataList�𐶐�
// @update: 2011/10/12 by Odashi
// @args:
//     const char *data:     X�t�H�[�}�b�g���i�[����'\0'�ŏI��镶����
//     size_t avail:         '\0'���܂܂Ȃ�������̒���
//     XDataList &data_list: �f�[�^�̊i�[��
// @ret:
//     void: 
// @note:
//     �e�L�X�g��X�̂ݑΉ��B
//     template�\���Ȃǂ͖������܂��B
// 
//------------------------------------------------------------------------------
void XParser::Parse(const char *data, size_t avail, XDataList &data_list)
{
	// �ŏ���16�o�C�g�̓w�b�_�Ȃ̂Ŗ����i�T�C�Y�����m�F���Ă����j
	if (avail < 16)
		throw Exception("Header mismatched.");
	
	// �����o����
	XDataListFactory factory(data_list);
	
	// ���@
	XGrammar g(factory);
	XSkip    skip_p;
	
	// ��͂̊J�n
	parse_info<> r = parse(data+16, g, skip_p);
	
	if (!r.hit)
		throw Exception("Unsupported format.");
}
