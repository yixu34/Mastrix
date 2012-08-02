#if 0
#include <deque>
//#include <SDL.h>

struct SDL_keysym;

struct Keystroke
{
	//Keystroke(int keycode, bool isDown) { this->keycode = keycode; this->isDown = isDown; }
	//long keycode;
	Keystroke(SDL_keysym keycode, bool isDown)
	{
		this->keycode = keycode;
		this->isDown  = isDown;
	}

	SDL_keysym keycode;
	bool	   isDown;
};

struct Mouseclick
{
	Mouseclick(Uint8 button, bool isPressed)
	{
		this->button	= button;
		this->isPressed = isPressed;
	}

	Uint8 button;
	bool  isPressed;
};

//#define Keystroke_Mouse1  -1
//#define Keystroke_Mouse2  -2
//#define Keystroke_Mouse3  -3

//enum button_code
//{
//	button_mouse1 = -1, 
//	button_mouse2 = -2, 
//	button_mouse3 = -3
//};

extern std::deque<Mouseclick> mouseBuffer;

#endif