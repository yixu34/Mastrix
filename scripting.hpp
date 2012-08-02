#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <stack>
#include <set>
#include <list>
#include "message.hpp"

class CallArgumentStack
{
public:
	void pushArguments(int argc, const char **argv);
	void popArguments(void);
	std::string getArgument(int index);
	int numArguments(void);
	
protected:
	struct ArgumentList {
		int argc;
		const char **argv;
	};
	std::stack<ArgumentList> arguments;
};

enum VarTag {
	tag_default         = -1,    // How this variable is tagged depends on context of the definition
	tag_normal          = 0,     // This variable just sticks around indefinitely
	tag_frame_temporary = 0x01,  // This variable stays defined for only one frame
	tag_map_temporary   = 0x02,  // This variable stays defined only until the map changes
	tag_sticky          = 0x04,  // Holds long-term state; should go in saved games
	tag_death_temporary = 0x08,  // Cleared when the player dies
};

class Scripting
{
public:
	Scripting(void) {shouldWalkTriggers = false;}
	virtual ~Scripting(void) {}
	
	void command(const char *command);
	void command(RecvMessage &mesg);
	void command(const char *command, bool inverted);
	void invertCommand(const char *command);
	virtual void runCommand(const char **tokens, int numTokens, int who) = 0;
	void runCommandInverted(const char **tokens, int numTokens, int who);
	
	bool isCompleteCommand(std::string cmd);
	
	void exec(const char *filename);
	inline void setTag(VarTag tag) { defaultTag = tag; }
	void defineVariable(std::string varname, std::string value, VarTag tag=tag_default);
	void tagVariable(std::string varname, VarTag tag=tag_sticky);
	void undefineVariable(std::string varname);
	void clearTaggedVariables(VarTag tag);
	void defineAlias(std::string name, std::string body);
	std::string lookupVariable(std::string varname);
	void defineTrigger(unsigned numConditions, const char*const*conditions, const char *action);
	void clearTriggers(void);
	
	bool shouldWalkTriggers;
	void walkTriggers(void);
	
protected:
	class MismatchedQuotes
	{};
	struct Trigger
	{
		typedef std::set<std::string> ConditionSet;
		ConditionSet conditions;
		std::string action;
		float definitionTime;
	};
	
	void substituteVarname(std::string &str, unsigned &parent_pos, bool useEscapes);
	std::string readQuoted(std::string source, unsigned &pos, bool substitute, bool interpret_escapes, bool force_substitute=false);
	static char interpretBackslash(const std::string &str, unsigned &pos);
	static char interpretBackslash(const char **pos);
	void singleCommand(const char *command, bool inverted);
	
	CallArgumentStack arguments;
	VarTag defaultTag;
	typedef std::map<std::string, VarTag> TagMap;
	TagMap vartags;
	std::map<std::string, std::string> variables;
	std::map<std::string, std::string> aliases;
	typedef std::list<Trigger*> TriggerList;
	TriggerList triggers;
};

class ClientScripting
	: public Scripting
{
public:
	ClientScripting(void) {}
	void runCommand(const char **tokens, int numTokens, int who);
};

class ServerScripting
	: public Scripting
{
public:
	ServerScripting(void) {}
	void runCommand(const char **tokens, int numTokens, int who);
};

extern ClientScripting clientScripting;
extern ServerScripting serverScripting;

#endif
