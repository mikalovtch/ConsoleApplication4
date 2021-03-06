// ConsoleApplication4.cpp: ���������� ����� ����� ��� ����������� ����������.
//
#include "stdafx.h"
#include <afx.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <omp.h>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <vector>
#include "shlwapi.h"
#include<algorithm>
#define _AFXDLL

using namespace std;

int maxfile;
int maxsize;
int checkstep;
struct inpout
{
	string input;
	string result;
};

omp_lock_t lock;
CFileFind finder;
WIN32_FIND_DATA fd;
SYSTEMTIME st;
bool flag;
string check_path = "..\\files\\checkpoint\\checkpoints.txt";

char * strToChar(string s)
{
	char *a = new char;
	strcpy(a, s.c_str());
	return a;
}

string charToString(char a[])
{
	stringstream s;
	s << a;
	string str = s.str();
	return str;
}

string wcharToString(WCHAR a[])
{
	char ch[260];
	char DefChar = ' ';
	WideCharToMultiByte(CP_ACP, 0, a, -1, ch, 260, &DefChar, NULL);
	string ss(ch);
	return ss;
}

FILETIME getLastWriteTime(HANDLE hFile)	//�������� ���� ���������� ��������� �����
{
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC;
	GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
	CloseHandle(hFile);
	// ������������� ����� ����������� � ��������� �����.
	FileTimeToSystemTime(&ftWrite, &stUTC);
	return ftWrite;
}

string getDir()
{
	WCHAR buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, sizeof(buffer) / sizeof(buffer[0]));
	PathRemoveFileSpec((LPWSTR)buffer);
	string a = wcharToString(buffer);
	return a;
}

HANDLE getHandle(string a)	//�������� ���������� �����
{
	return CreateFile(CA2CT(a.c_str()), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

void output(vector <string> &arr, string name)	//������� ������ � ����
{
	ofstream fout;
	fout.open(name, ios::out);
	if (fout.is_open()) {
		while (arr.size())
		{
			fout << arr.back() << endl;
			arr.pop_back();
		}
		fout.close();
	}
	else
	{
		cout << "�� ���� ������� ����: " << name << endl;
	}
}

void outputCheck(vector <string> &arr, string name)	//������� ������ � ����
{
	ofstream fout;
	fout.open(name, ios::out);
	if (fout.is_open()) {
		while (arr.size())
		{
			fout << arr.back() << endl;
			arr.pop_back();
		}
		fout.close();
	}
	else
	{
		cout << "�� ���� ������� ����: " << name << endl;
	}
}

string lastString(ifstream &in) //�������� ��������� ������ �����
{
	char str[100];
	in.getline(str, 100);
	if (!in.eof())
		lastString(in);
	in.close();
	return str;
}

bool compareDate(FILETIME f1, FILETIME  f2) //���������� ���� �������� ������
{
	if (CompareFileTime(&f1, &f2) > 0) {
		return true;
	}
	return false;
}

string Sha256(const string str)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, str.c_str(), str.size());
	SHA256_Final(hash, &sha256);
	stringstream ss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		ss << hex << setw(2) << setfill('0') << (int)hash[i];
	}
	return ss.str();
}

bool init(vector<string> &arr, vector<string> &checkpoints, string find) //������������� ��������
{
	omp_set_lock(&lock);
	vector<string> set;
	ifstream inpfile("..\\files\\settings\\settings.txt");
	if (inpfile.is_open())
	{
		while (!inpfile.eof())
		{
			string str;
			getline(inpfile, str);
			set.push_back(str);
		}
	}
	else {
		cout << "������ �������� ����� � �����������!" << endl << "��������� ����������!" << endl;
		omp_unset_lock(&lock);
		return false;
	}
	inpfile.close();
	inpfile.open("..\\files\\input.txt");
	if (inpfile.is_open())
	{
		while (!inpfile.eof())
		{
			string str;
			getline(inpfile, str);
			arr.push_back(str);
		}
		inpfile.close();
	}
	else {
		cout << "������ �������� ����� � ��������� ��������!" << endl << "��������� ����������!" << endl;
		omp_unset_lock(&lock);
		return false;
	}
	find = set.at(0);
	maxsize = stoi(set.at(1));
	maxfile = stoi(set.at(2));
	checkstep = stoi(set.at(3));
	inpfile.open("..\\files\\checkpoint\\checkpoints.txt");
	if (inpfile.is_open())
	{
		while (!inpfile.eof())
		{
			string str;
			getline(inpfile, str);
			checkpoints.push_back(str);
		}
		inpfile.close();
	}
	else {
		cout << "���� ���������� �� ������! �� ����� ������ �������������!" << endl;
	}
	omp_unset_lock(&lock);
	return true;
}

