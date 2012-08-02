#include "mastrix.hpp"

class LineEditor :public EventHandler
{
public:
	LineEditor();
	void setParams(std::string prompt, std::string command);
	bool handleEvent(SDL_Event *ev);
	void keypress(char key);
	void redraw(void);
	bool timepass(void);
	int getPriority(void)           {return 1;}
	
private:
	std::string prompt, command, line;
};
LineEditor editor;

LineEditor::LineEditor()
{
}

void LineEditor::setParams(std::string prompt, std::string command)
{
	this->prompt = prompt;
	this->command = command;
	this->line = "";
}

bool LineEditor::handleEvent(SDL_Event *ev)
{
	switch (ev->type)
	{
		case SDL_KEYDOWN:
			if(ev->key.keysym.unicode > 0 && ev->key.keysym.unicode < 256)
				keypress( ev->key.keysym.unicode );
			return true;

		case SDL_KEYUP:
			return true;
			
		default:
			return false;
	}
	return false;
}

void LineEditor::keypress(char key)
{
	switch(key)
	{
		case 27:
			remove();
			return;
		case '\b':
			line = line.substr(0, line.length()-1);
			break;
		case '\n': case '\r':
			clientScripting.command( retprintf(("%s \"%s\""), command.c_str(), quotify(line).c_str()).c_str() );
			remove();
			return;
		default:
			line += key;
			break;
	}
}
void LineEditor::redraw(void)
{
	graphics.drawText( retprintf(("%s %s"), prompt.c_str(), line.c_str()).c_str(), 50, 700);
}
bool LineEditor::timepass(void)
{
	return false;
}


CLIENT_CONSOLE_COMMAND(editline)
{
	if(argc < 2) {
		console.printf("Usage: editline [prompt] [command]");
		return;
	}
	editor.setParams(argv[0], argv[1]);
	editor.enable();
}
