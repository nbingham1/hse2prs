/*
 * variable.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "variable.h"
#include "variable_space.h"

variable::variable()
{
	name = "";
	written = false;
	read = false;
}

variable::variable(string name)
{
	this->name = name;
	this->written = false;
	this->read = false;
}

variable::~variable()
{
}

string variable::inverse_name()
{
	int idx = name.find_last_of(".");

	if (name[idx + 1] == '_')
		return name.substr(0, idx+1) + name.substr(idx+2);
	else
		return name.substr(0, idx+1) + "_" + name.substr(idx+1);
}

variable &variable::operator=(variable v)
{
	name = v.name;
	written = v.written;
	read = v.read;
	return *this;
}

ostream &operator<<(ostream &os, const variable &v)
{
	os << v.name << " "  << (v.written ? "driven " : "") << (v.read ? "read" : "");

    return os;
}

