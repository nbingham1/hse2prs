/*

 * common.h
 *
 * Common is a collection of functions not specific to the compiler that
 * we found useful to define. Note that our #defines for user flags are
 * also stored in common.h as it is accessed by everyone.
 */

#ifndef common_h
#define common_h

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <sstream>
#include <math.h>
#include <sys/time.h>

#include <vector>
#include <list>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <iterator>

using namespace std;

template <class type>
string to_string(type value)
{
	ostringstream os;
	os << value;
	return os.str();
}

bool ac(char c);
bool nc(char c);
bool oc(char c);
bool sc(char c);

int hex_to_int(string str);
int dec_to_int(string str);
int bin_to_int(string str);
string hex_to_bin(string str);
string dec_to_bin(string str);

unsigned int count_1bits(unsigned int x);
unsigned int count_0bits(unsigned int x);

int powi(int base, int exp);
int log2i(unsigned long long value);

uint32_t bitwise_or(uint32_t a, uint32_t b);
uint32_t bitwise_and(uint32_t a, uint32_t b);
uint32_t bitwise_not(uint32_t a);

string readfile(string filename);

string demorgan(string exp, int depth, bool invert);
string strip(string e);
vector<string> distribute(string exp);

size_t find_first_of_l0(string content, string search, size_t pos = 0);
size_t find_first_of_l0(string content, list<string> search, size_t pos = 0, list<string> exclude = list<string>());
size_t find_last_of_l0(string content, string search, size_t pos = 0);
size_t find_last_of_l0(string content, list<string> search, size_t pos = 0, list<string> exclude = list<string>());

template <typename type>
void vector_symmetric_compliment(vector<type> *v1, vector<type> *v2)
{
	vector<type> result;
	typename vector<type>::iterator i, j;
	for (i = v1->begin(), j = v2->begin(); i != v1->end() && j != v2->end();)
	{
		if (*j > *i)
			i++;
		else if (*i > *j)
			j++;
		else
		{
			i = v1->erase(i);
			j = v2->erase(j);
		}
	}
}

template <typename type>
ostream &operator<<(ostream &os, vector<type> s)
{
	os << "{";
	for (size_t i = 0; i < s.size(); i++)
	{
		if (i != 0)
			os << ", ";
		os << s[i];
	}
	os << "}";
	return os;
}

template <typename type>
ostream &operator<<(ostream &os, list<type> s)
{
	os << "{";
	for (typename list<type>::iterator i = s.begin(); i != s.end(); i++)
	{
		if (i != s.begin())
			os << ", ";
		os << *i;
	}
	os << "}";
	return os;
}

template <typename type1, typename type2>
ostream &operator<<(ostream &os, map<type1, type2> s)
{
	os << "{";
	for (typename map<type1, type2>::iterator i = s.begin(); i != s.end(); i++)
	{
		if (i != s.begin())
			os << ", ";
		os << i->first << "=>" << i->second;
	}
	os << "}";
	return os;
}

template <typename type1, typename type2>
ostream &operator<<(ostream &os, pair<type1, type2> s)
{
	os << "(" << s.first << ", " << s.second << ")";
	return os;
}


#endif
