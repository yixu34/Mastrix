#include "mastrix.hpp"


Keyboard keyboard;

ClientConsoleVar<bool> show_keys("show_keys", false);

CLIENT_CONSOLE_COMMAND(bind)
{
	if(argc != 2) {
		console.printf("Usage: bind [key] [command]");
		return;
	}
	keyboard.bind(argv[0], argv[1]);
}
CLIENT_CONSOLE_COMMAND(unbind)
{
	if(argc != 1) {
		console.printf("Usage: unbind [key]");
		return;
	}
	keyboard.unbind(argv[0]);
}
CLIENT_CONSOLE_COMMAND(unbindall)
{
	keyboard.unbindall();
}

Keyboard::Keyboard()
{
}

void Keyboard::bind(const char *key, const char *action)
{
	std::string keyname(key);
	// Convert to all-uppercase
	for(unsigned ii=0; ii<keyname.length(); ii++)
		keyname[ii] = toupper(keyname[ii]);
	bindings[keyname] = std::string(action);
}

void Keyboard::unbind(const char *key)
{
	std::string keyname(key);
	// Convert to all-uppercase
	for(unsigned ii=0; ii<keyname.length(); ii++)
		keyname[ii] = toupper(keyname[ii]);
	bindings.erase(keyname);
}

void Keyboard::unbindall(void)
{
	bindings.clear();
}

void Keyboard::keydown(const char *key, int mod)
{
	std::string cmd = getCommand(key, mod);
	if(cmd != "")
		clientScripting.command( cmd.c_str() );
}


void Keyboard::keyup(const char *key, int mod)
{
	std::string cmd = getCommand(key, mod);
	if(cmd != "")
		clientScripting.invertCommand( cmd.c_str() );
}


std::string Keyboard::getCommand(const char *key, int mod)
{
	std::string keyname(key);
	
	// Convert to all-uppercase
	for(unsigned ii=0; ii<keyname.length(); ii++)
		keyname[ii] = toupper(keyname[ii]);
	
	std::string mod_keyname = keyname;
	
	if(mod & KMOD_SHIFT)
		mod_keyname = "SHIFT-"+mod_keyname;
	if(mod & KMOD_CTRL)
		mod_keyname = "CTRL-"+mod_keyname;
	if(mod & KMOD_ALT)
		mod_keyname = "ALT-"+mod_keyname;
	
	if(bindings.find(mod_keyname) != bindings.end())
		return bindings[mod_keyname];
	else if(bindings.find(keyname) != bindings.end())
		return bindings[keyname];
	else if(show_keys) {
		console.printf("Unrecognized key: '%s'", mod_keyname.c_str());
	}
	return "";
}
