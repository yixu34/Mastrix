#include "mastrix.hpp"
#include <vector>

NetworkStats stats;


inline void MessageStats::messageSent(int size)
{
	messagesOut++;
	curMessagesOut++;
	bytesOut += size;
	curBytesOut += size;
}
inline void MessageStats::messageReceived(int size)
{
	messagesIn++;
	curMessagesIn++;
	bytesIn += size;
	curBytesIn += size;
}
inline void MessageStats::nextFrame(void)
{
	frameMessagesIn  = curMessagesIn;
	frameMessagesOut = curMessagesOut;
	frameBytesIn     = curBytesIn;
	frameBytesOut    = curBytesOut;
	
	curMessagesIn = curMessagesOut = curBytesIn = curBytesOut = 0;
}

MessageStats::MessageStats()
{
	messagesIn     = 0; messagesOut      = 0;
	bytesIn        = 0; bytesOut         = 0;
	frameMessagesIn= 0; frameMessagesOut = 0;
	frameBytesIn   = 0; frameBytesOut    = 0;
	curMessagesIn  = 0; curMessagesOut   = 0;
	curBytesIn     = 0; curBytesOut      = 0;
}


std::string MessageStats::toString(void)
{
	std::string ret;
	
	ret += "  This frame:\n";
	if(!frameMessagesIn && !frameMessagesOut)
		ret += "    Nothing sent or received.\n";
	else
	{
		if(frameMessagesIn)
			ret += retprintf("    In: %i bytes in %i messages.\n", frameBytesIn, frameMessagesIn);
		if(frameMessagesOut)
			ret += retprintf("    Out: %i bytes in %i messages.\n", frameBytesOut, frameMessagesOut);
	}
	
	ret += "  All time:\n";
	if(!messagesIn && !messagesOut)
		ret += "    Nothing sent or received.\n";
	else
	{
		if(messagesIn)
			ret += retprintf("    In: %i bytes in %i messages.\n", bytesIn, messagesIn);
		if(messagesOut)
			ret += retprintf("    Out: %i bytes in %i messages.\n", bytesOut, messagesOut);
	}
	return ret;
}


void NetworkStats::haveMessageType(int type)
{
	if(messageTypes.size() <= (unsigned)type)
		messageTypes.resize(type+1);
}

void NetworkStats::messageSent(int type, int size)
{
	mutex.lock();
		haveMessageType(type);
		messageTypes[type].messageSent(size);
		total.messageSent(size);
	mutex.unlock();
}

void NetworkStats::messageReceived(int type, int size)
{
	mutex.lock();
		haveMessageType(type);
		messageTypes[type].messageReceived(size);
		total.messageReceived(size);
	mutex.unlock();
}

void NetworkStats::nextFrame(void)
{
	mutex.lock();
		for(unsigned ii=0; ii<messageTypes.size(); ii++)
			messageTypes[ii].nextFrame();
		total.nextFrame();
	mutex.unlock();
}

MessageStats NetworkStats::getStats(void)
{
	MessageStats ret;
	mutex.lock();
		ret = total;
	mutex.unlock();
	return ret;
}

MessageStats NetworkStats::getStatsFor(int message)
{
	MessageStats ret;
	mutex.lock();
		ret = messageTypes[message];
	mutex.unlock();
	return ret;
}

CLIENT_CONSOLE_COMMAND(net_stats)
{
	std::string statString;
	
	switch(argc)
	{
		case 0:
			statString = retprintf("Stats for all message types:\n%s", stats.getStats().toString().c_str());
			break;
		case 1:
		{
			int messageType = atoi(argv[0]);
			statString = retprintf("Stats for message %i:\n%s", messageType,
			                       stats.getStatsFor(messageType).toString().c_str());
			break;
		}
		default:
			statString = "Usage: net_stats [message type].";
	}
	console.printf("%s", statString.c_str());
}

SERVER_CONSOLE_COMMAND(server_stats)
{
	std::string statString;
	
	switch(argc)
	{
		case 0:
			statString = stats.getStats().toString();
			break;
		case 1:
		{
			int messageType = atoi(argv[0]);
			statString = retprintf("Server stats for message %i:\n%s", messageType,
			                       stats.getStatsFor(messageType).toString().c_str());
			break;
		}
		default:
			statString = "Usage: server_stats [message type].";
	}
	console.printf("%s", statString.c_str());
}

