/*
 * canonical.cpp
 *
 *  Created on: May 11, 2013
 *      Author: nbingham
 */

#include "canonical.h"
#include "variable_space.h"

canonical::canonical()
{
	terms = vector<minterm>();
}

canonical::canonical(const canonical &c)
{
	terms = c.terms;
}

canonical::canonical(int s)
{
	terms.resize(s);
}

/**
 * \brief	Constructor that adds the minterm m to the canonical
 * \param	m	The one minterm to add.
 */
canonical::canonical(minterm m)
{
	terms.push_back(m);
}

/**
 * \brief	Constructor that adds all minterms in m to the canonical.
 * \param	m	All minterms to add.
 */
canonical::canonical(vector<minterm> m)
{
	terms = m;
	mccluskey();
}

/**
 * \brief	A constructor that parses a string and uses it to fill the canonical expression with values.
 * \param	s		A string that represents the expression.
 * \param	vars	The variable space used to parse the string.
 */
canonical::canonical(tokenizer &tokens, string s, variable_space &vars)
{
	if (s == "0")
		return;

	vector<string> t = distribute(demorgan(s, -1, false));
	for (size_t i = 0; i < t.size(); i++)
	{
		minterm m(tokens, t[i], vars);
		if (m != 0)
			terms.push_back(m);
	}
	mccluskey();
}

canonical::canonical(int var, uint32_t val)
{
	terms.push_back(minterm(var, val));
}

canonical::canonical(map<int, uint32_t> vals)
{
	terms.push_back(minterm(vals));
}

canonical::~canonical()
{
	terms.clear();
}

/**
 * \brief	Returns an iterator that represents the beginning of the list of minterms.
 */
vector<minterm>::iterator canonical::begin()
{
	return terms.begin();
}

/**
 * \brief	Returns an iterator that represents the end of the list of minterms.
 */
vector<minterm>::iterator canonical::end()
{
	return terms.end();
}

/**
 * \brief	Returns the total number of minterms in this canonical expression.
 */
int canonical::size()
{
	return terms.size();
}

/**
 * \brief	Sets the term located at index i in this canonical expression equal to t.
 * \param	i	The index of the term to assign.
 * \param	t	The new minterm to use.
 */
void canonical::assign(size_t i, minterm t)
{
	if (i >= terms.size())
		terms.resize(i+1, minterm());
	terms[i] = t;
}

/**
 * \brief	Adds another minterm to this canonical expression.
 * \details Specifically it represents the operation this = this | m.
 * \param	m	The minterm to add.
 */
void canonical::push_back(minterm m)
{
	terms.push_back(m);
}

/**
 * \brief	Adds a variable to every minterm in this canonical expression.
 * \param	m	The relation of the added variable to each minterm in the canonical.
 */
void canonical::push_up(minterm m)
{
	if (terms.size() < m.size)
		terms.resize(m.size);

	for (size_t i = 0; i < terms.size(); i++)
		terms[i].push_back(m.get(i));
}

/**
 * \brief	Removes a minterm from this canonical.
 * \param	i	The index of the minterm to remove.
 */
void canonical::remove(size_t i)
{
	terms.erase(terms.begin() + i);
}

/**
 * \brief	Remove all minterms from this canonical, effectively setting this expression to be 0.
 */
void canonical::clear()
{
	terms.clear();
}

/**
 * \brief	Executes the Quine-McCluskey algorithm, also known as the method of prime implicants.
 * \details	a method used for minimization of boolean functions which was developed by W.V. Quine
 * 			and Edward J. McCluskey in 1956. It is functionally identical to Karnaugh mapping, but
 * 			the tabular form makes it more efficient for use in computer algorithms, and it also
 * 			gives a deterministic way to check that the minimal form of a Boolean function has been
 * 			reached. It is sometimes referred to as the tabulation method. The method involves two steps:
 */
