#include "mastrix.hpp"

ClientConsoleVar<float> consoleSpeed("console_speed", 600);
ClientConsoleVar<float> consoleHeight("console_height", 200);

Console console;

const float console_margin_left = 3;
const float console_margin_bottom = 10;
const float fade_height = 10;
const float bottom_margin = 3;

const float text_height = 12;
const float line_margin = 4;

Color console_bgcolor(77,77,77, 180);
Color console_transparent(77,77,77, 0);
Color console_fade(100, 100, 100, 100);	//fix this
Color console_cheap_line(255,255,255);


CLIENT_CONSOLE_COMMAND(toggleconsole)
{
	console.toggle();
}


Console::Console()
{
	cursor_pos = 0;
	isDown = false;
	position = 0;
	pending = "";
}


void Console::toggle(void)
{
	isDown = !isDown;
	line = "";
	
	if(isDown) this->enable();
}


void Console::println(const char *str)
{
	history.push_front( std::string(str) );
}

void Console::printf(const char *fmt, ...)
{
	char buf[512] = "";
	va_list args;
	va_start(args, fmt);
		vsnprintf(buf, 512, fmt, args);
	va_end(args);
	
	char *pos = strtok(buf, "\n");
	do {
		if(pos)
			history.push_front( std::string(pos) );
	} while((pos = strtok(NULL, "\n")));
}


bool Console::timepass(void)
{
	char k = 0;
	if(isDown && position < consoleHeight) {
		position += consoleSpeed*getRealDt();
	} else if(!isDown && position > -fade_height) {
		position -= consoleSpeed*getRealDt();
	}

	return false;
}

bool Console::handleEvent(SDL_Event *ev)
{
	if(!isDown)
		return false;
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

void Console::keypress(char key)
{
	switch(key)
	{
		case '`': case'~':
			toggle();
			break;
		case '\b':
			line = line.substr(0, line.length()-1);
			break;
		case '\n': case '\r':
			history.push_front(line);
			pending += line;
			line = "";
			if(clientScripting.isCompleteCommand(pending)) {
				command(pending);
				pending = "";
			} else {
				pending += "\n";
			}
			break;
		default:
			line += key;
			break;
	}
}

void Console::command(std::string cmd)
{
	clientScripting.command(cmd.c_str());
}

ClientConsoleVar<bool> cheapConsole("cheapconsole", false);

void Console::redraw(void)
{
//	if(position <= 0) remove();
	
	float draw_y = position - bottom_margin;
	// Draw background
	if(cheapConsole)
		graphics.drawLine(
			console_cheap_line, 
			0, 
			position, 
			graphics.getScreenWidth(), 
			position + fade_height);
	else
	{
		glBegin(GL_QUADS);
			graphics.setColor(console_bgcolor);
				glVertex2f(0, 0);
				glVertex2f(graphics.getScreenWidth(), 0);
				glVertex2f(graphics.getScreenWidth(), position);
				glVertex2f(0, position);
			
				glVertex2f(0, position);
				glVertex2f(graphics.getScreenWidth(), position);
			graphics.setColor(console_transparent);
				glVertex2f(graphics.getScreenWidth(), position+fade_height);
				glVertex2f(0, position+fade_height);
		glEnd();
	}

	// Draw current line
	graphics.drawText(line.c_str(), console_margin_left, draw_y);
	
	draw_y -= (text_height+line_margin);
	
	// Draw history lines
	for(historyList::iterator ii=history.begin(); ii!=history.end(); ii++) {
		//GameX.DrawText(console_margin_left, draw_y, (char*)(*ii).c_str(), 230, 230, 255);
		graphics.drawText((*ii).c_str(), console_margin_left, draw_y);
		if(draw_y < 0) break;
		draw_y -= text_height;
	}
}

