#include "mastrix.hpp"
#include <string>
#include <vector>
#include <cassert>
using namespace std;

ClientScripting clientScripting;
ServerScripting serverScripting;

void Scripting::command(const char *cmd)
	{ command(cmd, false); }
void Scripting::invertCommand(const char *cmd)
	{ command(cmd, true); }

void Scripting::command(RecvMessage &mesg)
{
	int numTokens = mesg.getShort();
	string *str_tokens = new string[numTokens];
	for(int ii=0; ii<numTokens; ii++)
		str_tokens[ii] = mesg.getString();
	const char **tokens = new const char*[numTokens];
	for(int ii=0; ii<numTokens; ii++)
		tokens[ii] = str_tokens[ii].c_str();
	
	runCommand(tokens, numTokens, mesg.getSource());
	
	delete[] tokens;
	delete[] str_tokens;
}

static std::string stripWS(std::string str)
{
	unsigned start=0, end=str.length()-1;
	while(start+1 < str.length() && isspace(str[start]))
		start++;
	while(end > 0 && isspace(str[end]))
		end--;
	if(end>=start)
		return str.substr(start, end-start+1);
	else
		return "";
}

static void doLet(Scripting *where, int argc, const char *const *argv)
{
	std::string combined_string("");
	
	for(int ii=0; ii<argc; ii++) {
		if(ii>0) combined_string += " ";
		combined_string += argv[ii];
	}
	for(unsigned ii=0; ii<combined_string.length(); ii++) {
		if(combined_string[ii] == '=') {
			combined_string[ii] = '\0';
			where->defineVariable(
				stripWS(combined_string.c_str()),
				stripWS(combined_string.c_str() + ii + 1)
				);
			return;
		}
	}
	
	// There was no = sign in the arguments, so instead assume it was of the
	// form "let var value" and catenate everything but the first argument.
	std::string value("");
	for(int ii=1; ii<argc; ii++)
	{
		if(ii>0) combined_string += " ";
		value += argv[ii];
	}
	where->defineVariable(argv[0], value);
}


std::string Scripting::readQuoted(std::string source, unsigned &pos, bool substitute, bool interpret_escapes, bool force_substitute)
{
	char close_quote;
	bool follow_nested = true;
	std::string ret("");
	std::string str(source);
	int pos_offset = 0;
	
	assert(str.length() > pos);
	
	switch(str[pos++]) {
		case '\'': close_quote = '\''; substitute = false; follow_nested=false; break;
		case '\"': close_quote = '\"';                                          break;
		case '{':  close_quote = '}';  substitute = false;                      break;
		default:
			assert(0); // Should never get here, because that would mean this
			           // was called on something that's not a quoted string.
			return "";
	}
	if(force_substitute)
		substitute = true; // In some contexts, substitute even in '{' or '\''.
	
	while(pos < str.length())
	{
		if(str[pos] == close_quote) {
			pos++;
			pos += pos_offset;
			return ret;
		}
		switch(str[pos])
		{
			case '\"':
			case '\'':
			case '{':
				if(follow_nested)
				{
					// Skip over the nested string, but don't actually use the (quote-
					// stripped) parsed result.
					unsigned old_pos = pos;
					readQuoted(str, pos, substitute, interpret_escapes);
					ret += str.substr(old_pos, pos-old_pos);
				}
				else
					ret += str[pos++];
				break;
			case '\\':
				if(interpret_escapes)
					ret += interpretBackslash(str, pos);
				else {
					// Skip over the escaped chars, but don't actually use the interpretted
					// result (so they stay escaped).
					unsigned old_pos = pos;
					interpretBackslash(str, pos);
					ret += str.substr(old_pos, pos-old_pos);
				}
				break;
			case '$':
				if(substitute)
				{
					int old_len = str.length();
					substituteVarname(str, pos, true);
					pos_offset += old_len - str.length();
					break;
				}
				/* FALLTHROUGH */
			default:
				ret += str[pos++];
				break;
		}
	}
	throw MismatchedQuotes();
}


