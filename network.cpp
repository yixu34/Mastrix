#include "mastrix.hpp"
#include <cassert>
#include <set>

bool enableNetworkStatsLogging = true;

#ifndef DISABLE_NETWORKING
#include <hawkthreads.h>

#define NET_BUFSIZ 8192

struct RemoteConnection
{
	RemoteConnection();
	~RemoteConnection();
	
	NLsocket sock;
	unsigned netID;
	
	unsigned char *recv_buf;
	unsigned recv_len, recv_alloc;
	
	unsigned char *send_buf;
	unsigned send_len, send_alloc, send_start;
	
	bool disconnected;
	bool isPrimary;
};
typedef std::list<RemoteConnection*> ConnectionSet;
ConnectionSet connections;
Mutex connectionsMutex;

void collectMessages(RemoteConnection *conn);
NetworkNode *recvNode;

void readFromConnection(RemoteConnection *conn)
{
	if(conn->disconnected)
		return;
	
	// Ensure plenty of free buffer space
	while(conn->recv_alloc - conn->recv_len < NET_BUFSIZ)
	{
		conn->recv_alloc *= 2;
		conn->recv_buf = (unsigned char *)realloc(conn->recv_buf, conn->recv_alloc);
	}
	int ret = nlRead(conn->sock, conn->recv_buf, conn->recv_alloc - conn->recv_len);
	
	if(ret == NL_INVALID)
	{
		int err = nlGetError();
		const char *errStr = nlGetErrorStr(err);
		switch(err)
		{
			default:
			case NL_NULL_POINTER:
			case NL_INVALID_SOCKET:
			case NL_CON_REFUSED:
			case NL_SYSTEM_ERROR:
			case NL_MESSAGE_END:
			case NL_SOCK_DISCONNECT: {
				conn->disconnected = true;
				if(conn->isPrimary) {
					std::string message = retprintf("Disconnected: %s", getNlError());
					recvNode->addMessage(
						new RecvMessage(message_disconnect, 0, conn->netID, (unsigned char*)strdup(message.c_str()))
						);
					return;
				}
				}
				
			case NL_CON_PENDING:
				// Just try again later
				break;
		}
	}
	else
	{
		if(ret) {
			conn->recv_len += ret;
			collectMessages(conn);
		}
	}
}

void writeToConnection(RemoteConnection *conn)
{
	if(conn->disconnected)
		return;
	if(!conn->send_len)
		return;
	
	// Write as much of the pending data as possible
	int ret = nlWrite(conn->sock, conn->send_buf + conn->send_start, conn->send_len);
	
	if(ret == NL_INVALID)
	{
		int err = nlGetError();
		switch(err)
		{
			default: {
				conn->disconnected = true;
				if(conn->isPrimary) {
					std::string message = retprintf("Disconnected: %s", getNlError());
					recvNode->addMessage(
						new RecvMessage(message_disconnect, 0, conn->netID, (unsigned char*)strdup(message.c_str()))
						);
				}
				return;
				}
				
			case NL_CON_REFUSED:
				recvNode->addMessage(
					new RecvMessage(message_disconnect, 0, conn->netID, (unsigned char*)strdup("Server connection refused."))
					);
				conn->disconnected = true;
				break;
				
			case NL_CON_PENDING:
				// Just try again later
				break;
		}
	}
	else
	{
		conn->send_start += ret;
		conn->send_len -= ret;
		
		// If there's more than bufsiz free space at the beginning of the buffer,
		// move the remaining pending data into the buffer.
		if(conn->send_start > NET_BUFSIZ)
			memmove(conn->send_buf + conn->send_start, conn->send_buf, conn->send_len);
	}
}

void addSendData(RemoteConnection *conn, const char *data, unsigned len)
{
	connectionsMutex.lock();
		if(!conn->disconnected)
		{
			while(conn->send_alloc < conn->send_len + conn->send_start + len)
			{
				conn->send_alloc *= 2;
				conn->send_buf = (unsigned char *)realloc(conn->send_buf, conn->send_alloc);
			}
			memcpy(conn->send_buf + conn->send_start + conn->send_len, data, len);
			conn->send_len += len;
		}
	connectionsMutex.unlock();
}

