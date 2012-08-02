#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <map>

class RecvMessage;
class SendMessage;
class NetworkNode;
class MessageGroup;

#include "network.hpp"

class NetworkNode
{
public:
	NetworkNode();
	virtual ~NetworkNode();
	MessageGroup *getMessages(void);
	int numPending(void);
	void addMessage(SendMessage *msg);
	void addMessage(RecvMessage *msg);
	
private:
	RecvMessage **data;
	int messagesPending, messagesAllocated;
	Mutex mutex;
	
	void lock(void)   { mutex.lock(); }
	void unlock(void) { mutex.unlock(); }
};


class RecvMessage
{
public:
	RecvMessage(int type, int length, int source, unsigned char *data);
	RecvMessage(const SendMessage& send);
	~RecvMessage();
	
	// Query attributes
	inline int getType()      const { return type; }
	inline int getLength()    const { return length; }
	inline int getSource()    const { return source; }
	
	// Pull data from an incoming message
	char        getChar  (void);
	short       getShort (void);
	int         getInt   (void);
	std::string getString(void);
	float       getFloat (void);
	
	inline RecvMessage& operator>> (char &c)        {c=getChar();  return*this;}
	inline RecvMessage& operator>> (short &s)       {s=getShort(); return*this;}
	inline RecvMessage& operator>> (int &i)         {i=getInt();   return*this;}
	inline RecvMessage& operator>> (std::string &s) {s=getString();return*this;}
	inline RecvMessage& operator>> (float &f)       {f=getFloat(); return*this;}
	
private:
	unsigned char *data;
	int pos;
	int type, length;
	int source;
};

class SendMessage
{
public:
	SendMessage(int type);
	~SendMessage();
	
	// Query attributes
	inline int getType(void) const                  { return type;   }
	inline int getLength(void) const                { return length; }
	inline int getSource(void) const                { return source; }
	inline const unsigned char *getData(void) const { return data;   }
	inline int getNetLength() const                 { return length+4; }
	
	// Add data to an outgoing message
	inline SendMessage& operator<< (char c)                { putChar(c); return *this; }
	inline SendMessage& operator<< (short s)               { putShort(s); return *this; }
	inline SendMessage& operator<< (int i)                 { putInt(i); return *this; }
	inline SendMessage& operator<< (const char* s)         { putString(s); return *this; }
	inline SendMessage& operator<< (const std::string& s)  { putString(s.c_str()); return *this; }
	inline SendMessage& operator<< (float f)               { putFloat(f); return *this; }
	void putChar(char);
	void putShort(short);
	void putInt(int);
	void putFloat(float);
	void putString(const char*);
	
	// Finalize (send) an outgoing message
	// - A client cannot send a message directly to other clients - it must go
	//   through the server first.
	// - "Everyone" does not include the sender
	void sendToClient(int client);
	void sendToServer(void);
	void sendToEveryone(void);
	void sendToMyself(void);
	
private:
	void expand(int amt);
	
	unsigned char *data;
	int length, alloc;
	int type;
	int source;
};


class MessageGroup
{
public:
	MessageGroup(RecvMessage **messages, int numMessages);
	~MessageGroup();
	
	inline int numPending(void) const { return numMessages-iteratorPos; }
	RecvMessage *next(void);
	
private:
	RecvMessage **messages;
	int numMessages;
	int iteratorPos;
};

#endif