void canonical::mccluskey()
{
	canonical temp;
	vector<minterm> implicants;
	vector<minterm> primes;
	vector<int> essentials;
	vector<minterm> t[2] = {vector<minterm>(), vector<minterm>()};
	minterm implicant;

	vector<int> count;
	int count_sum;
	vector<int> vl;

	map<int, vector<int> > cov, Tcov;
	vector<int>::iterator ci;

	pair<int, int> xdiff;
	int diff;

	for (size_t i = 0; i < terms.size(); i++)
		if (terms[i] != 0)
			implicants.push_back(terms[i]);

	t[1] = implicants;
	terms.clear();

	/**
	 * 1.	Find all prime implicants of the expression.
	 */
	count_sum = t[1].size();
	while (count_sum > 0)
	{
		t[0].clear();
		count.clear();
		count.resize(t[1].size(), 0);
		for (size_t i = 0; i < t[1].size(); i++)
		{
			for (size_t j = i+1; j < t[1].size(); j++)
			{
				xdiff = t[1][i].xdiff_count(t[1][j]);
				diff = t[1][i].diff_count(t[1][j]);
				if ((diff <= 1 && xdiff.first + xdiff.second <= 1) ||
					(xdiff.first == 0 && diff - xdiff.second == 0) ||
					(xdiff.second == 0 && diff - xdiff.first == 0))
				{
					implicant = t[1][i] | t[1][j];
					count[i]++;
					count[j]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
				else if (xdiff.first == 0 && diff - xdiff.second == 1)
				{
					implicant = (t[1][i] & t[1][j]).xoutnulls();
					count[i]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
				else if (xdiff.second == 0 && diff - xdiff.first == 1)
				{
					implicant = (t[1][i] & t[1][j]).xoutnulls();
					count[j]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
			}
		}
		count_sum = 0;
		for (size_t i = 0; i < t[1].size(); i++)
		{
			count_sum += count[i];
			if (count[i] == 0)
				primes.push_back(t[1][i]);
		}

		t[1] = t[0];
	}

	/**
	 * 2.	Use those prime implicants in a prime implicant chart to find the essential prime implicants of the expression, as well as other prime implicants that are necessary to cover the expression.
	 */
	cov.clear();
	for (size_t j = 0; j < implicants.size(); j++)
		cov.insert(pair<int, vector<int> >(j, vector<int>()));
	for (size_t j = 0; j < implicants.size(); j++)
	{
		for (size_t i = 0; i < primes.size(); i++)
			if (primes[i].subset(implicants[j]))
				cov[j].push_back(i);

		if (cov[j].size() == 1 && find(essentials.begin(), essentials.end(), cov[j].front()) == essentials.end())
		{
			essentials.push_back(cov[j].front());
			terms.push_back(primes[cov[j].front()]);
		}
	}

	Tcov.clear();
	for (size_t j = 0; j < primes.size(); j++)
		Tcov.insert(pair<int, vector<int> >(j, vector<int>()));
	for (size_t j = 0; j < cov.size(); j++)
	{
		size_t i = 0;
		for (i = 0; i < essentials.size(); i++)
			if (find(cov[j].begin(), cov[j].end(), essentials[i]) != cov[j].end())
				break;

		for (size_t k = 0; i == essentials.size() && k < cov[j].size(); k++)
			Tcov[cov[j][k]].push_back(j);
	}

	size_t max_count = implicants.size();
	size_t choice = 0;
	while (max_count > 0)
	{
		max_count = 0;
		for (size_t i = 0; i < primes.size(); i++)
		{
			if (Tcov[i].size() > max_count)
			{
				max_count = Tcov.size();
				choice = i;
			}
		}

		if (max_count > 0)
		{
			terms.push_back(primes[choice]);

			for (size_t i = 0; i < primes.size(); i++)
				for (size_t j = 0; i != choice && j < Tcov[choice].size(); j++)
				{
					ci = find(Tcov[i].begin(), Tcov[i].end(), Tcov[choice][j]);
					if (ci != Tcov[i].end())
						Tcov[i].erase(ci);
				}

			Tcov[choice].clear();
		}
	}

	sort(terms.begin(), terms.end());
}

void canonical::mccluskey_or(size_t  separator)
{
	canonical temp;
	vector<minterm> implicants;
	vector<minterm> primes;
	vector<int> essentials;
	vector<minterm> t[2] = {vector<minterm>(), vector<minterm>()};
	minterm implicant;

	vector<int> count;
	int count_sum;
	vector<int> vl;

	map<int, vector<int> > cov, Tcov;
	vector<int>::iterator ci;

	pair<int, int> xdiff;
	int diff;

	for (size_t i = 0; i < terms.size(); i++)
	{
		if (terms[i] != 0)
			implicants.push_back(terms[i]);
		else if (i < separator)
			separator--;
	}

	// This first step is an optimization on the idea
	// that if we are oring two canonical expressions
	// together, then we assume that those two have
	// already been individually simplified.
	count.clear();
	count.resize(implicants.size(), 0);

	for (size_t i = 0; i < separator; i++)
		for (size_t j = separator; j < implicants.size(); j++)
		{
			xdiff = implicants[i].xdiff_count(implicants[j]);
			diff = implicants[i].diff_count(implicants[j]);
			if ((diff <= 1 && xdiff.first + xdiff.second <= 1) ||
				(xdiff.first == 0 && diff - xdiff.second == 0) ||
				(xdiff.second == 0 && diff - xdiff.first == 0))
			{
				implicant = implicants[i] | implicants[j];
				count[i]++;
				count[j]++;
				if (find(t[1].begin(), t[1].end(), implicant) == t[1].end())
					t[1].push_back(implicant);
			}
			else if (xdiff.first == 0 && diff - xdiff.second == 1)
			{
				implicant = (implicants[i] & implicants[j]).xoutnulls();
				count[i]++;
				if (find(t[1].begin(), t[1].end(), implicant) == t[1].end())
					t[1].push_back(implicant);
			}
			else if (xdiff.second == 0 && diff - xdiff.first == 1)
			{
				implicant = (implicants[i] & implicants[j]).xoutnulls();
				count[j]++;
				if (find(t[1].begin(), t[1].end(), implicant) == t[1].end())
					t[1].push_back(implicant);
			}
		}

	count_sum = 0;
	for (size_t i = 0; i < implicants.size(); i++)
	{
		count_sum += count[i];
		if (count[i] == 0)
			primes.push_back(implicants[i]);
	}

	terms.clear();

	/**
	 * 1.	Find all prime implicants of the expression.
	 */
	while (count_sum > 0)
	{
		t[0].clear();
		count.clear();
		count.resize(t[1].size(), 0);

		for (size_t i = 0; i < t[1].size(); i++)
		{
			for (size_t j = i+1; j < t[1].size(); j++)
			{
				xdiff = t[1][i].xdiff_count(t[1][j]);
				diff = t[1][i].diff_count(t[1][j]);
				if ((diff <= 1 && xdiff.first + xdiff.second <= 1) ||
					(xdiff.first == 0 && diff - xdiff.second == 0) ||
					(xdiff.second == 0 && diff - xdiff.first == 0))
				{
					implicant = t[1][i] | t[1][j];
					count[i]++;
					count[j]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
				else if (xdiff.first == 0 && diff - xdiff.second == 1)
				{
					implicant = (t[1][i] & t[1][j]).xoutnulls();
					count[i]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
				else if (xdiff.second == 0 && diff - xdiff.first == 1)
				{
					implicant = (t[1][i] & t[1][j]).xoutnulls();
					count[j]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
			}
		}

		count_sum = 0;
		for (size_t i = 0; i < t[1].size(); i++)
		{
			count_sum += count[i];
			if (count[i] == 0)
				primes.push_back(t[1][i]);
		}

		t[1] = t[0];
	}

	/**
	 * 2.	Use those prime implicants in a prime implicant chart to find the essential prime implicants of the expression, as well as other prime implicants that are necessary to cover the expression.
	 */
	cov.clear();
	for (size_t j = 0; j < implicants.size(); j++)
		cov.insert(pair<int, vector<int> >(j, vector<int>()));
	for (size_t j = 0; j < implicants.size(); j++)
	{
		for (size_t i = 0; i < primes.size(); i++)
			if (primes[i].subset(implicants[j]))
				cov[j].push_back(i);

		if (cov[j].size() == 1 && find(essentials.begin(), essentials.end(), cov[j].front()) == essentials.end())
		{
			essentials.push_back(cov[j].front());
			terms.push_back(primes[cov[j].front()]);
		}
	}

	Tcov.clear();
	for (size_t j = 0; j < primes.size(); j++)
		Tcov.insert(pair<int, vector<int> >(j, vector<int>()));
	for (size_t j = 0; j < cov.size(); j++)
	{
		size_t i;
		for (i = 0; i < essentials.size(); i++)
			if (find(cov[j].begin(), cov[j].end(), essentials[i]) != cov[j].end())
				break;

		for (size_t k = 0; i == essentials.size() && k < cov[j].size(); k++)
			Tcov[cov[j][k]].push_back(j);
	}

	size_t max_count = implicants.size();
	size_t choice = 0;
	while (max_count > 0)
	{
		max_count = 0;
		for (size_t i = 0; i < primes.size(); i++)
		{
			if (Tcov[i].size() > max_count)
			{
				max_count = Tcov.size();
				choice = i;
			}
		}

		if (max_count > 0)
		{
			terms.push_back(primes[choice]);

			for (size_t i = 0; i < primes.size(); i++)
				for (size_t j = 0; i != choice && j < Tcov[choice].size(); j++)
				{
					ci = find(Tcov[i].begin(), Tcov[i].end(), Tcov[choice][j]);
					if (ci != Tcov[i].end())
						Tcov[i].erase(ci);
				}

			Tcov[choice].clear();
		}
	}

	sort(terms.begin(), terms.end());
}

void canonical::mccluskey_and()
{
	canonical temp;
	vector<minterm> implicants;
	vector<minterm> primes;
	vector<int> essentials;
	vector<minterm> t[2] = {vector<minterm>(), vector<minterm>()};
	minterm implicant;

	vector<int> count;
	int count_sum;
	vector<int> vl;

	map<int, vector<int> > cov, Tcov;
	vector<int>::iterator ci;

	pair<int, int> xdiff;
	int diff;

	for (size_t i = 0; i < terms.size(); i++)
		if (terms[i] != 0)
			implicants.push_back(terms[i]);

	t[1] = implicants;
	terms.clear();

	/**
	 * 1.	Find all prime implicants of the expression.
	 */
	count_sum = t[1].size();
	while (count_sum > 0)
	{
		t[0].clear();
		count.clear();
		count.resize(t[1].size(), 0);
		for (size_t i = 0; i < t[1].size(); i++)
		{
			for (size_t j = i+1; j < t[1].size(); j++)
			{
				xdiff = t[1][i].xdiff_count(t[1][j]);
				diff = t[1][i].diff_count(t[1][j]);
				if ((diff <= 1 && xdiff.first + xdiff.second <= 1) ||
					(xdiff.first == 0 && diff - xdiff.second == 0) ||
					(xdiff.second == 0 && diff - xdiff.first == 0))
				{
					implicant = t[1][i] | t[1][j];
					count[i]++;
					count[j]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
				else if (xdiff.first == 0 && diff - xdiff.second == 1)
				{
					implicant = (t[1][i] & t[1][j]).xoutnulls();
					count[i]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
				else if (xdiff.second == 0 && diff - xdiff.first == 1)
				{
					implicant = (t[1][i] & t[1][j]).xoutnulls();
					count[j]++;
					if (find(t[0].begin(), t[0].end(), implicant) == t[0].end())
						t[0].push_back(implicant);
				}
			}
		}
		count_sum = 0;
		for (size_t i = 0; i < t[1].size(); i++)
		{
			count_sum += count[i];
			if (count[i] == 0)
				primes.push_back(t[1][i]);
		}

		t[1] = t[0];
	}

	/**
	 * 2.	Use those prime implicants in a prime implicant chart to find the essential prime implicants of the expression, as well as other prime implicants that are necessary to cover the expression.
	 */
	cov.clear();
	for (size_t j = 0; j < implicants.size(); j++)
		cov.insert(pair<int, vector<int> >(j, vector<int>()));
	for (size_t j = 0; j < implicants.size(); j++)
	{
		for (size_t i = 0; i < primes.size(); i++)
			if (primes[i].subset(implicants[j]))
				cov[j].push_back(i);

		if (cov[j].size() == 1 && find(essentials.begin(), essentials.end(), cov[j].front()) == essentials.end())
		{
			essentials.push_back(cov[j].front());
			terms.push_back(primes[cov[j].front()]);
		}
	}

	Tcov.clear();
	for (size_t j = 0; j < primes.size(); j++)
		Tcov.insert(pair<int, vector<int> >(j, vector<int>()));
	for (size_t j = 0; j < cov.size(); j++)
	{
		size_t i;
		for (i = 0; i < essentials.size(); i++)
			if (find(cov[j].begin(), cov[j].end(), essentials[i]) != cov[j].end())
				break;

		for (size_t k = 0; i == essentials.size() && k < cov[j].size(); k++)
			Tcov[cov[j][k]].push_back(j);
	}

	size_t max_count = implicants.size();
	size_t choice = 0;
	while (max_count > 0)
	{
		max_count = 0;
		for (size_t i = 0; i < primes.size(); i++)
		{
			if (Tcov[i].size() > max_count)
			{
				max_count = Tcov.size();
				choice = i;
			}
		}

		if (max_count > 0)
		{
			terms.push_back(primes[choice]);

			for (size_t i = 0; i < primes.size(); i++)
				for (size_t j = 0; i != choice && j < Tcov[choice].size(); j++)
				{
					ci = find(Tcov[i].begin(), Tcov[i].end(), Tcov[choice][j]);
					if (ci != Tcov[i].end())
						Tcov[i].erase(ci);
				}

			Tcov[choice].clear();
		}
	}

	sort(terms.begin(), terms.end());
}

minterm canonical::mask()
{
	minterm result;
	for (size_t i = 0; i < terms.size(); i++)
		result |= terms[i].mask();
	return result;
}

void canonical::vars(vector<size_t> *var_list)
{
	for (size_t i = 0; i < terms.size(); i++)
		terms[i].vars(var_list);
}

vector<size_t> canonical::vars()
{
	vector<size_t> result;
	for (size_t i = 0; i < terms.size(); i++)
		terms[i].vars(&result);
	return result;
}

canonical canonical::refactor(vector<size_t> ids)
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		result.terms.push_back(terms[i].refactor(ids));
	return result;
}

canonical canonical::refactor(vector<pair<size_t, size_t> > ids)
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		result.terms.push_back(terms[i].refactor(ids));
	return result;
}


canonical canonical::hide(size_t var)
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		result.terms.push_back(terms[i].hide(var));
	result.mccluskey();
	return result;
}

canonical canonical::hide(vector<size_t> vars)
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		result.terms.push_back(terms[i].hide(vars));
	result.mccluskey();
	return result;
}

canonical canonical::restrict(canonical r)
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		for (size_t j = 0; j < r.terms.size(); j++)
			if ((terms[i] & r.terms[j]) != 0)
				result.terms.push_back(terms[i].hide(r.terms[j].vars()));
	result.mccluskey();
	return result;
}

void canonical::extract(map<int, canonical> *result)
{
	minterm m;
	vector<size_t> v = vars();
	v.resize(unique(v.begin(), v.end()) - v.begin());

	for (size_t i = 0; i < terms.size(); i++)
		m = (i == 0 ? terms[i] : m | terms[i]);

	for (size_t i = 0; i < v.size(); i++)
		result->insert(pair<int, canonical>(v[i], canonical(m[v[i]])));
}

map<int, canonical> canonical::extract()
{
	map<int, canonical> result;
	minterm m;
	vector<size_t> v = vars();
	v.resize(unique(v.begin(), v.end()) - v.begin());

	for (size_t i = 0; i < terms.size(); i++)
		m = (i == 0 ? terms[i] : m | terms[i]);

	for (size_t i = 0; i < v.size(); i++)
		result.insert(pair<int, canonical>(v[i], canonical(m[v[i]])));
	return result;
}

uint32_t canonical::val(size_t uid)
{
	uint32_t v = 0;
	for (size_t i = 0; i < terms.size(); i++)
	{
		if (uid >= terms[i].size)
			terms[i].resize(uid+1, 0xFFFFFFFF);

		v |= terms[i].values[uid>>4];
	}

	return mtoi(v >> vidx(uid));
}

canonical canonical::pabs()
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		result.push_back(terms[i].pabs());
	result.mccluskey();
	return result;
}

canonical canonical::nabs()
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		result.push_back(terms[i].nabs());
	result.mccluskey();
	return result;
}

int canonical::satcount()
{
	return terms.size();
}

map<int, uint32_t> canonical::anysat()
{
	return terms[0].anysat();
}

vector<map<int, uint32_t> > canonical::allsat()
{
	vector<map<int, uint32_t> > sats;
	for (size_t i = 0; i < terms.size(); i++)
		sats.push_back(terms[i].anysat());
	return sats;
}

canonical &canonical::operator=(canonical c)
{
	this->terms.clear();
	this->terms = c.terms;
	return *this;
}

canonical &canonical::operator=(minterm t)
{
	this->terms.clear();
	this->terms.push_back(t);
	return *this;
}

canonical &canonical::operator=(uint32_t c)
{
	terms.clear();
	if (c == 1)
		terms.push_back(minterm());
	return *this;
}

canonical &canonical::operator|=(canonical c)
{
	*this = *this | c;
	return *this;
}

canonical &canonical::operator&=(canonical c)
{
	*this = *this & c;
	return *this;
}

canonical &canonical::operator^=(canonical c)
{
	*this = *this ^ c;
	return *this;
}

canonical &canonical::operator|=(uint32_t c)
{
	if (c == 1)
	{
		terms.clear();
		terms.push_back(minterm());
	}
	return *this;
}
canonical &canonical::operator&=(uint32_t c)
{
	if (c == 0)
		terms.clear();
	return *this;
}

/**
 * \brief	Restricts the variable whose index is i to the value v.
 * \details	Given a binary boolean expression f, a variable x whose index is i,
 * 			and a value v, this calculates f(x = v). This does not modify the
 * 			canonical this was applied on. Instead, it makes a copy, does the
 * 			operation and returns the copy.
 * \param	i	The index of the variable to restrict.
 * \param	v	The value to restrict the variable to (v0, v1, vX, or v_).
 * \return	The canonical that results from this restriction.
 */
canonical canonical::operator()(int j, uint32_t b)
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
	{
		uint32_t v = terms[i].val(j);
		if (v == 2 || v == b)
			result.push_back(terms[i](j, b));
	}
	result.mccluskey();
	return result;
}

