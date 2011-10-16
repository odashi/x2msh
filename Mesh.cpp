// Mesh.cpp
#include <fstream>
#include <algorithm>
#include <boost/scoped_array.hpp>
#include "Mesh.h"

using namespace std;

namespace
{
	// �ʏ����\�[�g���邽�߂̔�r�֐�
	bool compFace(const Face &left, const Face &right)
	{
		return left.material < right.material;
	}

	// bool�l��1�o�C�g�ŏ����o��
	inline void writeBool(ofstream &fout, bool val)
	{
		char n = val ? 1 : 0;
		fout.write(&n, 1);
	}
	
	// 2�o�C�g�����t�������������o��
	inline void writeInt(ofstream &fout, int val)
	{
		short n = (short)val;
		fout.write((const char *)&n, 2);
	}
	
	// ���������_����65536�{��4�o�C�g�����t�������ŏ����o��
	inline void writeReal(ofstream &fout, double val)
	{
		int n = (int)(val * 65536.0);
		fout.write((const char *)&n, 4);
	}
	
	// �}�e���A�����������o��
	void writeMaterial(ofstream &fout, Material &mat)
	{
		writeReal(fout, mat.face_color.r);
		writeReal(fout, mat.face_color.g);
		writeReal(fout, mat.face_color.b);
		writeReal(fout, mat.face_color.a);
		writeReal(fout, mat.power);
		writeReal(fout, mat.specular_color.r);
		writeReal(fout, mat.specular_color.g);
		writeReal(fout, mat.specular_color.b);
		writeReal(fout, mat.emissive_color.r);
		writeReal(fout, mat.emissive_color.g);
		writeReal(fout, mat.emissive_color.b);
	}
	
	// �x�N�g�����������o��
	void writeVector3(ofstream &fout, Vector3 &v)
	{
		writeReal(fout, v.x);
		writeReal(fout, v.y);
		writeReal(fout, v.z);
	}
	
	// ���_���������o��
	void writeVertices(ofstream &fout, vector<Vector3> &vertex, Face &f)
	{
		writeVector3(fout, vertex[f.vertex[0]]);
		writeVector3(fout, vertex[f.vertex[1]]);
		writeVector3(fout, vertex[f.vertex[2]]);

		if (f.vertex_num == 4)
		{
			writeVector3(fout, vertex[f.vertex[2]]);
			writeVector3(fout, vertex[f.vertex[3]]);
			writeVector3(fout, vertex[f.vertex[0]]);
		}
	}
	
	// �@�����������o��
	void writeNormals(ofstream &fout, vector<Vector3> &normal, Face &f)
	{
		writeVector3(fout, normal[f.normal[0]]);
		writeVector3(fout, normal[f.normal[1]]);
		writeVector3(fout, normal[f.normal[2]]);

		if (f.vertex_num == 4)
		{
			writeVector3(fout, normal[f.normal[2]]);
			writeVector3(fout, normal[f.normal[3]]);
			writeVector3(fout, normal[f.normal[0]]);
		}
	}
}

