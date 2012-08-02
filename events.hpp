#ifndef EVENTS_HPP
#define EVENTS_HPP

bool evHandle(SDL_Event *ev);
void evRedraw(void);
bool evTimepass(void);

class EventHandler
{
public:
	// Handle an event, if this handler can.
	// Returns: true if the event was handled, false if the next event handler
	//          should be tried.
	virtual bool handleEvent(SDL_Event *ev) {return false;}
	virtual void redraw(void)               {}
	virtual bool timepass(void)             {return false;}
	virtual bool isOpaque(void)             {return false;}
	
	virtual void onEnable(void)             {}
	virtual void onDisable(void)            {}
	virtual int getDrawPriority(void)       {return getPriority();}
	virtual int getPriority(void)           {return 0;}
	
	EventHandler();
	virtual ~EventHandler();
	void enable(void);
	void remove(void);
	
private:
	bool enabled;
};


class MainGameHandler :public EventHandler
{
public:
	bool handleEvent(SDL_Event *ev);
	void redraw(void);
	bool timepass(void);
	int getPriority(void)           {return 2;}
	bool isOpaque(void)             {return true;}
	void onEnable(void);
};
extern MainGameHandler mainGameHandler;


class FadeHandler :public EventHandler
{
public:
	FadeHandler(EventHandler *from, EventHandler *to);
	bool handleEvent(SDL_Event *ev);
	void redraw(void);
	bool timepass(void);
	int getPriority(void);
	
private:
	bool switched;
	float opacity;
	EventHandler *from, *to;
};


void showMainMenu(void);
void showGameOver(void);

extern bool fadeInProgress;

#endif
