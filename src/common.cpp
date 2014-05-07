#include "common.h"

/*Is this character a legal name starter character?
 */
bool ac(char c)
{
	return ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') );
}


/* Is this character a character that is legal to have
 * in a type name or variable name? a-z A-Z 0-9 _
 */
bool nc(char c)
{
	return ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			(c == '_') || (c == '.'));
}

/* Is this character an operator?
 *
 */
bool oc(char c)
{
	return (c == ':' ||
			c == '=' ||
			c == '|' ||
			c == '&' ||
			c == '~' ||
			c == '>' ||
			c == '<' ||
			c == ';' ||
			c == '*' ||
			c == '[' ||
			c == ']' ||
			c == '(' ||
			c == ')' ||
			c == '{' ||
			c == '}' ||
			c == '+' ||
			c == '-' ||
			c == '!' ||
			c == '?' ||
			c == '@' ||
			c == '#' ||
			c == '/');
}

/* Is this character whitespace?
 *
 */
bool sc(char c)
{
	return (c == ' '  ||
			c == '\t' ||
			c == '\n' ||
			c == '\r');
}

// BIG ENDIAN

int hex_to_int(string str)
{
	int result = 0;
	int mul = 1;
	string::reverse_iterator i;

	for (i = str.rbegin(), mul = 1; i != str.rend(); i++, mul *= 16)
	{
		switch (*i)
		{
		case '0': result += 0; break;
		case '1': result += mul; break;
		case '2': result += 2*mul; break;
		case '3': result += 3*mul; break;
		case '4': result += 4*mul; break;
		case '5': result += 5*mul; break;
		case '6': result += 6*mul; break;
		case '7': result += 7*mul; break;
		case '8': result += 8*mul; break;
		case '9': result += 9*mul; break;
		case 'a': result += 10*mul; break;
		case 'b': result += 11*mul; break;
		case 'c': result += 12*mul; break;
		case 'd': result += 13*mul; break;
		case 'e': result += 14*mul; break;
		case 'f': result += 15*mul; break;
		case 'A': result += 10*mul; break;
		case 'B': result += 11*mul; break;
		case 'C': result += 12*mul; break;
		case 'D': result += 13*mul; break;
		case 'E': result += 14*mul; break;
		case 'F': result += 15*mul; break;
		default:  return 0;
		}
	}

	return result;
}

string hex_to_bin(string str)
{
	string result = "";
	string::iterator i;

	for (i = str.begin(); i != str.end(); i++)
	{
		switch (*i)
		{
		case '0': result += "0000"; break;
		case '1': result += "0001"; break;
		case '2': result += "0010"; break;
		case '3': result += "0011"; break;
		case '4': result += "0100"; break;
		case '5': result += "0101"; break;
		case '6': result += "0110"; break;
		case '7': result += "0111"; break;
		case '8': result += "1000"; break;
		case '9': result += "1001"; break;
		case 'a': result += "1010"; break;
		case 'b': result += "1011"; break;
		case 'c': result += "1100"; break;
		case 'd': result += "1101"; break;
		case 'e': result += "1110"; break;
		case 'f': result += "1111"; break;
		case 'A': result += "1010"; break;
		case 'B': result += "1011"; break;
		case 'C': result += "1100"; break;
		case 'D': result += "1101"; break;
		case 'E': result += "1110"; break;
		case 'F': result += "1111"; break;
		default:  return "";
		}
	}

	return result;
}

int dec_to_int(string str)
{
	int result = 0;
	int mul = 1;
	string::reverse_iterator i;

	for (i = str.rbegin(), mul = 1; i != str.rend(); i++, mul *= 10)
	{
		switch (*i)
		{
		case '0': result += 0; break;
		case '1': result += mul; break;
		case '2': result += 2*mul; break;
		case '3': result += 3*mul; break;
		case '4': result += 4*mul; break;
		case '5': result += 5*mul; break;
		case '6': result += 6*mul; break;
		case '7': result += 7*mul; break;
		case '8': result += 8*mul; break;
		case '9': result += 9*mul; break;
		default:  return 0;
		}
	}

	return result;
}

int bin_to_int(string str)
{
	int result = 0;
	int mul = 1;
	string::reverse_iterator i;

	for (i = str.rbegin(), mul = 1; i != str.rend(); i++, mul *= 2)
	{
		switch (*i)
		{
		case '0': result += 0; break;
		case '1': result += mul; break;
		default:  return 0;
		}
	}

	return result;
}