bool Scripting::isCompleteCommand(std::string cmd)
{
	if(cmd.length()>0 && cmd[cmd.length()-1] == '\\')
		return false;
	if(cmd.length()>1 && (cmd[cmd.length()-1] == '\n' || cmd[cmd.length()-1] == '\r') && cmd[cmd.length()-2] == '\\')
		return false;
	
	unsigned pos = 0;
//	char close_quote;
	while(pos<cmd.length()) switch(cmd[pos])
	{
		case '\'': case '\"': case '{':
			try {
				readQuoted(cmd, pos, false, false);
			} catch(MismatchedQuotes) {
				return false;
			}
			break;
		case '\\':
			interpretBackslash(cmd, pos);
			break;
		default:
			pos++;
			break;
	}

	return true;
}


void Scripting::defineVariable(std::string varname, std::string value, VarTag tag)
{
	bool checkTriggers = (variables.find(varname)==variables.end());
	variables[varname] = value;
	
	if(tag != tag_normal && !(vartags[varname]&tag_sticky)) {
		if(tag == tag_default)
			vartags[varname] = defaultTag;
		else
			vartags[varname] = tag;
	}
	
	if(!checkTriggers)
		return;
	walkTriggers();
}

void Scripting::walkTriggers(void)
{
	shouldWalkTriggers = false;
	
	for(TriggerList::iterator ii=triggers.begin(); ii!=triggers.end();)
	{
		bool shouldFire = true;
		for(Trigger::ConditionSet::iterator jj=(*ii)->conditions.begin(); jj!=(*ii)->conditions.end(); jj++)
		{
			if(variables.find(*jj)==variables.end()) {
				shouldFire = false;
				break;
			}
		}
		if(shouldFire)
		{
			if((*ii)->definitionTime+0.0001 >= getTime()) {
				shouldWalkTriggers = true;
				break;
			} else {
				Trigger *trigger = *ii;
				triggers.erase(ii);
				command(trigger->action.c_str());
				delete trigger;
				ii = triggers.begin();
			}
		}
		else
			ii++;
	}
}

void Scripting::tagVariable(std::string varname, VarTag tag)
{
	vartags[varname] = tag;
}

void Scripting::undefineVariable(std::string varname)
{
	if(variables.find(varname) != variables.end())
		variables.erase(varname);
	if(vartags.find(varname) != vartags.end())
		vartags.erase(varname);
}

void Scripting::clearTaggedVariables(VarTag tag)
{
	std::vector<std::string> toErase;
	
	for(TagMap::iterator ii=vartags.begin(); ii!=vartags.end(); ii++)
	{
		if(ii->second & tag) {
			toErase.push_back(ii->first);
		}
	}
	for(unsigned int ii=0; ii<toErase.size(); ii++)
	{
		std::string s = toErase[ii];
		assert(variables.find(s) != variables.end());
		assert(vartags.find(s) != vartags.end());
		vartags.erase(s);
		variables.erase(s);
	}
}

void Scripting::defineAlias(std::string name, std::string body)
{
	aliases[name] = body;
}

void Scripting::defineTrigger(unsigned numConditions, const char*const*conditions, const char *action)
{
	bool fire_now = true;
	Trigger *trigger = new Trigger;
	trigger->action = action;
	trigger->definitionTime = getTime();
	for(unsigned ii=0; ii<numConditions; ii++) {
		if(variables.find(conditions[ii]) == variables.end())
			fire_now = false;
		trigger->conditions.insert( std::string(conditions[ii]) );
	}
	if(fire_now)
		shouldWalkTriggers = true;
	triggers.push_back(trigger);
}

void Scripting::clearTriggers(void)
{
	triggers.clear();
}

std::string Scripting::lookupVariable(std::string varname)
{
	if(variables.find(varname) == variables.end())
		return "";
	else
		return variables[varname];
}


