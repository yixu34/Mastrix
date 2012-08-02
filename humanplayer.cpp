#include "mastrix.hpp"

ServerConsoleVar<float> spawnShieldDelay("shield_time", 1.5);
ServerConsoleVar<float> powerupShieldDelay("powerup_shield_time", 10);
ServerConsoleVar<float> flagSlowdown("flag_slowdown", 0.6);

HumanPlayer::HumanPlayer(int clientID, std::string name)
	: Player(getPlayerShipType())
{
	entType = ent_player_ship;
	this->clientID = clientID;
	server->registerAvatar(clientID, entID);
	
	shieldTime = spawnShieldDelay;
	shielding = true;
	
	SendMessage avatarNotice(message_ship_entid);
		avatarNotice.putInt(entID);
	avatarNotice.sendToClient(clientID);

	SendMessage msg(message_updatehealth);
		msg.putFloat(health / maxHealth);
	msg.sendToClient(clientID);
	
	this->name = name;
	
	setTeam(server->clientInfo[clientID].team);
	
	switch(server->clientInfo[clientID].color)
	{
		case 0: sprite = "images/ship.png"; break;
		case 1: sprite = "images/ship-blue.png"; break;
		case 2: sprite = "images/ship-red.png"; break;
		case 3: sprite = "images/ship-green.png"; break;
		case 4: sprite = "images/ship-yellow.png"; break;
		case 5: sprite = "images/ship-purple.png"; break;
	}
	
	if(team == team_red)
		sprite = "images/ship-red.png";
	else if(team == team_blue)
		sprite = "images/ship-blue.png";
	
	weapons.updateWeapons();
	flag = false;
	sendAdd();
	sendAvatar();
	activateShield(shieldTime, effect_spawnshield);
}

void HumanPlayer::acquireFlag()
{
	flag = true;
	SendMessage msg(message_update_effect);
		msg.putInt(entID);
		msg.putShort(effect_money);
	msg.sendToEveryone();
}

void HumanPlayer::loseFlag()
{
	flag = false;
	SendMessage msg(message_update_effect);
		msg.putInt(entID);
		msg.putShort(effect_none);
	msg.sendToEveryone();
}

float HumanPlayer::getMaxSpeed(void) const
{
	if(hasFlag())
		return Player::getMaxSpeed() * flagSlowdown;
	else
		return Player::getMaxSpeed();
}

void HumanPlayer::timepass()
{
	Player::timepass();

	if (shielding) {
		shieldTime -= getDt();
		if (shieldTime < 0.0) {
			shielding = false;
			shieldTime = 0.0;
		}
	}
	// Check for entering regions
	for(unsigned ii=0; ii<server->regions.size(); ii++)
	{
		Server::Region *r = &server->regions[ii];
		float myX = position.getX(), myY = position.getY(), myRadius=getRadius();
		if( myX >= r->min_x - myRadius
		 && myX <= r->max_x + myRadius
		 && myY >= r->min_y - myRadius
		 && myY <= r->max_y + myRadius)
		{
			serverScripting.defineVariable(r->name, retprintf("%i",entID), tag_frame_temporary);
			if(hasFlag())
			{
				if(r->name == "red_flagdrop" && team==team_red) {
					server->giveFlagTo(team_red);
					server->teamScores[team_red]++;
					server->teamScores[team_blue]--;
					messageToClient(-1, "Red team captured a pile of gold!");
					loseFlag();
				} else if(r->name == "blue_flagdrop" && team==team_blue) {
					server->giveFlagTo(team_blue);
					server->teamScores[team_red]--;
					server->teamScores[team_blue]++;
					messageToClient(-1, "Blue team captured a pile of gold!");
					loseFlag();
				}
			}
		}
	}

	// HACK: Update camera manually
	if (position.getChanged()) { 
		SendMessage cameraUpdate(message_camera);
			position.writeToMessage(cameraUpdate);
			cameraUpdate.putFloat(leftBorder);
			cameraUpdate.putFloat(rightBorder);
			cameraUpdate.putFloat(topBorder);
			cameraUpdate.putFloat(bottomBorder);
		cameraUpdate.sendToClient(clientID);
	}
	
	// Update engine indicator
	SendMessage engineUpdate(message_update_engines);
	engineUpdate.putFloat(afterburn_thrust/thrustABMax);
	engineUpdate.putFloat(afterburn_left/strafeABMax);
	engineUpdate.putFloat(afterburn_right/strafeABMax);
	engineUpdate.putFloat(afterburn_brake/brakeABMax);
	engineUpdate.putFloat(warmup/maxWarmup);
	engineUpdate.sendToClient(clientID);
	
	Entity::timepass();
}

void HumanPlayer::takeDamage(float amt, int source)
{
	Player::takeDamage(amt, source);
	updateHealth();
}
void HumanPlayer::heal(float amt)
{
	Player::heal(amt);
	updateHealth();
}

void HumanPlayer::shieldsUp(float time)
{
	shieldTime = time;
	shielding = true;
	activateShield(shieldTime, effect_extrashield);
}

void HumanPlayer::updateHealth(void)
{
	SendMessage msg(message_updatehealth);
		msg.putFloat(health/maxHealth);
	msg.sendToClient(clientID);
}

void HumanPlayer::die(int killerID)
{
	if(shouldDelete()) return;
	server->addDeath(clientID, killerID, getClientId());
	serverScripting.defineVariable("player_died", retprintf("%i", getEntId()), tag_frame_temporary);
	serverScripting.clearTaggedVariables(tag_death_temporary);
	team_type enemyTeam;
	if(team==team_red) enemyTeam = team_blue;
	else enemyTeam = team_red;
	if(flag) placeFlagAt(position.getX(), position.getY(), enemyTeam, false);
	Player::die(killerID);
}

void HumanPlayer::dieSilently(void)
{
	this->explodeType = explode_none;
	if(shouldDelete()) return;
	server->silentDeath(clientID);
	serverScripting.clearTaggedVariables(tag_death_temporary);
	Entity::die(-1);
}

void HumanPlayer::deleteMe(void)
{
	server->deleteAvatar(clientID);
	Player::deleteMe();
}

void HumanPlayer::sendAvatar()
{
	SendMessage msg(message_avatar);
		msg.putInt(entID);
	msg.sendToClient(clientID);
}