canonical canonical::operator[](int i)
{
	minterm result;
	for (size_t j = 0; j < terms.size(); j++)
		result.sv_union(i, terms[j].get(i));
	return canonical(result);
}

canonical canonical::operator|(canonical c)
{
	canonical result;
	result.terms.insert(result.terms.end(), terms.begin(), terms.end());
	result.terms.insert(result.terms.end(), c.terms.begin(), c.terms.end());
	result.mccluskey_or(terms.size());
	return result;
}

canonical canonical::operator&(canonical c)
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		for (size_t j = 0; j < c.terms.size(); j++)
		{
			result.terms.push_back(terms[i] & c.terms[j]);
			if (result.terms.size() >= 128)
				result.mccluskey();
		}
	result.mccluskey();
	return result;
}

canonical canonical::operator~()
{
	canonical result = 1;
	int i;
	for (i = 0; i < (int)terms.size(); i++)
		result = result & ~terms[i];
	result.mccluskey();
	return result;
}

canonical canonical::operator^(canonical c)
{
	return ((*this & ~c) | (~*this & c));
}

canonical canonical::operator&&(canonical c)
{
	canonical result;
	for (size_t i = 0; i < terms.size(); i++)
		for (size_t j = 0; j < c.terms.size(); j++)
		{
			result.terms.push_back((terms[i] & c.terms[j]).xoutnulls());
			if (result.terms.size() >= 128)
				result.mccluskey();
		}
	result.mccluskey();
	return result;
}