// Substitute $var -> value in str, where str[pos]='$'.
void Scripting::substituteVarname(std::string &str, unsigned &parent_pos, bool useEscapes)
{
	std::string varname;
	std::string replacement;
	int pos = parent_pos+1;
	int replacement_length = 0;
	static int uniqueVarnum = 0;
	
	if(isdigit(str[pos])) // var is a number (argument)
	{
		while(isdigit(str[pos]))
			varname += str[pos++];
		int argIndex = atoi(varname.c_str()) - 1;
		if(argIndex < 0) argIndex = 0;
		replacement = arguments.getArgument(argIndex);
	} else if(str[pos]=='*') { // *all* arguments
		varname += '*';
		for(int ii=0; ii<arguments.numArguments(); ii++)
			replacement += arguments.getArgument(ii) + " ";
	} else if(str[pos]=='$') {
		varname += '$';
		replacement = retprintf("__var%i", ++uniqueVarnum);
	} else if(str[pos]=='!') {
		varname += '!';
		replacement = retprintf("__var%i", uniqueVarnum);
	} else if(str[pos]=='{') {
		// Evaluate the contents of the curlies and use that as a variable name
		unsigned quoteread_pos = pos;
		varname = readQuoted(str, quoteread_pos, true, true, true);
		replacement = lookupVariable(varname);
		replacement_length = quoteread_pos - parent_pos;
	} else {
		while(isdigit(str[pos]) || isalpha(str[pos]) || str[pos]=='_')
			varname += str[pos++];
		replacement = lookupVariable(varname);
	}
	
	if(useEscapes) {
		replacement = quotify(replacement);
	}
	if(!replacement_length)
		replacement_length = varname.length()+1;
	str.replace(parent_pos, replacement_length, replacement);
}

// Translate $vars, break up where there are semicolons, and pass the resulting
// commands to singleCommand.
void Scripting::command(const char *cmd, bool inverted)
{
//	std::string cmd_copy(cmd);
	unsigned pos = 0;
	int last_cmd = 0;
	
	std::string cmd_str(cmd);
	
	while(pos < cmd_str.length())
	{
		switch(cmd_str[pos])
		{
			case '\'': case '\"': case '{':
				try {
					readQuoted(cmd_str, pos, false, false);
				} catch(MismatchedQuotes) {
					return;
				}
				break;
			case '\\':
				interpretBackslash(cmd_str, pos);
				break;
			case ';':
				singleCommand(cmd_str.substr(last_cmd, pos-last_cmd).c_str(), inverted);
				last_cmd = pos+1;
				pos++;
				break;
			default:
				pos++;
				break;
		}
	}
	
	singleCommand(cmd_str.substr(last_cmd, pos-last_cmd).c_str(), inverted);
}

