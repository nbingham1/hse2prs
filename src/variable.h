/*
 * variable.h
 *
 * This structure describes a variable in the chp program. We need
 * to keep track of a variables name, its type (record, keyword, process),
 * and its bit width.
 *
 */

#include "common.h"

#ifndef variable_h
#define variable_h

struct variable_space;

struct variable
{
	variable();
	variable(string name);
	~variable();

	string		name;		// the name of the instantiated variable
	bool		written;	// keep track of whether or not this variable is driven within this process
	bool		read;

	string inverse_name();
	variable &operator=(variable v);
};

ostream &operator<<(ostream &os, const variable &v);

#endif
