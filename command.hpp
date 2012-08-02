#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <map>
#include <string>

class ConsoleCommandPool;
class BaseConsoleCommand;

class ConsoleCommandPool
{
public:
	void registerCommand(BaseConsoleCommand *cmd);
	BaseConsoleCommand *lookup(const char *name);
	
	typedef std::map<std::string, BaseConsoleCommand*> commandMap;
	commandMap commands;
};

extern ConsoleCommandPool *clientCommandPool, *serverCommandPool;

#define COMMAND_ARG_LIST int argc, const char **argv, int who

class BaseConsoleCommand
{
public:
	BaseConsoleCommand(ConsoleCommandPool *&pool);
	virtual ~BaseConsoleCommand();
	typedef const char *const *paramList;
	
	inline std::string getName(void) const
		{ return name; };
	virtual void execute(COMMAND_ARG_LIST) = 0;
protected:
	std::string name;
};


#define CONSOLE_COMMAND(cmd_name, cmd_string, cmd_pool)    \
	class ConsoleCommand_##cmd_name                        \
		: public BaseConsoleCommand                        \
	{                                                      \
	public:                                                \
		ConsoleCommand_##cmd_name(void) :                  \
			BaseConsoleCommand(cmd_pool)                   \
		{                                                  \
			this->name = cmd_string;                       \
			(cmd_pool)->registerCommand(this);             \
		}                                                  \
		void execute(COMMAND_ARG_LIST);                    \
	} command_##cmd_name;                                  \
	void ConsoleCommand_##cmd_name::execute(COMMAND_ARG_LIST)

#define CLIENT_CONSOLE_COMMAND(cmd_name) \
	CONSOLE_COMMAND(cmd_name##_client, #cmd_name, clientCommandPool)
#define SERVER_CONSOLE_COMMAND(cmd_name) \
	CONSOLE_COMMAND(cmd_name##_server, #cmd_name, serverCommandPool)

#define INVERTIBLE_CONSOLE_COMMAND(cmd_name, cmd_pool) \
	CONSOLE_COMMAND(cmd_name##_plus, "+" #cmd_name, cmd_pool)
#define INVERSE_CONSOLE_COMMAND(cmd_name, cmd_pool) \
	CONSOLE_COMMAND(cmd_name##_minus, "-" #cmd_name, cmd_pool)

#define CLIENT_INVERTIBLE_CONSOLE_COMMAND(cmd_name) \
	INVERTIBLE_CONSOLE_COMMAND(cmd_name, clientCommandPool)
#define CLIENT_INVERSE_CONSOLE_COMMAND(cmd_name) \
	INVERSE_CONSOLE_COMMAND(cmd_name, clientCommandPool)

#define SERVER_INVERTIBLE_CONSOLE_COMMAND(cmd_name) \
	INVERTIBLE_CONSOLE_COMMAND(cmd_name, serverCommandPool)
#define SERVER_INVERSE_CONSOLE_COMMAND(cmd_name) \
	INVERSE_CONSOLE_COMMAND(cmd_name, serverCommandPool)

#endif