void Scripting::singleCommand(const char *command, bool inverted)
{
	unsigned ii;
	
	// Tokenize the command
	vector<string> tokens;
	string current_token;
	
	std::string cmd_str(command);
	unsigned pos = 0;
	while(pos < cmd_str.length())
	{
		switch(cmd_str[pos])
		{
			// All non-special characters just get dumped into the current tok
			default:
				current_token += cmd_str[pos++];
				break;
			
			// Whitespace starts a new token
			case ' ': case '\t': case '\n': case '\r':
				if(current_token.length() > 0) {
					tokens.push_back(current_token);
					current_token = "";
				}
				pos++;
				break;
			
			// # marks a comment continuing till the end of the line
			case '#':
				while(pos < cmd_str.length() && cmd_str[pos] != '\n' && cmd_str[pos] != '\r')
					pos++;
				break;
			
			// Quotes group text into a single token.
			case '\"': case '\'': case '{':
				try {
					current_token += readQuoted(cmd_str, pos, true, true);
				} catch(MismatchedQuotes) {
					return;
				}
				break;
			
			case '$':
				substituteVarname(cmd_str, pos, false);
				break;
				
			// Interpret backslash as an escape character
			case '\\':
				current_token += interpretBackslash(cmd_str, pos);
				break;
		}
	}
	// Add the last token (treat the end-of-string as a delimiter)
	if(current_token.length() > 0) {
		tokens.push_back(current_token);
		current_token = "";
	}
	
	// Sanity-check this result
	if(tokens.size() == 0)
		return;
	
	// If this is to be inverted, change '+' to '-' in the command token
	// If there is no '+' starting the first token, then the command is
	// non-invertible, so it shouldn't be executed at all.
	if(inverted)
	{
		if(tokens[0][0]=='+')
			tokens[0][0] = '-';
		else
			return;
	}
	
	// Convert from vector<string> to a more natural const char**
	const char **str_tokens = new const char*[tokens.size()];
	for(ii=0; ii<tokens.size(); ii++)
		str_tokens[ii] = tokens[ii].c_str();
	
	runCommand(str_tokens, tokens.size(), currentNode->getNodeId());
	
	delete[] str_tokens;
}

void Scripting::runCommandInverted(const char **tokens, int numTokens, int who)
{
	if(tokens[0][0] != '+')
		return;
	
	char *reversed_token = strdup(tokens[0]);
	reversed_token[0] = '-';
	const char **tokens_copy = new const char*[numTokens];
	tokens_copy[0] = reversed_token;
	for(int ii=1; ii<numTokens; ii++)
		tokens_copy[ii] = tokens[ii];
	runCommand(tokens_copy, numTokens, who);
	delete reversed_token;
	delete tokens_copy;
}

void ClientScripting::runCommand(const char **tokens, int numTokens, int who)
{
	if(aliases.find(std::string(tokens[0])) != aliases.end())
	{
		arguments.pushArguments(numTokens-1, tokens+1);
		command(aliases[tokens[0]].c_str());
		arguments.popArguments();
		return;
	}
	
	BaseConsoleVar *v = currentNode->cvars->lookup(tokens[0]);
	BaseConsoleCommand *cmd = currentNode->commands->lookup(tokens[0]);
	if(v != NULL) {
		// If no parameter, print the cvar's value.
		// If one parameter, change the cvar's value.
		// If multiple parameters, complain.
		if(numTokens == 1) {
			console.println(v->valueString().c_str());
		} else if(numTokens == 2) {
			v->valueFromString(tokens[1]);
		} else {
			console.println("Cannot give multiple parameters when setting a cvar."
			                " (Did you mean to put that in quotes?");
		}
	} else if(cmd != NULL) {
		cmd->execute(numTokens-1, tokens+1, -1);
	} else {
		// Attempt to execute as a server command
		// The server will send back any results which need to go to the
		// console, including notification if the command was invalid. This is
		// sent *tokenized*.
		SendMessage msg(message_command);
			// Number of tokens
			msg << (short)numTokens;
			// Tokens themselves
			for(int ii=0; ii<numTokens; ii++)
				msg << tokens[ii];
		msg.sendToServer();
	}
}

void ServerScripting::runCommand(const char **tokens, int numTokens, int who)
{
	if(aliases.find(std::string(tokens[0])) != aliases.end())
	{
		arguments.pushArguments(numTokens-1, tokens+1);
		command(aliases[tokens[0]].c_str());
		arguments.popArguments();
		return;
	}
	
	BaseConsoleVar *v = server->cvars->lookup(tokens[0]);
	BaseConsoleCommand *cmd = server->commands->lookup(tokens[0]);
	if(v != NULL) {
		// If no parameter, print the cvar's value.
		// If one parameter, change the cvar's value.
		// If multiple parameters, complain.
		if(numTokens == 1) {
			printfToClient(who, "%s", v->valueString().c_str());
		} else if(numTokens == 2) {
			v->valueFromString(tokens[1]);
		} else {
			printfToClient(who, "Cannot give multiple parameters when setting a cvar."
			                    " (Did you mean to put that in quotes?");
		}
	} else if(cmd != NULL) {
		cmd->execute(numTokens-1, tokens+1, who);
	} else {
		printfToClient(who, "Unknown command: '%s'", tokens[0]);
	}
}


