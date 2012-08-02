#include "mastrix.hpp"

Messagebox::Messagebox()
{
	//numLines = 5;
	lineHeight = 18;
}

void Messagebox::setView(int l, int r, int t, int b)
{
	xleft = l;
	xright = r;
	ytop = t;
	ybottom = b;
}

void Messagebox::draw()
{
	/*
	GameX.DrawRect(ColorX(64,64,64,100), xleft, ytop, xright, ybottom - lineHeight);
	GameX.DrawRect(ColorX(128,128,128,100), xleft, ytop + (lineHeight*(numLines-1)), xright, ybottom);
	for (int i = 0; i < numLines; i++) {
		GameX.DrawText(xleft, ytop + (lineHeight*i),
			messageList[(numLines - 1) - i].c_str(), 255, 0, 0);
	}
	*/
	if (messageList.size() != 0) {
		MessageQueue::iterator ii=messageList.end();
		ii--;
		if ((*ii).lifetime <= 0.0) {
			messageList.pop_back();
		}
	}

	bool colorFlag = true;
	int curr_Y = ybottom - 18;
	for (MessageQueue::iterator ii=messageList.begin(); ii != messageList.end(); ii++) {
		(*ii).lifetime -= getDt();
		//GameX.DrawText(xleft, curr_Y, (char*)((*ii).msg).c_str(), 255, 64, 0);
		graphics.drawText(ii->msg.c_str(), xleft, curr_Y);
		curr_Y -= 18;
		colorFlag = false;
	}
}

void Messagebox::draw(std::string message)
{
	/*
	for (int i = numLines - 1; i > 0; i--) {
		messageList[i] = messageList[i-1];
	}
	messageList[0] = message;
	draw();
	*/
	ScreenMessage scrMsg;
	scrMsg.msg = message;
	scrMsg.lifetime = 10.0;
	messageList.push_front(scrMsg);

}