string int_to_bin(int dec)
{
	string result = "";
	int i = 0;

	if (dec == 0)
		return "0";

	while ((dec&0x80000000) == 0)
	{
		i++;
		dec <<= 1;
	}

	for (; i < 32; i++)
	{
		result += char('0' + ((dec&0x80000000) > 1));
		dec <<= 1;
	}

	return result;
}

string dec_to_bin(string str)
{
	return int_to_bin(dec_to_int(str));
}

unsigned int count_1bits(unsigned int x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x0000003F;
}

unsigned int count_0bits(unsigned int x)
{
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
	x = x + (x >> 8);
	x = x + (x >> 16);
    return 32 - (x & 0x0000003F);
}

int powi(int base, int exp)
{
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

int log2i(unsigned long long value)
{
  static const unsigned long long t[6] = {
    0xFFFFFFFF00000000ull,
    0x00000000FFFF0000ull,
    0x000000000000FF00ull,
    0x00000000000000F0ull,
    0x000000000000000Cull,
    0x0000000000000002ull
  };

  int y = (((value & (value - 1)) == 0) ? 0 : 1);
  int j = 32;
  int i;

  for (i = 0; i < 6; i++) {
    int k = (((value & t[i]) == 0) ? 0 : j);
    y += k;
    value >>= k;
    j >>= 1;
  }

  return y;
}

uint32_t bitwise_or(uint32_t a, uint32_t b)
{
	return a||b;
}

uint32_t bitwise_and(uint32_t a, uint32_t b)
{
	return a&&b;
}

uint32_t bitwise_not(uint32_t a)
{
	return !a;
}

size_t find_first_of_l0(string content, string search, size_t pos)
{
	string::iterator i, j;
	int depth[3] = {0, 0, 0};
	size_t ret;

	for (i = content.begin() + pos, ret = pos; i != content.end(); i++, ret++)
	{
		if (*i == ')')
			depth[0]--;
		else if (*i == ']')
			depth[1]--;
		else if (*i == '}')
			depth[2]--;

		for (j = search.begin(); j != search.end() && depth[0] == 0 && depth[1] == 0 && depth[2] == 0; j++)
			if (*i == *j)
			{
				if (i == content.end())
					return string::npos;
				else
					return ret;
			}

		if (*i == '(')
			depth[0]++;
		else if (*i == '[')
			depth[1]++;
		else if (*i == '{')
			depth[2]++;
	}

	return string::npos;
}

size_t find_first_of_l0(string content, list<string> search, size_t pos, list<string> exclude)
{
	bool found;
	string::iterator i;
	list<string>::iterator j;
	int depth[3] = {0, 0, 0};
	size_t ret;

	found = false;
	for (i = content.begin() + pos, ret = pos; i != content.end(); i++, ret++)
	{
		if (*i == ')')
			depth[0]--;
		else if (*i == ']')
			depth[1]--;
		else if (*i == '}')
			depth[2]--;

		for (j = search.begin(); j != search.end() && depth[0] == 0 && depth[1] == 0 && depth[2] == 0 && !found; j++)
			if (content.substr(ret, j->length()) == *j)
				found = true;
		for (j = exclude.begin(); j != exclude.end() && depth[0] == 0 && depth[1] == 0 && depth[2] == 0 && found; j++)
			if (content.substr(ret, j->length()) == *j)
				found = false;

		if (*i == '(')
			depth[0]++;
		else if (*i == '[')
			depth[1]++;
		else if (*i == '{')
			depth[2]++;

		if (found)
		{
			if (i == content.end())
				return string::npos;
			else
				return ret;
		}
	}

	return string::npos;
}

size_t find_last_of_l0(string content, string search, size_t pos)
{
	string::reverse_iterator i, j;
	int depth[3] = {0, 0, 0};
	size_t ret;

	if (pos == string::npos)
		pos = 0;
	else
		pos = content.length() - pos;

	for (i = content.rbegin() + pos, ret = pos; i != content.rend(); i++, ret++)
	{
		if (*i == '(')
			depth[0]--;
		else if (*i == '[')
			depth[1]--;
		else if (*i == '{')
			depth[2]--;

		for (j = search.rbegin(); j != search.rend() && depth[0] == 0 && depth[1] == 0 && depth[2] == 0; j++)
			if (*i == *j)
			{
				if (i == content.rend())
					return string::npos;
				else
					return content.length() - ret - 1;
			}

		if (*i == ')')
			depth[0]++;
		else if (*i == ']')
			depth[1]++;
		else if (*i == '}')
			depth[2]++;
	}

	return string::npos;
}

size_t find_last_of_l0(string content, list<string> search, size_t pos, list<string> exclude)
{
	bool found;
	string::reverse_iterator i;
	list<string>::iterator j;
	int depth[3] = {0, 0, 0};
	size_t ret;

	if (pos == string::npos)
		pos = 0;
	else
		pos = content.length() - pos;

	found = false;
	for (i = content.rbegin() + pos, ret = pos; i != content.rend(); i++, ret++)
	{
		if (*i == '(')
			depth[0]--;
		else if (*i == '[')
			depth[1]--;
		else if (*i == '{')
			depth[2]--;

		for (j = search.begin(); j != search.end() && depth[0] == 0 && depth[1] == 0 && depth[2] == 0 && !found; j++)
			if (content.substr(content.length() - ret - 1 - j->length(), j->length()) == *j)
				found = true;
		for (j = exclude.begin(); j != exclude.end() && depth[0] == 0 && depth[1] == 0 && depth[2] == 0 && found; j++)
			if (content.substr(content.length() - ret - 1 - j->length(), j->length()) == *j)
				found = false;

		if (*i == ')')
			depth[0]++;
		else if (*i == ']')
			depth[1]++;
		else if (*i == '}')
			depth[2]++;

		if (found)
		{
			if (i == content.rend())
				return string::npos;
			else
				return content.length() - ret - 1;
		}
	}

	return string::npos;
}

// Only | & ~ operators allowed
string demorgan(string exp, int depth, bool invert)
{
	list<string> ops, ex;

	string left, right;
	size_t p;

	if (depth != 0)
	{
		p = find_first_of_l0(exp, "|");
		if (p != exp.npos)
		{
			left = exp.substr(0, p);
			right = exp.substr(p+1);
			if (invert)
				return demorgan(left, depth-1, true) + "&" + demorgan(right, depth-1, true);
			else
				return "(" + demorgan(left, depth-1, false) + "|" + demorgan(right, depth-1, false) + ")";
		}

		p = find_first_of_l0(exp, "&");
		if (p != exp.npos)
		{
			left = exp.substr(0, p);
			right = exp.substr(p+1);
			if (invert)
				return "(" + demorgan(left, depth-1, true) + "|" + demorgan(right, depth-1, true) + ")";
			else
				return demorgan(left, depth-1, false) + "&" + demorgan(right, depth-1, false);
		}

		p = find_first_of_l0(exp, "~");
		if (p != exp.npos)
		{
			if (invert)
				return demorgan(exp.substr(p+1), depth, false);
			else
				return demorgan(exp.substr(p+1), depth, true);
		}

		if (exp[0] == '(' && exp[exp.length()-1] == ')')
			return demorgan(exp.substr(1, exp.length()-2), depth, invert);
	}

	if (invert)
		return "~" + exp;
	else
		return exp;
}

string strip(string e)
{
	while (e.length() >= 2 && find_first_of_l0(e, "|&~") == e.npos && e.find_first_of("()") != e.npos)
		e = e.substr(1, e.length() - 2);
	return e;
}

vector<string> distribute(string exp)
{
	vector<string> result;
	vector<string> terms;
	vector<string> temp0, temp1;
	string term, tempstr;
	for (size_t i = 0, j = 0, count = 0; i <= exp.length(); i++)
	{
		if (i < exp.length() && exp[i] == '(')
			count++;
		else if (i < exp.length() && exp[i] == ')')
			count--;

		if (i == exp.length() || (count == 0 && exp[i] == '|'))
		{
			terms.clear();
			term = exp.substr(j, i-j);
			for (size_t k = 0, l = 0, count1 = 0; k <= term.size(); k++)
			{
				if (k < term.size() && term[k] == '(')
					count1++;
				else if (k < term.size() && term[k] == ')')
					count1--;

				if (k == term.size() || (count1 == 0 && term[k] == '&'))
				{
					temp0 = terms;
					temp1.clear();
					terms.clear();
					tempstr = term.substr(l, k-l);
					if (tempstr.find_first_of("|&()") == tempstr.npos)
						temp1.push_back(tempstr);
					else
						temp1 = distribute(strip(tempstr));

					if (temp0.size() == 0)
						terms = temp1;
					else if (temp1.size() == 0)
						terms = temp0;
					else
						for (size_t m = 0; m < temp0.size(); m++)
							for (size_t n = 0; n < temp1.size(); n++)
								terms.push_back(temp0[m] + "&" + temp1[n]);
					l = k+1;
				}
			}

			result.insert(result.end(), terms.begin(), terms.end());

			j = i+1;
		}
	}

	return result;
}