void Scripting::exec(const char *filename)
{
	char buf[512];
	std::string pending = "";
	FILE *fin;
	
	fin = fopen(filename, "r");
	if(!fin) {
		// FIXME: Print to client
		console.printf("File '%s' not found.", filename);
		return;
	}
	
	// While (more lines in file)
	while(!feof(fin))
	{
		// Read line. (NULL from fgets means nothing was read)
		if(fgets(buf, 512, fin) == NULL)
			break;
		pending += buf;
		
		// Execute line
		if(isCompleteCommand(pending)) {
			command(pending.c_str());
			pending = "";
		}
	}
	fclose(fin);
	
	command(pending.c_str());
}


char Scripting::interpretBackslash(const std::string &str, unsigned &pos)
{
	const char *c_str = str.c_str();
	const char *ptr_pos = c_str + pos;
	char ret = interpretBackslash(&ptr_pos);
	pos = ptr_pos - c_str;
	return ret;
}

char Scripting::interpretBackslash(const char **pos)
{
	const char *p = *pos;
	char ret = '?';
	
	assert(*p == '\\');
	p++;
	
	switch(*p) {
		// Treat backslash on unrecognized characters as a no-op.
		default:   p++; ret = *p;   break;
		
		// These match up with C escape sequences
		case 'a':  p++; ret = '\a'; break;
		case 'b':  p++; ret = '\b'; break;
		case 'f':  p++; ret = '\f'; break;
		case 'n':  p++; ret = '\n'; break;
		case 'r':  p++; ret = '\r'; break;
		case 't':  p++; ret = '\t'; break;
		case 'v':  p++; ret = '\v'; break;
		case '\\': p++; ret = '\\'; break;
		case '?':  p++; ret = '\?'; break;
		case '\'': p++; ret = '\''; break;
		case '"':  p++; ret = '\"'; break;
		case '\n': p++; ret = ' ';  break;
		case '{':  p++; ret = '{';  break;
		case '}':  p++; ret = '}';  break;
		case '$':  p++; ret = '$';  break;
		
		// Hex numbers of the form \xhh.
		case 'x':
			p++; // Skip over the x
			
			// First digit
			if(*p >= '0' && *p <= '9')      // 0-9
				ret = *p - '0';
			else if(*p >= 'a' && *p <= 'f') // a-f
				ret += *p - 'a';
			else if(*p >= 'A' && *p <= 'F') // A-F
				ret += *p - 'A';
			else
				break; // If it wasn't [0-9a-fA-F], stop here (don't advance).
			p++;
			
			// Second digit
			if(*p >= '0' && *p <= '9')
				ret = (ret<<4) + (*p-'0');
			else if(*p >= 'a' && *p <= 'f')
				ret += (ret<<4) + (*p-'a');
			else if(*p >= 'A' && *p <= 'F')
				ret += (ret<<4) + (*p-'A');
			else
				break;
			p++;
			break;
		
		// Octal numbers of the form \x0, \x00, or \x000.
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			// First digit
			ret = *p - '0';
			p++;
			
			// Second digit
			if(*p < '0' || *p > '7')
				break;
			ret = (ret<<3) + *p - '0';
			p++;
			
			// Third digit
			if(*p < '0' || *p > '7')
				break;
			ret = (ret<<3) + *p - '0';
			p++;
			
			break;
	}
	
	*pos = p;
	return ret;
}



void CallArgumentStack::pushArguments(int argc, const char **argv)
{
	ArgumentList addition = {argc, argv};
	arguments.push(addition);
}

