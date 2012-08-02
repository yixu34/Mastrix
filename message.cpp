#include "mastrix.hpp"
//#include "message.hpp"

RecvMessage::RecvMessage(int type, int length, int source, unsigned char *data)
{
	this->type = type;
	this->length = length;
	this->data = data;
	this->source = source;
	this->pos = 0;
}

RecvMessage::RecvMessage(const SendMessage& send)
{
	type = send.getType();
	length = send.getLength();
	pos = 0;
	source = send.getSource();
	
	data = (unsigned char*)malloc(length);
	memcpy(data, send.getData(), length);
}

RecvMessage::~RecvMessage()
{
	free(data);
}


SendMessage::SendMessage(int type)
{
	this->type = type;
	length = 0;
	alloc = 32;
	data = (unsigned char*)malloc(alloc);
	source = currentNode->getNodeId();
}

SendMessage::~SendMessage()
{
	free(data);
}


char RecvMessage::getChar(void) {
	return data[pos++];
}
short RecvMessage::getShort(void) {
	short ret = (unsigned)data[pos] |
	            ((unsigned)data[pos+1])<<8;
	pos += 2; return ret;
}
int RecvMessage::getInt(void) {
	int ret = ((unsigned)data[pos  ])     |
	          ((unsigned)data[pos+1])<<8  |
	          ((unsigned)data[pos+2])<<16 |
	          ((unsigned)data[pos+3])<<24;
	pos += 4; return ret;
}
std::string RecvMessage::getString(void) {
	std::string ret("");
	while(data[pos])
		ret += data[pos++];
	pos++;
	return ret;
}
float RecvMessage::getFloat(void) {
	float ret;
	memcpy(&ret, data+pos, sizeof(float)); // FIXME: Not portable
	pos += sizeof(float);
	return ret;
}

void SendMessage::putChar(char c) {
	expand(1);
	data[length++] = c;
}
void SendMessage::putShort(short s) {
	expand(2);
	data[length++] = (s    & 0xFF);
	data[length++] = (s>>8 & 0xFF);
}
void SendMessage::putInt(int i) {
	expand(4);
	data[length++] = (i     & 0xFF);
	data[length++] = (i>>8  & 0xFF);
	data[length++] = (i>>16 & 0xFF);
	data[length++] = (i>>24 & 0xFF);
}
void SendMessage::putFloat(float f) {
	expand(sizeof(float));
	memcpy(data+length, &f, sizeof(float)); // FIXME: Not portable
	length += sizeof(float);
}
void SendMessage::putString(const char*s) {
	expand((int)strlen(s)+1);
	while(*s != '\0')
		data[length++] = *(s++);
	data[length++] = '\0';
}
void SendMessage::expand(int amt) {
	while(length+amt >= alloc) {
		alloc *= 2;
		data = (unsigned char*)realloc(data, alloc);
	}
}

