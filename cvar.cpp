#include <string>
#include "cvar.hpp"
using namespace std;

VarPool *clientCvars = NULL,
        *serverCvars = NULL;

bool loadingMap;


void VarPool::registerVar(BaseConsoleVar *var)
{
	vars[var->getName()] = var;
}

BaseConsoleVar *VarPool::lookup(const char *name)
{
	if(vars.find(std::string(name)) == vars.end())
		return NULL;
	return vars[ std::string(name) ];
}

VarPool::iterator VarPool::begin(void)
	{ return vars.begin(); }
VarPool::iterator VarPool::end(void)
	{ return vars.end(); }

void VarPool::restoreVars(void)
{
	for(iterator ii=begin(); ii!=end(); ii++)
	{
		ii->second->restoreValue();
	}
}

void ConsoleVar<bool>::valueFromString(const char *c_str)
{
	if(!strcasecmp(c_str, "true"))
		setValue(true);
	else if(!strcasecmp(c_str, "false"))
		setValue(false);
	else if(atoi(c_str) != 0)
		setValue(true);
	else
		setValue(false);
}

void ConsoleVar<int>::valueFromString(const char *c_str)
{
	setValue(atoi(c_str));
}

void ConsoleVar<float>::valueFromString(const char *c_str)
{
	setValue(atof(c_str));
}

void ConsoleVar<string>::valueFromString(const char *c_str)
{
	setValue( string(c_str) );
}
