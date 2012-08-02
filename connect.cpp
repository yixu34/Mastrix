#include "mastrix.hpp"
#include <set>

bool networkServer = true;
bool networkDedicatedServer = false;
bool networkClient = false;
const char *networkAddress = NULL;
const int defaultServerPort = 34572;
int serverPort = defaultServerPort;;
NLsocket serverSock;

typedef std::set<int> RemoteClientPool;
RemoteClientPool remoteClients;


NetworkNode *myself(void)
{
	return currentNode;
}

void SendMessage::sendToClient(int client)
{
	// If this is a local client
	if(clients.find(client) != clients.end()) {
		if(enableNetworkStatsLogging) {
			// Locally, server->client messages count as "sent" and client->server as
			// "received". (It's from the server's perspective.)
			stats.messageSent(getType(), getNetLength());
		}
		clients[client]->addMessage(this);
	} else if(networkServer)
		sendToAddress(this, client);
}

void SendMessage::sendToServer(void)
{
	if(!networkClient) {
		if(enableNetworkStatsLogging) {
			stats.messageReceived(getType(), getNetLength());
		}
		server->addMessage(this);
	} else
		sendToAddress(this, -1);
}

void SendMessage::sendToEveryone(void)
{
	NetworkNode *skipnode = myself();
	
	if(networkClient)
	{
		sendToAddress(this, -1);
	} else {
		if(skipnode != server) {
			server->addMessage(this);
			
			if(enableNetworkStatsLogging)
				stats.messageReceived(getType(), getNetLength());
		}
	}
	
	for(ClientPool::iterator ii=clients.begin(); ii!=clients.end(); ii++) {
		NetworkNode *n = ii->second;
		if(n != skipnode) n->addMessage(this);
		
		if(enableNetworkStatsLogging)
			stats.messageSent(getType(), getNetLength());
	}
	if(networkServer) {
		for(RemoteClientPool::iterator ii=remoteClients.begin(); ii!=remoteClients.end(); ii++)
			sendToAddress(this, *ii);
	}
}

void SendMessage::sendToMyself(void)
{
	myself()->addMessage(this);
}


bool nlInitialized = false;

#ifndef DISABLE_NETWORKING
static void networkCleanup(void)
{
	killNetworkThread();
	if(networkServer) {
		blockUnregisterServer();
	}
	if(nlInitialized) {
		nlShutdown();
		nlInitialized = false;
	}
}
#endif



bool initNl(void)
{
	if(nlInitialized)
		return true;
	
	int ret = nlInit();
	nlInitialized = true;
	if(!ret) {
		errorDialog((char*)retprintf("Failed to initialize HawkNL: %s.", getNlError()).c_str());
		return false;
	}
	nlInitialized = true;
	ret = nlSelectNetwork(NL_IP);
	if(!ret) {
		const char *errorStr = nlGetErrorStr(nlGetError());
		errorDialog((char*)retprintf("Failed to initialize HawkNL: %s.", getNlError()).c_str());
		return false;
	}
	return true;
}

bool startNetworkServer(void)
{
	initNl();
	
	if(!server)
		server = new Server();
	initNetTarget(server);
	serverSock = nlOpen(serverPort, NL_TCP);
	
	if(serverSock == NL_INVALID)
	{
		errorDialog((char*)retprintf("Failed to open port: %s.", getNlError()).c_str());
		return false;
	}
	if(!nlListen(serverSock))
	{
		errorDialog((char*)retprintf("Failed to bind port: %s.", getNlError()).c_str());
		return false;
	}
	spawnNetworkThread();
	registerServer();
	
	networkServer = true;
	networkClient = false;

	currentNode = clients[0] = new Client(0);
	clients[0]->connect();
	assignViewports();
	clientScripting.exec("config.cfg");
	
	currentNode = server;
	serverScripting.exec("server.cfg");
	serverScripting.exec("multi.cfg");
	serverScripting.command( retprintf("map %s", server->cycle.getNext().c_str()).c_str() );
	server->startGame();
	
	return true;
}

SERVER_CONSOLE_COMMAND(become_net_server)
{
	if(argc > 0) serverPort = atoi(argv[0]);
	startNetworkServer();
}

Client *newClient = NULL;

bool startNetworkClient(const char *host)
{
	if(networkClient) // Already connected to a different server, so need to disconnect.
		nlClose(serverSock);
	killNetworkThread();
	initNl();
	
	if(server)
		delete server;
	
	networkServer = false;
	networkClient = true;
	networkAddress = host;
	
	newClient = new Client(0);
	initNetTarget(newClient);
	
	connectToServer();
	
	GameNode *prevNode = currentNode;
	currentNode = newClient;
	newClient->connect();
	currentNode = prevNode;
	
	spawnNetworkThread();
	return true;
}

CLIENT_CONSOLE_COMMAND(connect)
{
	if(argc != 1) {
		console.printf("Usage: connect address");
		return;
	}
	startNetworkClient(argv[0]);
}

void startSinglePlayer(void)
{
	if(server)
		delete server;
	if(nlInitialized) {
		killNetworkThread();
		nlShutdown();
		nlInitialized = false;
	}
	// FIXME: Create clients properly (ie don't leak)
	
	currentNode = server = new Server();
	
	networkServer = networkClient = false;
	currentNode = clients[0] = new Client(0);
	clients[0]->connect();
	assignViewports();
	clientScripting.exec("config.cfg");
	
	currentNode = server;
	serverScripting.exec("server.cfg");
	server->startGame();
	
	needSinglePlayerInit = true;
}


void initNetwork(void)
{
	initNl();
	atexit(networkCleanup);
}

#ifndef DISABLE_NETWORKING
void connectToServer(void)
{
	NLaddress serverAddress;
	nlStringToAddr(networkAddress, &serverAddress);
	if(nlGetPortFromAddr(&serverAddress) == 0)
		nlSetAddrPort(&serverAddress, defaultServerPort);
	
	serverSock = nlOpen(0, NL_TCP);
	if(serverSock == NL_INVALID)
	{
		int err = nlGetError();
		errorDialog("Could not connect to server.");
		exit(1);
	}
	
	nlEnable(NL_BLOCKING_IO);
	int ret = nlConnect(serverSock, &serverAddress);
	nlDisable(NL_BLOCKING_IO);
	if(!ret) {
		errorDialog("Could not connect to server.");
		exit(1);
	}
	
	addNetAddress(-1, serverSock);
}

void acceptConnections(void)
{
	NLsocket newsock = nlAcceptConnection(serverSock);
	
	if(newsock != NL_INVALID)
	{
		int max_cl_id = 0;
		
		if(remoteClients.size() > 0) {
			RemoteClientPool::iterator ii = remoteClients.end(); ii--;
			max_cl_id = *ii;
		}
		int cl_id = max_cl_id + 1;
		remoteClients.insert(cl_id);
		addNetAddress(cl_id, newsock);
		printfToClient(-1, "A new player has joined the game.");
	}
	else
	{
		int err = nlGetError();
		if(err != NL_NO_PENDING)
		{
			errorDialog((char*)retprintf("nlAcceptConnection failed: %s.", getNlError()).c_str());
			exit(1);
		}
	}
}
#else
void acceptConnections(void) {}
#endif
