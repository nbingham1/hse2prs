/*
 * variable_space.cpp
 *
 *  Created on: Feb 9, 2013
 *      Author: nbingham
 */

#include "variable_space.h"

variable_space::variable_space()
{
}

variable_space::~variable_space()
{
}

size_t variable_space::find(string name)
{
	for (size_t i = 0; i < globals.size(); i++)
		if (globals[i].name == name || globals[i].name == "this." + name)
			return i;

	return globals.size();
}

bool variable_space::contains(string name)
{
	return (find(name) != globals.size());
}

string variable_space::closest(string name)
{
	int dist = 999999999;
	string result = "";
	size_t count = 0;

	for (size_t i = 0; i < globals.size(); i++)
	{
		int temp = 0;
		size_t j = 0;
		for (j = 0; j < name.size() && j < globals[i].name.size(); j++)
			if (name[j] != globals[i].name[j])
				temp += name.size() - j;

		for (; j < globals[i].name.size(); j++)
			temp += j - name.size();

		for (; j < name.size(); j++)
			temp += name.size() - j;

		if (temp < dist)
		{
			result = "'" + globals[i].name + "'";
			dist = temp;
			count = 1;
		}
		else if (temp == dist)
		{
			if (count == 1)
				result = "or " + result;
			result = "'" + globals[i].name + "' ";
			count++;
		}
	}

	return result;
}

variable_space &variable_space::operator=(variable_space s)
{
	this->globals = s.globals;
	return *this;
}

variable &variable_space::at(size_t i)
{
	if (i < globals.size())
		return globals[i];
	else
	{
		size_t idx = find("null");
		if (idx == globals.size())
			globals.push_back(variable("null"));
		return globals[idx];
	}
}

string variable_space::variable_list()
{
	string result;
	for (size_t i = 0; i < globals.size(); i++)
	{
		if (i != 0)
			result += ",";
		result += globals[i].name;
	}
	return result;
}

string variable_space::unique_name()
{
	size_t i = 0;
	while (contains("_sv" + to_string(i)))
		i++;

	return "_sv" + to_string(i);
}

ostream &operator<<(ostream &os, const variable_space &s)
{
	for (size_t i = 0; i < s.globals.size(); i++)
		os << i << " " << s.globals[i] << endl;

	return os;
}
