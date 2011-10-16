// XParser.h
#pragma once
#include <vector>
#include <string>
#include <exception>

////////////////////////////////////////////////////////////////////////////////
// 
// struct XDataList
// 
// @brief:  X�t�@�C�����璊�o���ꂽ�f�[�^
// @update: 2011/10/11 by Odashi
// @note:
//     Id�ɗ񋓂��ꂽ�e���v���[�g�̂݊i�[����B
// 
////////////////////////////////////////////////////////////////////////////////
struct XDataList
{
	enum Id
	{
		// �f�[�^�Z�b�g�̏I���i���ʁj
		ID_DATASET_END        = 0,
		
		// �e�f�[�^�Z�b�g�̎n�܂�
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
// @brief:  X�t�@�C���̉�͊�
// @update: 2011/10/12 by Odashi
// 
////////////////////////////////////////////////////////////////////////////////
class XParser
{
	XParser();
	XParser(const XParser &);
	
public:
	// ��O�N���X
	class Exception : public std::exception
	{
	public:
		Exception(const std::string &what)
		: std::exception(("XParser: " + what).c_str())
		{}
	};
	
	// ���
	static void Parse(const char *data, size_t avail, XDataList &data_list);
};
