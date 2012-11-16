/*
 * record.h
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham
 */

#include "common.h"
#include "keyword.h"
#include "variable.h"

#ifndef record_h
#define record_h

/* This structure represents a structure or record. A record
 * contains a bunch of member variables that help you index
 * segments of bits within the multibit signal.
 */
struct record : keyword
{
	record();
	record(string chp, map<string, keyword*> types, string tab);
	~record();

	map<string, variable*> vars;	// the list of member variables that make up this record

	record &operator=(record r);
	void parse(string chp, map<string, keyword*> types, string tab);
};

map<string, variable*> expand(string chp, string super, map<string, keyword*> types, string tab);
ostream &operator<<(ostream &os, record s);

#endif
