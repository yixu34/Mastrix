//#include "command.hpp"
#include "mastrix.hpp"

ConsoleCommandPool *clientCommandPool = NULL,
                   *serverCommandPool = NULL;

void ConsoleCommandPool::registerCommand(BaseConsoleCommand *cmd)
{
	commands[ cmd->getName() ] = cmd;
}

BaseConsoleCommand *ConsoleCommandPool::lookup(const char *name)
{
	std::string strname(name);
	if( commands.find(strname) == commands.end() )
		return NULL;
	else
		return commands[strname];
}

BaseConsoleCommand::BaseConsoleCommand(ConsoleCommandPool *&pool)
{
	if(pool == NULL) {
		pool = new ConsoleCommandPool;
	}
}

BaseConsoleCommand::~BaseConsoleCommand()
	{ }


