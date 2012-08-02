#ifndef PARTICLEPOOL_HPP
#define PARTICLEPOOL_HPP

#include <list>


enum explosion_type 
{
	explode_none = -1, 
	explode_ship = 0, 
	explode_planet,
	explode_fireball,
	explode_missile,
	explode_mine,
	explode_asteroid,
};

class Particle;
class ParticlePool;
struct ClientViewport;

class Particle
{
public:
	typedef unsigned char uchar;

	Particle(
		uchar r, //put this typedef into the header?
		uchar g, 
		uchar b, 
		uchar a,
		float drawScale, 
		const Position &pos, 
		float fadeSpeed,
		const char *sprite=NULL,
		bool under=false);

	~Particle();

	Position	position;
	float		drawScale;
	float		fadeSpeed;
	uchar		red;
	uchar		green;
	uchar		blue;
	float		alpha;
	Image		*sprite;
	bool		isAlive;
	bool        under;
};

class ParticlePool
{
public:
	ParticlePool();
	~ParticlePool();

	void draw(const Position *camera, const ClientViewport *viewport, bool under);
	void timepass();
	void add(Particle p);
	int  size();
	void clear();
	void explodeAt(const Position &source, float sourceRadius, explosion_type explodeType);
	void shield(Position* source, short type, float timer, float radius);
	void engineExhaust(int id, float radius, short selectEngines);
	void spark(Position &pos);
	void trail(Position &pos, float radius);
	void missileTrail(const Position &pos, float radius);
	void moneyTrail(Position &pos, float radius);
    void lightningTrail(const Position &source, const Position &dest);

private:
	void explodeShipAt(const Position &source, float sourceRadius);
	void explodeFireballAt(const Position &source, float sourceRadius);
	void explodeMissileAt(const Position &source, float sourceRadius);
	void explodeMineAt(const Position &source, float sourceRadius);
	void explodeAsteroidAt(const Position &source, float sourceRadius);

	typedef std::list<Particle> ParticleList;
	ParticleList particles;
};

#endif	//PARTICLEPOOL_HPP