void addSendMessage(RemoteConnection *conn, SendMessage *msg)
{
	unsigned len = msg->getLength() + 4;
	unsigned type = msg->getType();
	char *msg_data = (char*)malloc(len);
	
	msg_data[0] =  len     &0xFF;
	msg_data[1] = (len>>8) &0xFF;
	msg_data[2] =  type    & 0xFF;
	msg_data[3] = (type>>8)& 0xFF;
	memcpy(msg_data+4, msg->getData(), msg->getLength());
	
	addSendData(conn, msg_data, len);
	free(msg_data);
	
	if(enableNetworkStatsLogging)
		stats.messageSent(msg->getType(), len);
}

void sendToAddress(SendMessage *msg, int address)
{
	for(ConnectionSet::iterator ii = connections.begin(); ii != connections.end(); ii++)
	{
		if((*ii)->netID == address)
		{
			addSendMessage(*ii, msg);
			return;
		}
	}
}

void collectMessages(RemoteConnection *conn)
{
	unsigned pos = 0;
	unsigned char *buf = conn->recv_buf;
	
	// Extract, frame, and dispatch messages
	while(1)
	{
		if(pos+2 > conn->recv_len)
			break;
		
		unsigned len = buf[pos] + (buf[pos+1]<<8);
		assert(len >= 4);
		
		if(len+pos > conn->recv_len)
			break;
		
		unsigned type = buf[pos+2] + (buf[pos+3]<<8);
		
		unsigned char *data_copy = (unsigned char*)malloc(len-4);
		memcpy(data_copy, buf+pos+4, len-4);
		RecvMessage *m = new RecvMessage(type, len-4, conn->netID, data_copy);
		recvNode->addMessage(m);
		if(enableNetworkStatsLogging)
			stats.messageReceived(type, len);
		
		pos += len;
	}
	
	// Shift leftover data to the bottom of the buffer
	memmove(buf, buf+pos, conn->recv_len-pos);
	conn->recv_len -= pos;
}

void initNetTarget(NetworkNode *mainNode)
{
	recvNode = mainNode;
}

bool networkThreadRunning = false;
HThreadID threadId;
bool killNetwork = false;

void *networkLoop(void *data)
{
	do
	{
		connectionsMutex.lock();
		for(ConnectionSet::iterator ii = connections.begin(); ii != connections.end(); ii++)
		{
			if(!(*ii)->disconnected) writeToConnection(*ii);
			if(!(*ii)->disconnected) readFromConnection(*ii);
			
			if( (*ii)->disconnected )
			{
				recvNode->addMessage(
					new RecvMessage(message_disconnect, 0, (*ii)->netID, (unsigned char*)strdup("Connection closed."))
					);
				nlClose( (*ii)->sock );
				delete *ii;
				ii = connections.erase(ii);
				ii--;
			}
		}
		connectionsMutex.unlock();
		htThreadSleep(15);
	}
	while(!killNetwork);
	return NULL;
}

void spawnNetworkThread(void)
{
	if(networkThreadRunning) return;
	killNetwork = false;
	threadId = htThreadCreate(networkLoop, NULL, 1);
	networkThreadRunning = true;
}
void killNetworkThread(void)
{
	if(!networkThreadRunning) return;
	killNetwork = true;
	htThreadJoin(threadId, NULL);
	networkThreadRunning = false;
}

void addNetAddress(int address, NLsocket socket)
{
	RemoteConnection *c = new RemoteConnection;
	c->sock = socket;
	c->netID = address;
	if(address==-1) c->isPrimary = true;
	else c->isPrimary = false;
	
	connectionsMutex.lock();
		connections.push_back(c);
	connectionsMutex.unlock();
}

RemoteConnection::RemoteConnection()
{
	recv_len = send_len = send_start = 0;
	recv_alloc = send_alloc = NET_BUFSIZ;
	recv_buf = (unsigned char *)malloc(recv_alloc);
	send_buf = (unsigned char *)malloc(send_alloc);
	disconnected = false;
}

RemoteConnection::~RemoteConnection()
{
	free(recv_buf);
	free(send_buf);
}

#else // DISABLE_NETWORKING
void initNetTarget(NetworkNode *mainNode) {}
void spawnNetworkThread(void) {}
void killNetworkThread(void) {}
void addNetAddress(int address, NLsocket socket) {}
void sendToAddress(SendMessage *msg, int address) {}

#endif
