// XParser.h
#pragma once
#include <vector>
#include <string>
#include <exception>

////////////////////////////////////////////////////////////////////////////////
// 
// struct XDataList
// 
// @brief:  Xファイルから抽出されたデータ
// @update: 2011/10/11 by Odashi
// @note:
//     Idに列挙されたテンプレートのみ格納する。
// 
////////////////////////////////////////////////////////////////////////////////
struct XDataList
{
	enum Id
	{
		// データセットの終わり（共通）
		ID_DATASET_END        = 0,
		
		// 各データセットの始まり
		ID_MESH               = 1,
		ID_MESH_MATERIAL_LIST = 2,
		ID_MESH_NORMALS       = 3,
		ID_MATERIAL           = 4,
	};

	std::vector<int>    id;
	std::vector<int>    int_val;
	std::vector<double> real_val;
};

////////////////////////////////////////////////////////////////////////////////
// 
// class XParser
// 
// @brief:  Xファイルの解析器
// @update: 2011/10/12 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
class XParser
{
	XParser();
	XParser(const XParser &);
	
public:
	// 例外クラス
	class Exception : public std::exception
	{
	public:
		Exception(const std::string &what)
		: std::exception(("XParser: " + what).c_str())
		{}
	};
	
	// 解析
	static void Parse(const char *data, size_t avail, XDataList &data_list);
};