canonical canonical::operator|(uint32_t c)
{
	canonical result;
	if (c == 1)
		result.terms.push_back(minterm());
	else
		result = *this;
	return result;
}

canonical canonical::operator&(uint32_t c)
{
	if (c == 1)
		return *this;
	else
		return canonical();
}

bool canonical::operator==(const canonical c) const
{
	canonical t1 = *this;
	canonical t2 = c;
	sort(t1.terms.begin(), t1.terms.end());
	sort(t2.terms.begin(), t2.terms.end());
	return t1.terms == t2.terms;
}

bool canonical::operator!=(const canonical c) const
{
	canonical t1 = *this;
	canonical t2 = c;
	sort(t1.terms.begin(), t1.terms.end());
	sort(t2.terms.begin(), t2.terms.end());
	return t1.terms != t2.terms;
}

bool canonical::operator==(minterm c)
{
	return (terms.size() == 1 && terms[0] == c);
}

bool canonical::operator!=(minterm c)
{
	return (terms.size() != 1 || terms[0] != c);
}

bool canonical::operator==(uint32_t c)
{
	if (c == 0)
	{
		bool zero = true;
		for (int i = 0; i < (int)terms.size() && zero; i++)
			zero = zero && (terms[i] == 0);
		return zero;
	}
	else if (c == 1)
	{
		bool one = false;
		for (int i = 0; i < (int)terms.size() && !one; i++)
			one = one || (terms[i] == 1);
		return one;
	}
	else
		return false;
}

