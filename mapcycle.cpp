#include "mastrix.hpp"


MapCycle::MapCycle()
{
	pos = 0;
}

std::string MapCycle::getNext(void)
{
	if(pos >= levels.size())
		pos = 0;
	if(pos >= levels.size())
		return "blank";
	return levels[pos++];
}

void MapCycle::addLevel(const char *lvl)
{
	levels.push_back(lvl);
}

void MapCycle::removeLevel(const char *lvl)
{
	int index = 0;
	for(std::vector<std::string>::iterator ii=levels.begin(); ii!=levels.end(); ii++, index++)
	{
		if(*ii == lvl) {
			levels.erase(ii);
			if(pos > index) pos--;
			break;
		}
	}
}

void MapCycle::clear(void)
{
	levels.clear();
	pos = 0;
}


SERVER_CONSOLE_COMMAND(addlevel)
{
	if(argc != 1) {
		printfToClient(who, "Usage: addlevel level");
		return;
	}
	server->cycle.addLevel(argv[0]);
}

SERVER_CONSOLE_COMMAND(removelevel)
{
	if(argc != 1) {
		printfToClient(who, "Usage: removelevel level");
		return;
	}
	server->cycle.removeLevel(argv[0]);
}

SERVER_CONSOLE_COMMAND(clearlevels)
{
	server->cycle.clear();
}

SERVER_CONSOLE_COMMAND(cycle)
{
	std::string nextmap = server->cycle.getNext();
	serverScripting.command( retprintf("map %s", nextmap.c_str()).c_str() );
}
