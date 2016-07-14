// ConsoleApplication4.cpp: определяет точку входа для консольного приложения.
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
vector<string> checkpoints;
omp_lock_t lock;
CFileFind finder;
WIN32_FIND_DATA fd;
SYSTEMTIME st;

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

FILETIME getLastWriteTime(HANDLE hFile)	//Получаем дату последнего изменения файла
{
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC, stLocal;
	GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
	CloseHandle(hFile);
	// преобразовать время модификации в локальное время.
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

HANDLE getHandle(string a)	//Получаем дескриптор файла
{
	return CreateFile(CA2CT(a.c_str()), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

void output(vector <string> &arr, string name)	//Выводим вектор в файл
{
	ofstream fout;
	fout.open(name, ios::out);
	if (fout.is_open()) {
	//while (arr.size())
	//	{
			fout << arr.back() << endl;
			arr.pop_back();
	//	}
		fout.close();
	}
	else
	{
		cout << "Не могу открыть файл: " << name << endl;
	}
}

string lastString(ifstream &in) //Получаем последнюю строку файла
{
	char str[100];
	in.getline(str, 100);
	if (!in.eof())
		lastString(in);
	in.close();
	return str;
}

bool compareDate(FILETIME f1, FILETIME  f2) //Сравниваем даты создания файлов
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

void init(vector<string> &arr, string find) //Инициализация настроек
{
	vector<string> set;
	ifstream inpfile("..\\files\\settings\\settings.txt");
	if (inpfile.is_open())
	{
		string str;
		while (!inpfile.eof())
		{
			getline(inpfile, str);
			set.push_back(str);
		}
		inpfile.close();
	}
	else {
		cout << "Ошибка открытия файла с настройками!" << endl << "Требуется перезапуск!" << endl;
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
		cout << "Ошибка открытия файла с исходными строками!" << endl << "Требуется перезапуск!" << endl;
	}
	find = set.at(0).substr((0, set.at(0).find_first_of(" //")));
	maxsize = stoi(set.at(1));
	maxfile = stoi(set.at(2));
	checkstep = stoi(set.at(3));
}

void out(vector<string> &arr, int num)
{
	int oldfile = 0;
	string file1 = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(0) + ".txt";
	fstream infile;
	infile.open(file1, ios::in);
	if (infile.is_open()) //Не сущестувет
	{
		int i;
		for (i = 1;i < maxfile;i++) {
			infile.close();
			file1 = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(i) + ".txt";
			infile.open(file1, ios::in);
			if (!infile.is_open()) //Не сущестувет
			{
				oldfile = i;
				break;
			}
			string file2 = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(i - 1) + ".txt";
			if (!compareDate(getLastWriteTime(getHandle(file1)), getLastWriteTime(getHandle(file2)))) //Дата изменения
			{
				oldfile = i;
			}
		}
		file1 = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(maxfile - 1) + ".txt";
		infile.open(file1);
	}
	string name = "..\\files\\damps\\damp" + to_string(num) + "_" + to_string(oldfile) + ".txt";
	//omp_set_lock(&lock);
	output(arr, name); //Вывод в файл
//	omp_unset_lock(&lock);
}

void work(string find, string val, int num)		//Выполняем хеширование
{
	vector <string> arr;

	std::vector<string>::iterator a;
	int j = 0;
	while (val != find)
	{
		val = Sha256(val); //Получаем новый хеш
		if (((std::find(checkpoints.begin(),checkpoints.end(), val))!=checkpoints.end()))
		{
			omp_set_lock(&lock);
			cout << "Поток номер" << num << "закончил работу!" << endl;
			cout << "SHA256=" << val;
			omp_unset_lock(&lock);
			exit(-1);
		}
		else {
			if (j > checkstep)
			{
				omp_set_lock(&lock);
				checkpoints.push_back(val);
				omp_unset_lock(&lock);
				j = 0;
			}
		}
		j++;
		arr.push_back(val);
		if (arr.size() > maxsize)
		{
			out(arr, num);
			j = 0;
			arr.clear();
		}
	}
	string name;
	name = "..\\files\\damps\\damp" + to_string(num) + "_last.txt";
//	omp_set_lock(&lock);
	output(arr, name); //Вывод в файл last
	arr.clear();
//	omp_unset_lock(&lock);
}


void start()		//Проводим действия начального этапа
{
	vector <string> a;
	string fin;
	init(a, fin);
	string name1;
	string val = "";
	int file;
	ifstream infile;
	int i;
#pragma omp parallel  private(val,infile,i,file)//shared(checkpoints)  //Создаем потоки
	{
		file = 1;
		val = "";
		omp_init_lock(&lock);
#pragma omp for 
		for (int j = 0; j < a.size(); j++)
		{
			int num = omp_get_thread_num();
			for (i = 1;i < maxfile;i++)
			{
				name1 = "..\\files\\damps\\damp" + to_string(omp_get_thread_num()) + "_" + to_string(i) + ".txt";
				infile.open(name1);
				if (!infile.is_open())
				{
					continue;
				}
				string name2 = "..\\files\\damps\\damp" + to_string(omp_get_thread_num()) + "_" + to_string(i - 1) + ".txt";
				if (compareDate(getLastWriteTime(getHandle(name1)), getLastWriteTime(getHandle(name2))))	//Сравнение даты файла
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
					val = lastString(infile); //+работает
					infile.close();
				}
			}
			num = omp_get_thread_num();
			if (val == "")
			{
				val = a.at(num);
			}
			work(fin, val, num);
		}
	}
}
void stop()
{
	cout << "СТоп";
}
int main(int argc, char *argv[])	//Основная программа
{
	setlocale(LC_ALL, "Russian");
	start();
	stop();
	system("Pause");
	return 0;
}