void out(vector<string> &arr, int num)
{
	int oldfile = 0;
	string file1 = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(0) + ".txt";
	fstream infile;
	infile.open(file1, ios::in);
	if (infile.is_open()) //�� ����������
	{
		int i;
		for (i = 1;i < maxfile;i++) {
			infile.close();
			file1 = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(i) + ".txt";
			infile.open(file1, ios::in);
			if (!infile.is_open()) //�� ����������
			{
				oldfile = i;
				break;
			}
			string file2 = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(i - 1) + ".txt";
			if (!compareDate(getLastWriteTime(getHandle(file1)), getLastWriteTime(getHandle(file2)))) //���� ���������
			{
				oldfile = i;
			}
		}
		file1 = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(maxfile - 1) + ".txt";
		infile.open(file1);
	}
	string name = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(oldfile) + ".txt";
	omp_set_lock(&lock);
	output(arr, name); //����� � ����
	omp_unset_lock(&lock);
}

void stop(vector<string> &checkpoints)
{
	flag = false;
	cout << "��������� ���������";
	outputCheck(checkpoints, check_path);
	system("Pause");
	exit(0);
}

bool work(string find, string val, vector<string> *checkpoints, int num,bool flag)		//��������� �����������
{
	vector <string> arr;
	int j = 0;
	while (val != find && !GetAsyncKeyState(VK_ESCAPE)&&flag)
	{
	/*	val = Sha256(val); //�������� ����� ���
		omp_set_lock(&lock);
	if (((std::find(checkpoints->begin(), checkpoints->end(), val)) != checkpoints->end()))
		{
			cout << "����� ����� " << num << " �������� ������!" << endl;
			checkpoints->push_back(val);
		outputCheck(*checkpoints, check_path);
			cout << "SHA256=" << val << endl;
			flag = false;
			system("Pause");
		}
		else {
			if (j > checkstep)
			{
				omp_set_lock(&lock);

			checkpoints->push_back(val);
				omp_unset_lock(&lock);
				j = 0;
			}
		}
		omp_unset_lock(&lock);
		j++;
		arr.push_back(val);
		if (arr.size() > maxsize)
		{
			out(arr, num);
			arr.clear();
		}*/
	}
	if (!GetAsyncKeyState(VK_ESCAPE)||!flag)
	{
		string name;
		name = "..\\files\\damps\\damp" + to_string(num) + "_last.txt";
		omp_set_lock(&lock);
		output(arr, name); //����� � ���� last
		arr.clear();
		string check_pat = "..\\files\\checkpoint\\checkpoints_"+to_string(omp_get_thread_num())+".txt";
		outputCheck(*checkpoints, check_pat);
		omp_unset_lock(&lock);
	}
}

void start()		//�������� �������� ���������� �����
{
	vector <string> a;
	vector<string> checkpoints;
	string fin;
	bool flag;

	{
		flag = true;
		omp_init_lock(&lock);
	if (init(a, checkpoints, fin))
	{
		cout << "��� ��������� ������� ���������" << endl;
		string name1;
		string val = "";
		int file;
		ifstream infile;
		int i;
#pragma omp parallel shared(a,fin,checkpoints,flag) //num_threads(a.size()-1) ������� ������
#pragma omp for  private(val,i,file) 
			for (int j = 0; j < a.size(); j++)
			{
				infile.close();
				file = 1;
				val = "";
			
				for (i = 1;i < maxfile;i++)
				{
					infile.close(); 
					name1 = "..\\files\\damps\\damp" + to_string(omp_get_thread_num()) + "_" + to_string(i) + ".txt";
					infile.open(name1); //Heap error
					if (!infile.is_open())
					{
						continue;
					}
					string name2 = "..\\files\\damps\\damp" + to_string(omp_get_thread_num()) + "_" + to_string(i - 1) + ".txt";
					if (compareDate(getLastWriteTime(getHandle(name1)), getLastWriteTime(getHandle(name2))))	//��������� ���� �����
					{
						file = i;
					}
				}
				if (file != 0) {
					infile.close();
					string name3 = "..\\files\\damps\\damp" + to_string(omp_get_thread_num()) + "_" + to_string(file) + ".txt";
					infile.open("..\\files\\damps\\damp" + to_string(omp_get_thread_num()) + "_" + to_string(file) + ".txt");
					if (infile.is_open())
					{
						val = lastString(infile); //+��������
						infile.close();
					}
				}
				if (val == "")
				{
					val = a.at(omp_get_thread_num());
				}
				work(fin, val, &checkpoints, omp_get_thread_num(),flag);
			}

		stop(checkpoints);
	}
	else {
		cout << "������ �������������!" << endl;
		system("Pause");
		exit(-1);
	}
	}
}

int main(int argc, char *argv[])	//�������� ���������
{
	setlocale(LC_ALL, "Russian");
	flag = true;
	start();
	system("Pause");
	return 0;
}

