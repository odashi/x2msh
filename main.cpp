// mqo2msh.cpp
#include <fstream>
#include <iostream>
#include <boost/scoped_array.hpp>
#include "XParser.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

using namespace std;

int main(int argc, char **argv)
{
	// �I�����Ƀq�[�v�̃`�F�b�N������
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	if (argc != 3)
	{
		// ��������������
		cout << "USAGE: x2msh <input file> <output file>" << endl;
		return 1;
	}
	
	try
	{
		// �t�@�C�����J��
		ifstream fin(argv[1], ios::binary);

		// �t�@�C���T�C�Y���擾
		size_t avail;
		fin.peek(); // �����I�Ƀt�@�C�����J��
		fin.seekg(0, ios::end);
		avail = fin.tellg();
		fin.seekg(0, ios::beg);
		
		// ������o�b�t�@�ɓǂݍ���
		boost::scoped_array<char> buffer(new char[avail+1]);
		fin.read(buffer.get(), avail);
		buffer[avail] = '\0';
		
		// �ϊ�
		x2msh(argv[2], buffer.get(), avail);
	}
	catch (...)
	{
		// �G���[�����i�������ĂȂ��j
		cerr << "some error detected." << endl;
		return 1;
	}
	
	return 0;
}