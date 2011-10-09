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
	// 終了時にヒープのチェックをする
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	if (argc != 3)
	{
		// 引数がおかしい
		cout << "USAGE: x2msh <input file> <output file>" << endl;
		return 1;
	}
	
	try
	{
		// ファイルを開く
		ifstream fin(argv[1], ios::binary);

		// ファイルサイズを取得
		size_t avail;
		fin.peek(); // 強制的にファイルを開く
		fin.seekg(0, ios::end);
		avail = fin.tellg();
		fin.seekg(0, ios::beg);
		
		// 文字列バッファに読み込み
		boost::scoped_array<char> buffer(new char[avail+1]);
		fin.read(buffer.get(), avail);
		buffer[avail] = '\0';
		
		// 変換
		x2msh(argv[2], buffer.get(), avail);
	}
	catch (...)
	{
		// エラー処理（何もしてない）
		cerr << "some error detected." << endl;
		return 1;
	}
	
	return 0;
}