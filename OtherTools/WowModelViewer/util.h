#ifndef UTIL_H
#define UTIL_H

#ifdef _WIN32
	#include <hash_set>
#else
	#include <ext/hash_set>
#endif


// STL headers
#include <algorithm>
//#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <set>
#include <sstream>
#include <vector>

// Standard C++ headers
#include <stdio.h>

// Our other utility headers
#include "vec3d.h"

using namespace std;

extern bool useLocalFiles;
extern bool useRandomLooks;
extern bool hasBeta;

extern long langID;
extern int ssCounter;
extern int imgFormat;

float frand();
float randfloat(float lower, float upper);
int randint(int lower, int upper);

template <class T>
bool from_string(T& t, const string& s, ios_base& (*f)(ios_base&))
{
  istringstream iss(s);
  return !(iss >> f >> t).fail();
}

void fixname(std::string &name);
void fixnamen(char *name, size_t len);

#endif

