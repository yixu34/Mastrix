#include "mastrix.hpp"

void drawWrappedString(const char *string, int x, int y, int width, void *font=GLUT_BITMAP_9_BY_15);

class BriefingHandler :public EventHandler
{
public:
	BriefingHandler(std::string text);
	bool handleEvent(SDL_Event *ev);
	void redraw(void);
	bool timepass(void);
	int getPriority(void)           {return 1;}
	bool isOpaque(void)             {return true;}
	
protected:
	std::string briefText;
};


BriefingHandler::BriefingHandler(std::string text)
{
	briefText = text;
}

bool BriefingHandler::handleEvent(SDL_Event *ev)
{
	switch(ev->type)
	{
		case SDL_KEYDOWN:
		case SDL_MOUSEBUTTONDOWN:
			if(fadeInProgress) return false;
			remove();
			return true;
			
		default:
			return false;
	}
	return false;
}

const float briefingMargin = 60;
const float briefingVMargin = 100;
const float innerMargin = 10;

void BriefingHandler::redraw(void)
{
	glColor3f(0, 0.8, 0);
	glLineWidth(1);
	glBegin(GL_LINE_LOOP);
		glVertex2f(briefingMargin,                            briefingVMargin);
		glVertex2f(graphics.getScreenWidth()-briefingMargin,  briefingVMargin);
		glVertex2f(graphics.getScreenWidth()-briefingMargin,  graphics.getScreenHeight()-briefingVMargin);
		glVertex2f(briefingMargin,                            graphics.getScreenHeight()-briefingVMargin);
	glEnd();
	glColor4f(0, 0.8, 0, 0.3);
	glBegin(GL_QUADS);
		glVertex2f(briefingMargin,                            briefingVMargin);
		glVertex2f(graphics.getScreenWidth()-briefingMargin,  briefingVMargin);
		glVertex2f(graphics.getScreenWidth()-briefingMargin,  graphics.getScreenHeight()-briefingVMargin);
		glVertex2f(briefingMargin,                            graphics.getScreenHeight()-briefingVMargin);
	glEnd();
	
	drawWrappedString(briefText.c_str(),
		briefingMargin+innerMargin,
		briefingVMargin+innerMargin+15,
		graphics.getScreenWidth()-(briefingMargin+innerMargin)*2);
}

bool BriefingHandler::timepass(void)
{
	return true;
}


CLIENT_CONSOLE_COMMAND(briefing)
{
	std::string brieftext("");
	if(argc < 1) {
		console.printf("Usage: briefing [text]");
		return;
	}
	for(int ii=0; ii<argc; ii++)
		brieftext += argv[ii];
	BriefingHandler *h = new BriefingHandler(brieftext);
	h->enable();
}




void drawWrappedString(const char *string, int x, int y, int width, void *font)
{
	int ii;
	int curX=0;
	int tempX;
	int curRow=0;
//	int font_height = FontHeight();
	int font_height = 15;

	tempX = curX;

	ii=0;

	while( string[ii] != '\0' )
	{
		tempX += graphics.drawCharWidth(string[ii], font);

		// If past the end of the line
		if(tempX >= width || string[ii] == '\n') {
			if(string[ii] != '\n')
			{
				// Back up until (a) it fits and (b) it isn't in the middle of
				// a word
				while(ii >= 0 && string[ii] != ' ' && tempX>0)
					tempX -= graphics.drawCharWidth(string[ii--], font);
				if(tempX<=0) // Overshot; this line needs to be broken on a
				{            // non-word-break
					while(tempX<=width)
						tempX += graphics.drawCharWidth(string[++ii], font);
					tempX -= graphics.drawCharWidth(string[--ii], font);
				}
			}

			// Print it out
			graphics.drawTextN(string, ii, x, y+curRow*font_height, font);

			string += ii;
			string++;

			ii=0;

			curRow++;
			curX = tempX = 0;
		} else {
			ii++;
		}
	}

	// Print the remainder
	graphics.drawText( string, x+curX, y+curRow*font_height, font);
}