bool canonical::operator!=(uint32_t c)
{
	if (c == 0)
	{
		bool zero = true;
		for (int i = 0; i < (int)terms.size() && zero; i++)
			zero = zero && (terms[i] == 0);
		return !zero;
	}
	else if (c == 1)
	{
		bool one = false;
		for (int i = 0; i < (int)terms.size() && !one; i++)
			one = one || (terms[i] == 1);
		return !one;
	}
	else
		return true;
}

bool canonical::constant()
{
	return ((terms.size() == 0) || (terms.size() == 1 && terms[0] == 1));
}

canonical canonical::operator>>(canonical c)
{
	canonical result;
	int i, j;

	for (i = 0; i < (int)terms.size(); i++)
		for (j = 0; j < (int)c.terms.size(); j++)
			result.terms.push_back(terms[i] >> c.terms[j]);
	result.mccluskey();

	return result;
}

/**
 * \brief	Prints the sum of minterms to stdout.
 * \param	vars	A list of variable names indexed by variable index.
 */
string canonical::print(variable_space &vars, string prefix)
{
	if (terms.size() == 0)
		return "0";
	else if (terms.size() == 1 && terms[0] == 1)
		return "1";

	string res;
	for (int i = 0; i < (int)terms.size(); i++)
	{
		if (i != 0)
			res += "|";
		res += terms[i].print(vars, prefix);
	}
	return res;
}

