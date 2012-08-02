#include "mastrix.hpp"
#include <map>

typedef std::multimap<int, EventHandler*> HandlerSet;
typedef std::pair<int, EventHandler*> handlerKvPair;
static HandlerSet handlers;

bool fadeInProgress = false;

bool evHandle(SDL_Event *ev)
{
	std::vector<EventHandler*> q;
	for(HandlerSet::iterator ii=handlers.begin(); ii!=handlers.end(); ii++) {
		q.push_back(ii->second);
	}
	for(unsigned ii=0; ii<q.size(); ii++)
		if(q[ii]->handleEvent(ev))
			return true;
	return false;
}

void evRedraw(void)
{
	std::vector<EventHandler*> q;
	for(HandlerSet::iterator ii=handlers.begin(); ii!=handlers.end(); ii++) {
		q.push_back(ii->second);
		if(ii->second->isOpaque())
			break;
	}
	for(int ii=q.size()-1; ii>=0; ii--)
	{
		q[ii]->redraw();
	}
}

bool evTimepass(void)
{
	std::vector<EventHandler*> q;
	for(HandlerSet::iterator ii=handlers.begin(); ii!=handlers.end(); ii++) {
		q.push_back(ii->second);
	}
	for(unsigned ii=0; ii<q.size(); ii++)
		if(q[ii]->timepass())
			return true;
	return false;
}


EventHandler::EventHandler()
{
	enabled = false;
}

EventHandler::~EventHandler()
{
	remove();
}

void EventHandler::enable(void)
{
	if(enabled) return;
	enabled = true;
	handlers.insert(handlerKvPair(getPriority(), this));
	onEnable();
}
void EventHandler::remove(void)
{
	enabled = false;
	if(handlers.size() == 0) return;
	
	for(HandlerSet::iterator ii=handlers.begin(); ii!=handlers.end(); ii++) {
		if(ii->second == this) {
			handlers.erase(ii);
			break;
		}
	}
	onDisable();
}


const float fadeSpeed = 2.0;

FadeHandler::FadeHandler(EventHandler *from, EventHandler *to)
{
	fadeInProgress = true;
	this->from = from;
	this->to = to;
	opacity = 1.0;
	switched = false;
	from->remove();
	enable();
}

bool FadeHandler::handleEvent(SDL_Event *ev)
{
	if(switched)
		return to->handleEvent(ev);
	else
		return from->handleEvent(ev);
}

void FadeHandler::redraw(void)
{
	if(switched)
		to->redraw();
	else
		from->redraw();
	
	glColor4f(0, 0, 0, 1.0-opacity);
	glBegin(GL_QUADS);
		glVertex2f(0,                         0);
		glVertex2f(graphics.getScreenWidth(), 0);
		glVertex2f(graphics.getScreenWidth(), graphics.getScreenHeight());
		glVertex2f(0,                         graphics.getScreenHeight());
	glEnd();
}

bool FadeHandler::timepass(void)
{
	if(switched)
	{
		to->timepass();
		opacity += getDt() * fadeSpeed;
		if(opacity >= 1) {
			remove();
			to->enable();
			fadeInProgress = false;
			delete this;
		}
	}
	else
	{
		from->timepass();
		opacity -= getDt() * fadeSpeed;
		if(opacity <= 0) {
			opacity = 0;
			switched = true;
			remove(); enable();
		}
	}
	return false;
}

int FadeHandler::getPriority(void)
{
	if(switched)
		return to->getPriority();
	else
		return from->getPriority();
}
