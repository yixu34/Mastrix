#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <vector>

#ifndef DISABLE_NETWORKING
#include "nl.h"
#include "hawkthreads.h"

class Mutex
{
public:
	inline Mutex()           { htMutexInit(&mutex);    }
	inline ~Mutex()          { htMutexDestroy(&mutex); }
	inline void lock(void)   { htMutexLock(&mutex);    }
	inline void unlock(void) { htMutexUnlock(&mutex);  }
	
private:
	HTmutex mutex;
};
#else
class Mutex
{ public:
	inline Mutex()           { }
	inline ~Mutex()          { }
	inline void lock(void)   { }
	inline void unlock(void) { }
};

#endif

void addNetAddress(int address, NLsocket socket);
void sendToAddress(SendMessage *msg, int address);
void spawnNetworkThread(void);
void killNetworkThread(void);

/// Statistics on some subset of messages. Tracks:
///  - All-time totals: number of messages, bytes of messages
///  - One-frame totals: number of messages, bytes of messages
struct MessageStats
{
	MessageStats();
	
	inline void messageSent(int size);
	inline void messageReceived(int size);
	inline void nextFrame(void);
	std::string toString(void);
	
	// All-time
	unsigned messagesIn, messagesOut;
	unsigned bytesIn, bytesOut;
	
	// Last frame
	unsigned frameMessagesIn, frameMessagesOut;
	unsigned frameBytesIn, frameBytesOut;
	
	// This frame
	unsigned curMessagesIn, curMessagesOut;
	unsigned curBytesIn, curBytesOut;
};

class NetworkStats
{
public:
	void messageSent(int type, int size);
	void messageReceived(int type, int size);
	void nextFrame(void);
	MessageStats getStats(void);
	MessageStats getStatsFor(int message);
	
private:
	void haveMessageType(int type);
	
	std::vector<MessageStats> messageTypes;
	MessageStats total;
	
	Mutex mutex;
};
extern NetworkStats stats;
extern bool enableNetworkStatsLogging;
extern class Client *newClient;

void startSinglePlayer(void);
bool startNetworkClient(const char *host);
bool startNetworkServer(void);

void registerServer(void);
void unregisterServer(void);
void blockUnregisterServer(void);
void requestServerList(void);
bool serverListFinished(void);
std::string getServerList(void);


#endif