string canonical::print_assign(variable_space &v, string prefix)
{
	string result = "";
	for (size_t i = 0; i < terms.size(); i++)
	{
		if (i != 0)
			result += ", ";
		result += terms[i].print_assign(v, prefix);
	}
	return result == "" ? "skip" : result;
}

string canonical::print_with_quotes(variable_space &vars, string prefix)
{
	if (terms.size() == 0)
                return "0";
        else if (terms.size() == 1 && terms[0] == 1)
                return "1";

        string res;
        for (int i = 0; i < (int)terms.size(); i++)
        {
                if (i != 0)
                        res += "|";
                res += terms[i].print_with_quotes(vars, prefix);
        }
        return res;
}

bool operator<(canonical c0, canonical c1)
{
	sort(c0.terms.begin(), c0.terms.end());
	sort(c1.terms.begin(), c1.terms.end());
	return c0.terms < c1.terms;
}

bool is_mutex(canonical *c0, canonical *c1)
{
	int i, j;
	for (i = 0; i < (int)c0->terms.size(); i++)
		for (j = 0; j < (int)c1->terms.size(); j++)
			if ((c0->terms[i] & c1->terms[j]) != 0)
				return false;

	return true;
}

bool is_mutex(canonical c0, canonical c1)
{
	int i, j;
	for (i = 0; i < (int)c0.terms.size(); i++)
		for (j = 0; j < (int)c1.terms.size(); j++)
			if ((c0.terms[i] & c1.terms[j]) != 0)
				return false;

	return true;
}