void CallArgumentStack::popArguments(void)
{
	arguments.pop();
}

std::string CallArgumentStack::getArgument(int index)
{
	ArgumentList current = arguments.top();
	if(index >= current.argc)
		return "";
	else
		return std::string(current.argv[index]);
}

int CallArgumentStack::numArguments(void)
{
	ArgumentList current = arguments.top();
	return current.argc;
}


/////////////////////////////////////////////////////////////////////////////
//                     Scripting Meta-commands                             //
/////////////////////////////////////////////////////////////////////////////

/**
 * sv [command]: Run a console command from the server's context.
 * This is useful for commands where it would be ambiguous whether it should
 * be interpretted by the client, or by the server, such as 'let' and 'fun'.
 */
SERVER_CONSOLE_COMMAND(sv)
{
	serverScripting.runCommand(argv, argc, who);
}

/**
 * ifdef [var] [command]: Run a command if and only if [var] is defined.
 */
SERVER_CONSOLE_COMMAND(ifdef)
{
	if(argc < 2) {
		printfToClient(who, "Usage: ifdef [var] [command]");
		return;
	}
	if(serverScripting.lookupVariable(std::string(argv[0])) != "")
		serverScripting.runCommand(argv+1, argc-1, who);
}

/**
 * ifeq [var] [var] [command]: Run a command if and only if the arguments are equal
 */
SERVER_CONSOLE_COMMAND(ifeq)
{
	if(argc < 3) {
		printfToClient(who, "Usage: ifeq a b [command]");
		return;
	}
	if(serverScripting.lookupVariable(std::string(argv[0])) == serverScripting.lookupVariable(std::string(argv[1])))
		serverScripting.runCommand(argv+2, argc-2, who);
}

/**
 * ifneq [var] [var] [command]: Run a command unless the arguments are equal
 */
SERVER_CONSOLE_COMMAND(ifneq)
{
	if(argc < 3) {
		printfToClient(who, "Usage: ifneq a b [command]");
		return;
	}
	if(serverScripting.lookupVariable(std::string(argv[0])) != serverScripting.lookupVariable(std::string(argv[1])))
		serverScripting.runCommand(argv+2, argc-2, who);
}

/**
 * ifgt a b [command]: Run a command if a>b
 */
SERVER_CONSOLE_COMMAND(ifgt)
{
	if(argc < 3) {
		printfToClient(who, "Usage: ifneq a b [command]");
		return;
	}
	float a = atof(argv[0]),
	      b = atof(argv[1]);
	if(a>b)
		serverScripting.runCommand(argv+2, argc-2, who);
}



/**
 * sticky [var]: Make var persistent
 */
SERVER_CONSOLE_COMMAND(sticky)
{
	if(argc < 1) {
		printfToClient(who, "Usage: sticky [var]");
		return;
	}
	serverScripting.tagVariable(argv[0], (VarTag)(tag_death_temporary|tag_sticky));
}

/**
 * add [var] [values]: Sum up values and store into var
 */
SERVER_CONSOLE_COMMAND(add)
{
	if(argc < 2) {
		printfToClient(who, "Usage: add [var] [values]");
		return;
	}
	float result = 0;
	for(int ii=1; ii<argc; ii++)
		result += atof(argv[ii]);
	serverScripting.defineVariable(argv[0], retprintf("%f", result));
}


/**
 * ifndef [var] [command]: Run a command unless [var] is defined.
 */
SERVER_CONSOLE_COMMAND(ifndef)
{
	if(serverScripting.lookupVariable(std::string(argv[0])) == "")
		serverScripting.runCommand(argv+1, argc-1, who);
}

SERVER_CONSOLE_COMMAND(ifempty)
{
	if(argc < 3) {
		printfToClient(who, "Usage: ifempty x y cmd");
		return;
	}
	float x = atof(argv[0]),
	      y = atof(argv[1]);
	for(Server::Entpool::iterator ii=server->entities.begin(); ii!=server->entities.end(); ii++)
	{
		if(distSquaredBetween(ii->second->getPosition(), Position(x,y)) < 100)
			return;
	}
	serverScripting.runCommand(argv+2, argc-2, who);
}


