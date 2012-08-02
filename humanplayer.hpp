#ifndef HUMANPLAYER_HPP
#define HUMANPLAYER_HPP

#include "player.hpp"
#include <string>

class HumanPlayer:public Player
{
public:
	HumanPlayer(int clientID, std::string name);

	void timepass();
	void die(int killer);
	void dieSilently(void);
	void deleteMe(void);
	void takeDamage(float amt, int source);
	void heal(float amt);
	void shieldsUp(float time);
	int getDamageSource(void) { return clientID; }
	int getDrawEffect(void) { return hasFlag()?effect_money:effect_none; }
	void sendAvatar();
	
	bool hasFlag() const { return flag; }
	void acquireFlag();
	void loseFlag();
	float getMaxSpeed(void) const;

	bool isHuman(void) { return true; }
	int getClientId(void) { return clientID; }
	void updateHealth(void);
	
protected:
	int clientID;
	bool flag;
};

#endif	//HUMANPLAYER_HPP