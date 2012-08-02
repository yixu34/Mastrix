#ifndef CONSOLE_H
#define CONSOLE_H

#include <deque>
#include <string>
#include "events.hpp"

class Console
	: public EventHandler
{
public:
	Console();
	
	bool handleEvent(SDL_Event *ev);
	void redraw(void);
	bool timepass(void);
	int getPriority(void)           {return 1;}
	
	void println(const char *str);
	void printf(const char *fmt, ...);
	
	void keypress(char key);
	void command(std::string cmd);
	
	void toggle(void);
	
	inline bool getIsDown(void) { return isDown; }
	
private:
	typedef std::deque<std::string> historyList;
	historyList history;
	int cursor_pos;
	std::string line;
	std::string pending;
	
	bool isDown;
	float position;
};
extern Console console;

#endif

