#include "mastrix.hpp"
#include <deque>
#include "time.h"	//to seed the random number generator

using namespace std;

const char *const programName = "Mastrix";

ClientPool clients;
Server *server;
GameNode *currentNode;
//std::deque<Mouseclick> mouseBuffer;

static void initialize();
static void shutDown();
static void eventLoop();
static void initCommandLineArgs(int argc, char **argv);

int main(int argc, char **argv)
{	
	initialize();
	initCommandLineArgs(argc, argv);
	initNetwork();
	
	showMainMenu();
	eventLoop();

	return 0;
}

static void initialize()
{
	srand(time(0));	//seed the random number generator

	graphics.initialize(1024, 768);
	audio.initialize();
	atexit(shutDown);
	setSoundtrack("sounds/MastrixTheme.mp3", false);

	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(1);	
	SDL_WM_SetCaption(programName, programName);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

static void eventLoop()
{
	SDL_Event event;
	
	while(1)
	{
		updateTime();
		
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_QUIT)
				exit(0);
			
			evHandle(&event);
		}
		
		evTimepass();
		
		graphics.clearScreen();
		evRedraw();
		graphics.swapBuffers();
		audio.update();
	}
}

static void shutDown()
{
	graphics.shutDown();
	audio.shutdown();
}

static void initCommandLineArgs(int argc, char **argv)
{
/*	for(int ii=1; ii<argc; ii++)
	{
		if(!strcmp(argv[ii], "-server") || !strcmp(argv[ii], "-listen")) {
			networkServer = true;
		} else if(!strcmp(argv[ii], "-dedicated"))
			networkDedicatedServer = networkServer = true;
		else if(!strcmp(argv[ii], "-single")) {
			networkServer = networkDedicatedServer = false;
		}
		else if(!strcmp(argv[ii], "-connect")) {
			ii++;
			if(ii>=argc) {
				fprintf(stderr, "Must give an IP address with -connect.\n");
				exit(1);
			}
			networkClient = true;
			networkServer = false;
			networkAddress = argv[ii];
		}
	}*/
	networkClient = networkServer = networkDedicatedServer = false;
}

SERVER_CONSOLE_COMMAND(addclient)
{
	ClientPool::iterator ii = clients.end();
	ii--;
	int max_clientID = ii->first;
	GameNode *prev_currentNode = currentNode;
	
	max_clientID++;
	currentNode = clients[max_clientID] = new Client(max_clientID);
	clients[max_clientID]->connect();
	
	currentNode = prev_currentNode;
	
	assignViewports();
}


MainGameHandler mainGameHandler;

bool MainGameHandler::handleEvent(SDL_Event *ev)
{
	switch (ev->type)
	{
		case SDL_KEYDOWN:
			keyboard.keydown(SDL_GetKeyName(ev->key.keysym.sym), ev->key.keysym.mod);
			return true;

		case SDL_KEYUP:
			keyboard.keyup(SDL_GetKeyName(ev->key.keysym.sym), ev->key.keysym.mod);
			return true;

		case SDL_MOUSEBUTTONDOWN:
			keyboard.keydown(buttonName(ev->button.button), 0);
			return true;

		case SDL_MOUSEBUTTONUP:
			keyboard.keyup(buttonName(ev->button.button), 0);
			return true;

		default:
			return false;
	}
}

void MainGameHandler::redraw(void)
{
	if(!networkDedicatedServer)
	{
		for(ClientPool::iterator ii=clients.begin(); ii!=clients.end(); ii++) {
			currentNode = ii->second;
			ii->second -> redraw();
		}
	}
}

bool MainGameHandler::timepass(void)
{
	stats.nextFrame();
	
	// Network stuff
	if(networkServer)
		acceptConnections();
	
	// Server Timepass
	if(!networkClient) {
		currentNode = server;
		server->timepass();
	}
	
	if(!networkDedicatedServer)
	{
		// Client timepass
		for(ClientPool::iterator ii=clients.begin(); ii!=clients.end(); ii++) {
			currentNode = ii->second;
			currentNode->timepass();
		}
	}
	if(newClient) {
		for(ClientPool::iterator ii=clients.begin(); ii!=clients.end(); ii++)
			delete ii->second;
		clients.clear();
		clients[0] = newClient;
		currentNode = clients[0];
		assignViewports();
		currentNode = clients[0];
		clientScripting.exec("config.cfg");
		newClient = NULL;
	}
	
	return false;
}

void MainGameHandler::onEnable(void)
{
	if(needSinglePlayerInit) {
		clientScripting.command("sv exec singleplayer.cfg");
		needSinglePlayerInit = false;
	}
}
