#include "mastrix.hpp"

struct HttpConnection;
void *httpGet(void *a);

struct HttpConnection
{
	HttpConnection(const char *serverName, const char *path, bool blocking=false) {
		this->serverName = serverName;
		this->path = path;
		
		alloc = 64;
		data = (char*)calloc(alloc, 1);
		len = 0;
		
		finished = success = false;
		error = "";
		
		if(blocking)
			httpGet(this);
		else
			htThreadCreate(httpGet, this, 0);
	}
	~HttpConnection() {
		free(data);
	}
	std::string getResult(void)
	{
		std::string ret;
		mutex.lock();
			ret = data;
		mutex.unlock();
		return ret;
	}
	
	std::string serverName;
	std::string path;
	
	Mutex mutex;
	char *data;
	unsigned len, alloc;
	
	bool finished, success;
	std::string error;
};


/*
 * This code based on the 'getfile' sample program for the HawkNL
 * network library, which is
 * Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
 */
void *httpGet(void *a)
{
	HttpConnection *opt = (HttpConnection*)a;
	NLsocket    sock;
	NLaddress   addr;
	NLbyte      buffer[4096];
	NLint       count, total = 0;
	NLint       crfound = 0;
	NLint       lffound = 0;

	nlGetAddrFromName(opt->serverName.c_str(), &addr);

	/* use the standard HTTP port */
	nlSetAddrPort(&addr, 80);

	/* open the socket and connect to the server */
	sock = nlOpen(0, NL_TCP);
	if(sock == NL_INVALID)
	{
		opt->success = false;
		opt->finished = true;
		opt->error = getNlError();
		return 0;
	}
	if(nlConnect(sock, &addr) == NL_FALSE)
	{
		opt->success = false;
		opt->finished = true;
		opt->error = getNlError();
		return 0;
	}

	/* now let's ask for the file */
	sprintf(buffer, "GET %s HTTP/1.0\r\nHost:%s\r\nAccept: */*\r\nUser-Agent: HawkNL sample program Getfile\r\n\r\n"
	              , opt->path.c_str(), opt->serverName.c_str());
	while(1)
	{
		int ret = nlWrite(sock, (NLvoid *)buffer, (NLint)strlen(buffer));
		if(ret != NL_INVALID)
			break;
		if(nlGetError() == NL_CON_PENDING)
			htThreadSleep(30);
		else
		{
			opt->success = false;
			opt->finished = true;
			opt->error = getNlError();
			return 0;
		}
	}

	/* receive the file and write it locally */
	while(1)
	{
		count = nlRead(sock, (NLvoid *)buffer, (NLint)sizeof(buffer) - 1);
		if(count < 0)
		{
			NLint err = nlGetError();

			/* is the connection closed? */
			if(err == NL_MESSAGE_END)
				break;
			else
			{
				opt->success = false;
				opt->finished = true;
				opt->error = getNlError();
				return 0;
			}
		}
		total += count;
		if(count > 0)
		{
			/* parse out the HTTP header */
			if(lffound < 2)
			{
				int i;
	            
				for(i=0;i<count;i++)
				{
					if(buffer[i] == 0x0D)
						crfound++;
					else
					{
						if(buffer[i] == 0x0A)
							lffound++;
						else
							crfound = lffound = 0; /* reset the CR and LF counters back to 0 */
					}
					if(lffound == 2)
					{
						/* i points to the second LF */
						/* NUL terminate the header string and print it out */
						buffer[i] = buffer[i-1] = 0x0;

						/* write out the rest to the file */
						int writesize = count-i-1;
						char *writestart = &buffer[i+1];
						
						opt->mutex.lock();
							while(opt->len+writesize >= opt->alloc) {
								opt->alloc *= 2;
								opt->data = (char*)realloc(opt->data, opt->alloc);
								memset(opt->data+opt->len, 0, opt->alloc-opt->len);
							}
							memcpy(opt->data+opt->len, writestart, writesize);
							opt->len += writesize;
						opt->mutex.unlock();

						break;
					}
				}
				if(lffound < 2)
					buffer[count + 1] = 0x0; /* we reached the end of buffer */
			} else {
				opt->mutex.lock();
					while(opt->len+count >= opt->alloc) {
						opt->alloc *= 2;
						opt->data = (char*)realloc(opt->data, opt->alloc);
						memset(opt->data+opt->len, 0, opt->alloc-opt->len);
					}
					memcpy(opt->data+opt->len, buffer, count);
					opt->len += count;
				opt->mutex.unlock();
			}
		}
		htThreadSleep(15);
	}
	opt->success = true;
	opt->finished = true;
	return 0;
}



static HttpConnection *registrationConnection;
void registerServer(void)
{
	// Small memory leak, doesn't really matter, easier than syncing the threads to delete
	registrationConnection = new HttpConnection("mastrix.jimrandomh.org", "/addserver.php");
}

static HttpConnection *unregistrationConnection;
void unregisterServer(void)
{
	unregistrationConnection = new HttpConnection("mastrix.jimrandomh.org", "/removeserver.php");
}

void blockUnregisterServer(void)
{
	unregistrationConnection = new HttpConnection("mastrix.jimrandomh.org", "/removeserver.php", true);
}

static HttpConnection *serversConnection;
void requestServerList(void)
{
	serversConnection = new HttpConnection("mastrix.jimrandomh.org", "/getservers.php");
}

bool serverListFinished(void)
{
	return serversConnection->finished;
}

std::string getServerList(void)
{
	if(serversConnection)
		return serversConnection->getResult();
	else
		return "";
}

