#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <map>
#include <string>

class Keyboard
{
public:
	Keyboard();
	void bind(const char *key, const char *action);
	void unbind(const char *key);
	void unbindall(void);
	
	void keydown(const char *key, int mod);
	void keyup(const char *key, int mod);
	std::string getCommand(const char *key, int mod);
	
private:
	std::map<std::string, std::string> bindings;
};

extern Keyboard keyboard;

#endif

