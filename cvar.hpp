#ifndef CVAR_HPP
#define CVAR_HPP

#include "msvcfix.hpp"

#include <string>
#include <map>
#include <sstream>
#include <iterator>

class BaseConsoleVar;
extern bool loadingMap;

class VarPool
{
protected:
	typedef std::map<std::string, BaseConsoleVar*> varmap;
	typedef varmap::iterator varmap_iterator;
	varmap vars;
	
	class VarIterator : public varmap::iterator
	{
	public:
		VarIterator(varmap::iterator i)
			: varmap_iterator(i) { }
		BaseConsoleVar *operator*(void) {
			return (this->varmap_iterator::operator*()).second;
		}
	};
	
public:
	typedef VarIterator iterator;
	
	void registerVar(BaseConsoleVar *var);
	BaseConsoleVar *lookup(const char *name);
	void restoreVars(void);
	iterator begin(void);
	iterator end(void);
};

extern VarPool *clientCvars, *serverCvars;


class BaseConsoleVar
{
public:
	BaseConsoleVar(std::string name, VarPool *&pool)
	{
		this->name = name;
		if(pool==NULL) pool = new VarPool;
		pool->registerVar(this);
	}
	virtual ~BaseConsoleVar() { }
	
	const char *getName(void) {
		return name.c_str();
	}
	virtual std::string defaultValueString(void) = 0;
	virtual std::string currentValueString(void) = 0;
	
	virtual std::string valueString(void) = 0;
	virtual void valueFromString(const char *str) = 0;
	virtual void restoreValue(void) = 0;
	
protected:
	std::string name;
};


template<class T>
class ConsoleVar : BaseConsoleVar
{
public:
	ConsoleVar(const char *name, T default_value, VarPool *&pool)
		: BaseConsoleVar(name, pool)
	{
		this->default_value = default_value;
		this->change_handler = NULL;
		value = previous_value = default_value;
	}
	ConsoleVar(const char *name, T default_value, VarPool *&pool, void (*change_handler)(T new_value))
		: BaseConsoleVar(name, pool)
	{
		this->default_value = default_value;
		this->change_handler = change_handler;
		value = previous_value = default_value;
	}
	
	void setValue(const T& rhs) {
		bool isChange;
		isChange = (rhs != value);
		value = rhs;
		if(!loadingMap) previous_value = rhs;
		if(change_handler != NULL && isChange)
			change_handler(rhs);
	}
	void restoreValue(void) {
		setValue(previous_value);
	}
	
	T& getValue(void) {
		return value;
	}
	const T& getDefault(void) {
		return default_value;
	}
	
	std::string defaultValueString(void) {
		std::stringstream str;
		str << default_value;
		return str.str();
	}
	std::string currentValueString(void) {
		std::stringstream str;
		str << value;
		return str.str();
	}
	std::string valueString(void) {
		return name+" is "+currentValueString()+
			" (default "+defaultValueString()+")";
	}
	void valueFromString(const char *c_str);
	
protected:
	void (*change_handler)(T new_value);
	T default_value;
	T value;
	T previous_value;
};

template<class T>
class ClientConsoleVar
	: public ConsoleVar<T>
{
public:
	ClientConsoleVar(const char *name, T default_value)
		: ConsoleVar<T>(name, default_value, clientCvars) { }
	ClientConsoleVar(const char *name, T default_value, void (*change_handler)(T new_value))
		: ConsoleVar<T>(name, default_value, clientCvars, change_handler) { }
	
	inline operator T(void) {
		return this->value;
	}
	inline const T& operator= (const T& rhs) {
		setValue(rhs);
		return rhs;
	}
};

template<class T>
class ServerConsoleVar
	: public ConsoleVar<T>
{
public:
	ServerConsoleVar(const char *name, T default_value)
		: ConsoleVar<T>(name, default_value, serverCvars) { }
	ServerConsoleVar(const char *name, T default_value, void (*change_handler)(T new_value))
		: ConsoleVar<T>(name, default_value, serverCvars, change_handler) { }
	
	inline operator T(void) {
		return this->value;
	}
	inline const T& operator= (const T& rhs) {
		setValue(rhs);
		return rhs;
	}
};

#endif