bool is_mutex(minterm *m0, canonical *c1)
{
	int j;
		for (j = 0; j < (int)c1->terms.size(); j++)
			if ((*m0 & c1->terms[j]) != 0)
				return false;

	return true;
}

bool is_mutex(canonical *c0, minterm *m1)
{
	int i;
	for (i = 0; i < (int)c0->terms.size(); i++)
		if ((c0->terms[i] & *m1) != 0)
			return false;

	return true;
}


bool is_mutex(canonical *c0, canonical *c1, canonical *c2)
{
	int i, j, k;
	for (i = 0; i < (int)c0->terms.size(); i++)
		for (j = 0; j < (int)c1->terms.size(); j++)
			for (k = 0; k < (int)c2->terms.size(); k++)
			if ((c0->terms[i] & c1->terms[j] & c2->terms[k]) != 0)
				return false;

	return true;
}

bool is_mutex(canonical *c0, canonical *c1, canonical *c2, canonical *c3)
{
	int i, j, k, l;
	for (i = 0; i < (int)c0->terms.size(); i++)
		for (j = 0; j < (int)c1->terms.size(); j++)
			for (k = 0; k < (int)c2->terms.size(); k++)
				for (l = 0; l < (int)c3->terms.size(); l++)
					if ((c0->terms[i] & c1->terms[j] & c2->terms[k] & c3->terms[l]) != 0)
						return false;

	return true;
}