/**
 * cl [command]: Run a console command on every client.
 */
SERVER_CONSOLE_COMMAND(cl)
{
	SendMessage cmd(message_command);
		cmd << (short)argc;
		for(int ii=0; ii<argc; ii++)
			cmd << argv[ii];
	cmd.sendToEveryone();
}


/**
 * svstr '[command]': Parse and run a console command from the server's context.
 * Useful where parsing depends on variables defined in the server's context.
 */
SERVER_CONSOLE_COMMAND(svstr)
{
	std::string str("");
	for(int ii=0; ii<argc; ii++)
		str += std::string(argv[ii]) + ' ';
	serverScripting.command(str.c_str());
}

/**
 * Three different syntaxes accepted:
 *  let var
 *  let var val
 *  let var=val
 * Define var equal to val, so that in the future $var will be replaced with
 * val. Whitespace is allowed on either side of the = sign. If val is not given,
 * var is defined to the empty string.
 */
CLIENT_CONSOLE_COMMAND(let)
{
	if(argc == 0) {
		console.printf("Usage: let variable [= value]");
		return;
	}
	doLet(&clientScripting, argc, argv);
}
SERVER_CONSOLE_COMMAND(let)
{
	if(argc == 0) {
		printfToClient(who, "Usage: let variable [= value]");
		return;
	}
	doLet(&serverScripting, argc, argv);
}

/**
 * set: equivalent to 'let' (above).
 */
CLIENT_CONSOLE_COMMAND(set)
{
	if(argc == 0) {
		console.printf("Usage: set variable [= value]");
		return;
	}
	doLet(&clientScripting, argc, argv);
}
SERVER_CONSOLE_COMMAND(set)
{
	if(argc == 0) {
		printfToClient(who, "Usage: set variable [= value]");
		return;
	}
	doLet(&serverScripting, argc, argv);
}
SERVER_CONSOLE_COMMAND(unset)
{
	if(argc != 1) {
		printfToClient(who, "Usage: unset variable");
		return;
	}
	serverScripting.undefineVariable(argv[0]);
}

/**
 * fun [name] [body]
 * Define a function (alias). When [name] is used as a console command, [body]
 * will be executed instead. Note that if curly braces are used, as in
 *   fun [name] {
 *     [body]
 *   }
 * the opening curly brace MUST be on the same line, or else the body will not
 * be interpretted as part of the function.
 */
CLIENT_CONSOLE_COMMAND(fun)
{
	if(argc < 2) {
		console.printf("Usage: fun [name] [body]");
		return;
	}
	clientScripting.defineAlias(argv[0], argv[1]);
}

SERVER_CONSOLE_COMMAND(fun)
{
	if(argc < 2) {
		console.printf("Usage: fun name body");
		return;
	}
	serverScripting.defineAlias(argv[0], argv[1]);
}

/**
 * trigger action condition*
 *   Specify a console command which will be executed when all of the
 * conditions are set.
 */
SERVER_CONSOLE_COMMAND(trigger)
{
	if(argc < 2) {
		console.printf("Usage: trigger action condition");
		return;
	}
	serverScripting.defineTrigger(argc-1, argv+1, argv[0]);
}

/**
 * exec [filename]
 * Runs all of the commands appearing in [filename].
 */
CLIENT_CONSOLE_COMMAND(exec)
{
	if(argc != 1)
		console.printf("Usage: exec [filename]");
	else
		clientScripting.exec(argv[0]);
}

SERVER_CONSOLE_COMMAND(exec)
{
	if(argc != 1)
		printfToClient(who, "Usage: exec [filename]");
	else
		serverScripting.exec(argv[0]);
}
