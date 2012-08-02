#include "mastrix.hpp"

NetworkNode::NetworkNode()
{
	data = NULL;
	messagesPending = messagesAllocated = 0;
}

NetworkNode::~NetworkNode()
{
	for(int ii=0; ii<messagesPending; ii++)
		delete data[ii];
	delete[] data;
}

MessageGroup *NetworkNode::getMessages(void)
{
	RecvMessage **old_data;
	int numMessages;
	
	lock();
		old_data = data;
		numMessages = messagesPending;
		
		data = NULL;
		messagesPending = messagesAllocated = 0;
	unlock();
	
	return new MessageGroup(old_data, numMessages);
}


int NetworkNode::numPending(void)
{
	return messagesPending;
}

void NetworkNode::addMessage(RecvMessage *msg)
{
	lock();
		if(data == NULL) {
			messagesAllocated = 64;
			data = (RecvMessage**)malloc(
				messagesAllocated * sizeof(RecvMessage*));
		} else if(messagesPending >= messagesAllocated) {
			messagesAllocated *= 2;
			data = (RecvMessage**)realloc(
				data, messagesAllocated * sizeof(RecvMessage*));
		}
		data[messagesPending++] = msg;
	unlock();
}

void NetworkNode::addMessage(SendMessage *msg)
{
	addMessage(new RecvMessage(*msg));
}

MessageGroup::MessageGroup(RecvMessage **messages, int numMessages)
{
	this->messages = messages;
	this->numMessages = numMessages;
	this->iteratorPos = 0;
}

MessageGroup::~MessageGroup()
{
	for(int ii=0; ii<numMessages; ii++)
		delete messages[ii];
	delete[] messages;
}

RecvMessage *MessageGroup::next(void)
{
	if(iteratorPos >= numMessages)
		return NULL;
	return messages[iteratorPos++];
}