bool mergible(canonical *c0, canonical *c1)
{
	for (size_t i = 0; i < c0->terms.size(); i++)
	{
		for (size_t j = 0; j < c1->terms.size(); j++)
		{
			pair<int, int> xdiff = c0->terms[i].xdiff_count(c1->terms[j]);
			int diff = c0->terms[i].diff_count(c1->terms[j]);
			if ((diff <= 1 && xdiff.first + xdiff.second <= 1) ||
				(xdiff.first == 0 && diff - xdiff.second <= 1) ||
				(xdiff.second == 0 && diff - xdiff.first <= 1))
				return true;
		}
	}
	return false;
}

bool mergible(minterm c0, minterm c1)
{
	pair<int, int> xdiff = c0.xdiff_count(c1);
	int diff = c0.diff_count(c1);
	return ((diff <= 1 && xdiff.first + xdiff.second <= 1) ||
		(xdiff.first == 0 && diff - xdiff.second <= 1) ||
		(xdiff.second == 0 && diff - xdiff.first <= 1));
}

canonical merge(canonical c0, canonical c1)
{
	canonical result = c0;

	for (size_t j = 0; j < c1.terms.size(); j++)
	{
		bool merged = false;
		for (size_t i = 0; i < result.terms.size(); i++)
			if (mergible(result.terms[i], c1.terms[j]) && (result.terms[i] & c1.terms[j]) != 0)
			{
				result.terms[i] &= c1.terms[j];
				merged = true;
			}

		if (!merged)
			result.terms.push_back(c1.terms[j]);
	}

	result.mccluskey();

	return result;
}