Mesh::Mesh(const XDataList &data_list)
try
: has_normal(false)
{
	int id_it = 0;
	int int_it = 0;
	int real_it = 0;
	
	if (data_list.id.at(id_it++) != XDataList::ID_MESH)
			throw Exception("Invalid format.");
	
	// ���_���̓ǂݍ���
	int vertex_num = data_list.int_val.at(int_it++);
	for (int i = 0; i < vertex_num; i++)
	{
		Vector3 v;
		v.x = data_list.real_val.at(real_it++);
		v.y = data_list.real_val.at(real_it++);
		v.z = data_list.real_val.at(real_it++);
		vertex.push_back(v);
	}
	
	// �ʏ��̓ǂݍ���
	int face_num = data_list.int_val.at(int_it++);
	for (int i = 0; i < face_num; i++)
	{
		Face f;
		f.vertex_num = data_list.int_val.at(int_it++);
		if (f.vertex_num != 3 && f.vertex_num != 4)
			throw Exception("Invalid format.");
		
		for (int j = 0; j < f.vertex_num; j++)
			f.vertex[j] = data_list.int_val.at(int_it++);
		
		face.push_back(f);
	}
	
	int id;
	while ((id = data_list.id.at(id_it++)) != XDataList::ID_DATASET_END)
	{
		if (id == XDataList::ID_MESH_MATERIAL_LIST)
		{
			// �}�e���A�����̓ǂݍ���
			int material_num = data_list.int_val.at(int_it++);
			if (data_list.int_val.at(int_it++) != face_num)
				throw Exception("Invalid format.");
			for (int i = 0; i < face_num; i++)
				face[i].material = data_list.int_val.at(int_it++);
			
			for (int i = 0; i < material_num; i++)
			{
				if (data_list.id.at(id_it++) != XDataList::ID_MATERIAL)
					throw Exception("Invalid format.");
				Material m;
				m.face_color.r = data_list.real_val.at(real_it++);
				m.face_color.g = data_list.real_val.at(real_it++);
				m.face_color.b = data_list.real_val.at(real_it++);
				m.face_color.a = data_list.real_val.at(real_it++);
				m.power = data_list.real_val.at(real_it++);
				m.specular_color.r = data_list.real_val.at(real_it++);
				m.specular_color.g = data_list.real_val.at(real_it++);
				m.specular_color.b = data_list.real_val.at(real_it++);
				m.emissive_color.r = data_list.real_val.at(real_it++);
				m.emissive_color.g = data_list.real_val.at(real_it++);
				m.emissive_color.b = data_list.real_val.at(real_it++);
				material.push_back(m);
				if (data_list.id.at(id_it++) != XDataList::ID_DATASET_END)
					throw Exception("Invalid format.");
			}
			
			if (data_list.id.at(id_it++) != XDataList::ID_DATASET_END)
				throw Exception("Invalid format.");
		}
		else if (id == XDataList::ID_MESH_NORMALS)
		{
			// �@�����̓ǂݍ���
			has_normal = true;
			
			int normal_num = data_list.int_val.at(int_it++);
			for (int i = 0; i < normal_num; i++)
			{
				Vector3 n;
				n.x = data_list.real_val.at(real_it++);
				n.y = data_list.real_val.at(real_it++);
				n.z = data_list.real_val.at(real_it++);
				normal.push_back(n);
			}
			
			if (data_list.int_val.at(int_it++) != face_num)
				throw Exception("Invalid format.");
			
			for (int i = 0; i < face_num; i++)
			{
				if (data_list.int_val.at(int_it++) != face[i].vertex_num)
					throw Exception("Invalid format.");
				for (int j = 0; j < face[i].vertex_num; j++)
					face[i].normal[j] = data_list.int_val.at(int_it++);
			}
			
			if (data_list.id.at(id_it++) != XDataList::ID_DATASET_END)
				throw Exception("Invalid format.");
		}
	}
	
	// �}�e���A���ԍ��Ŗʂ��\�[�g����
	sort(face.begin(), face.end(), compFace);
}
catch (...)
{
}

void Mesh::WriteOut(const char *filename)
{
	ofstream fout(filename, ios::out | ios::binary);
	
	// �V�O�l�`��
	fout.write("MSH ", 4);
	
	// �e����
	writeBool(fout, !material.empty());
	writeBool(fout, has_normal);
	
	if (!material.empty())
	{
		// �ގ�����
		// �ގ����ƍގ����Ƃ̎O�p�`�̖ʐ��������o��
		boost::scoped_array<int> face_num(new int[material.size()]);
		for (size_t i = 0; i < material.size(); i++)
			face_num[i] = 0;
		for (size_t i = 0; i < face.size(); i++)
			face_num[face[i].material] += face[i].vertex_num - 2;
		
		writeInt(fout, material.size());
		for (size_t i = 0; i < material.size(); i++)
			writeInt(fout, face_num[i]);
	}
	else
	{
		// �ގ��Ȃ�
		// �O�p�`�S���̖ʐ��������o��
		int face_num = 0;
		for (size_t i = 0; i < face.size(); i++)
			face_num += face[i].vertex_num - 2;
		writeInt(fout, face_num);
	}
	
	// �ʂ������o��
	for (size_t i = 0; i < face.size(); i++)
		writeVertices(fout, vertex, face[i]);
	
	// �ގ��������o��
	for (size_t i = 0; i < material.size(); i++)
		writeMaterial(fout, material[i]);
	
	// �@���������o��
	if (has_normal)
		for (size_t i = 0; i < face.size(); i++)
			writeNormals(fout, normal, face[i]);
}
