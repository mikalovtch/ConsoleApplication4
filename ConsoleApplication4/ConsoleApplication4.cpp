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
#include <list>
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

FILETIME getLastWriteTime(HANDLE hFile)	//Получаем дату последнего изменения файла
{
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC;
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
		while (arr.size() > 0)
		{
			fout << arr.back() << endl;
			arr.pop_back();
		}
		fout.close();
	}
	else
	{
		cout << "Не могу открыть файл: " << name << endl;
	}
}

void outputCheck(vector <string> &arr, string name)	//Выводим вектор в файл
{
	ofstream fout;
	fout.open(name, ios::out);
	if (fout.is_open()) {
		while (arr->size())
		{
			fout << arr->back() << endl;
			arr->pop_back();
		}
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

bool init(vector<string> &arr, vector<string> &checkpoints, string find) //Инициализация настроек
bool init(vector<string> &arr, list<string> *&checkpoints, string find)
{
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
		cout << "Ошибка открытия файла с настройками!" << endl << "Требуется перезапуск!" << endl;
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
		cout << "Ошибка открытия файла с исходными строками!" << endl << "Требуется перезапуск!" << endl;
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
			checkpoints->push_back(str);
		}
		inpfile.close();
	}
	else {
		cout << "Файл чекпоинтов не найден! Он будет создан автоматически!" << endl;
	}
	return true;
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
	omp_set_lock(&lock);
	output(arr, name); //Вывод в файл
	omp_unset_lock(&lock);
}

//void stop(vector<string> *checkpoints)
void stop(list<string> *checkpoints)
{
	flag = false;
	cout << "Остановка программы";
	outputCheck(checkpoints, check_path);
	system("Pause");
	exit(0);
}

//void work(string find, string val, vector<string> *checkpoints, int num)		//Выполняем хеширование
void work(string find, string val, int num)	
{
	vector <string> arr;
	int j = 0;
	while (val != find && !GetAsyncKeyState(VK_ESCAPE))
	{
		val = Sha256(val); //Получаем новый хеш
		if (((std::find(checkpoints->begin(), checkpoints->end(), val)) != checkpoints->end()))
		{
			cout << "Поток номер " << num << " закончил работу!" << endl;
			checkpoints->push_back(val);
			outputCheck(checkpoints, check_path);
			cout << "SHA256=" << val << endl;
			system("Pause");
			exit(0);
		}
		else {
			if (j > checkstep)
			{
				checkpoints->push_back(val);
				j = 0;
			}
		}
		j++;
		arr.push_back(val);
		if (arr.size() > maxsize)
		{
			out(arr, num);
			arr.clear();
		}
	}
	if (!GetAsyncKeyState(VK_ESCAPE))
	{
		string name;
		name = "..\\files\\damps\\damp" + to_string(num) + "_last.txt";
		omp_set_lock(&lock);
		output(arr, name); //Вывод в файл last
		arr.clear();
		omp_unset_lock(&lock);
	}
}

void start()		//Проводим действия начального этапа
{
	vector <string> a;
	//vector<string> checkpoints;
	list<string> *checkpoints1 = new list<string>;
	string fin;
	if (init(a, checkpoints1, fin))
	{
		cout << "Все параметры успешно загружены" << endl;
		string name1;
		string val = "";
		int file;
		ifstream infile;
		int i;
#pragma omp parallel shared(checkpoints) num_threads(a.size())// Создаем потоки
		{
#pragma omp for  private(val,infile,i,file) 
			for (int j = 0; j < a.size(); j++)
			{
				file = 1;
				val = "";
				omp_init_lock(&lock);
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
				if (val == "")
				{
					val = a.at(omp_get_thread_num());
				}
				work(fin, val, omp_get_thread_num());
			}
		}
		stop(checkpoints);
	}
	else {
		cout << "Ошибка инициализации!" << endl;
		system("Pause");
		exit(-1);
	}
}

int main(int argc, char *argv[])	//Основная программа
{
	setlocale(LC_ALL, "Russian");
	flag = true;
	start();
	system("Pause");
	return 0;
}

