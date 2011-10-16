// mqo2msh.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <boost/scoped_array.hpp>
#include "XParser.h"
#include "Mesh.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

using namespace std;

int main(int argc, char **argv)
{
	// �I�����Ƀq�[�v�̃`�F�b�N������
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	if (argc != 2 && argc != 3)
	{
		// ��������������
		cout << "USAGE: x2msh <input file> [<output file>]" << endl;
		return 1;
	}
	
	// �t�@�C����
	string infilename = argv[1];
	string outfilename;
	if (argc == 3)
		outfilename = argv[2];
	else
	{
		size_t dot_pos = infilename.find_last_of('.');
		if (dot_pos == string::npos)
			outfilename = infilename;
		else
			outfilename = infilename.substr(0, dot_pos);
		outfilename += ".msh";
	}
	
	cout << "Input : " << infilename << endl;
	cout << "Output: " << outfilename << endl;
	
	try
	{
		// �t�@�C�����J��
		ifstream fin(infilename.c_str(), ios::in | ios::binary);
		if (!fin.is_open())
			throw exception(("Opening \"" + infilename + "\" failed.").c_str());

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
		
		// X�t�@�C���̉��
		cout << "Parsing \"" << infilename << "\" ... ";
		XDataList data_list;
		XParser::Parse(buffer.get(), avail, data_list);
		cout << "Succeeded." << endl;
		
		// ���b�V�����̍\�z
		cout << "Restructuring ... ";
		Mesh mesh(data_list);
		cout << "Succeeded." << endl;
		
		// ���b�V�����̏����o��
		cout << "Writing \"" << outfilename << "\" ... ";
		mesh.WriteOut(outfilename.c_str());
		cout << "Succeeded." << endl;
	}
	catch (exception &e)
	{
		cout << endl;
		cerr << "**** ERROR **** " << e.what() << endl;
		return 1;
	}
	catch (...)
	{
		cout << endl;
		cerr << "**** ERROR **** Unknown error." << endl;
		return 1;
	}
	
	cout << "Converting completed successfully." << endl;
	
	return 0;
}
