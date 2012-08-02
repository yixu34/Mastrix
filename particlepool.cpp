#include "mastrix.hpp"

ClientConsoleVar<float> particleSpeed("particle_speed", 600.0f);
ClientConsoleVar<float> particleFadeSpeed("particle_fade", 150.0f);
ClientConsoleVar<float> maxParticles("max_particles", 24000.0f);

//Particle members
Particle::Particle(
	uchar r, 
	uchar g, 
	uchar b, 
	uchar a, 
	float drawScale, 
	const Position &pos, 
	float fadeSpeed,
	const char *sprite,
	bool under)
{
	red   = r;
	green = g;
	blue  = b;
	alpha = a;

	position		= pos;
	this->fadeSpeed = fadeSpeed;
	this->drawScale = drawScale;
	this->under = under;
	
	if(sprite)
		this->sprite  = images.getImage(sprite);
	else
		this->sprite = NULL;

	isAlive = true;
}

Particle::~Particle()
{
	sprite  = 0;
	isAlive = false;
}



//ParticlePool members

ParticlePool::ParticlePool()
{}

ParticlePool::~ParticlePool()
{
	clear();
}

void ParticlePool::draw(const Position *camera, const ClientViewport *viewport, bool under)
{
	glEnable(GL_POINT_SMOOTH);
	
	//draw a blank particle sprite w/ some blending
	for(ParticleList::iterator ii = particles.begin(); ii != particles.end(); ++ii)
	{
		if(ii->under != under) continue;
		float px = (ii->position.getX() - camera->getX())*zoom + viewport->center_x;
		float py = (ii->position.getY() - camera->getY())*zoom + viewport->center_y;
		if (px >= viewport->left && px <= viewport->right &&
			py >= viewport->top && py <= viewport->bottom) 
		{		
			if(ii->sprite)
			{
				glColor4ub(ii->red, ii->green, ii->blue, ii->alpha);
				graphics.drawColoredImage(ii->sprite, px, py, 0, ii->drawScale*zoom);
			}
			else
			{
				graphics.drawPoint(
					Color(ii->red, ii->green, ii->blue, ii->alpha), 
					px, py,
					ii->drawScale * zoom * 32);
			}
		}
	}
	glDisable(GL_POINT_SMOOTH);
}

//position class takes care of movement
//have to manually update the colors
void ParticlePool::timepass()
{
	for(ParticleList::iterator ii = particles.begin(); ii != particles.end(); ++ii)
	{
		ii->alpha -= ii->fadeSpeed * getDt();

		if (ii->alpha <= 0.0f)
		{
			ii = particles.erase(ii);
			--ii;
		}
	}
}

void ParticlePool::add(Particle p)
{
	if (particles.size() <  maxParticles)
		particles.push_back(p);	
}

int ParticlePool::size() {
	return particles.size();
}

void ParticlePool::clear()
{
	particles.clear();
}

void ParticlePool::explodeAt(
	const Position &source, 
	float sourceRadius, 
	explosion_type explodeType)
{
	switch(explodeType)
	{
		case explode_ship:     explodeShipAt    (source, sourceRadius); break;
		case explode_fireball: explodeFireballAt(source, sourceRadius); break;
		case explode_missile:  explodeMissileAt (source, sourceRadius); break;
		case explode_mine:     explodeMineAt    (source, sourceRadius); break;
		case explode_asteroid: explodeAsteroidAt(source, sourceRadius); break;
		default: break;
	};
}

void ParticlePool::explodeShipAt(const Position &source, float sourceRadius)
{
	int numParticles = randInt(500, 750);
	for (int i = 0; i < numParticles; i++)
	{
		float randAngle = randFloat(0.0f, M_PI * 2);
		float spawnX	= source.getX() + sourceRadius * cos(randAngle);
		float spawnY	= source.getY() + sourceRadius * sin(randAngle);
		float speed		= randFloat(100.0f, 650.0f);
		float spawnVelX = speed * cos(randAngle);
		float spawnVelY = speed * sin(randAngle);

		Position spawnPos = Position(spawnX, spawnY);
		spawnPos.setX_vel(spawnVelX);
		spawnPos.setY_vel(spawnVelY);

		Particle p = Particle(
			255, 
			randInt(63, 216), 
			0, 
			255, 
			randFloat(0.1, 0.6),  
			spawnPos, 
			particleFadeSpeed);

		add(p);
	}
}

void ParticlePool::explodeMissileAt(const Position &source, float sourceRadius)
{
	int numParticles = randInt(100, 150);
	for (int i = 0; i < numParticles; i++)
	{
		float randAngle = randFloat(0.0f, M_PI * 2);
		float randDist  = randFloat(0, 2*sourceRadius);
		float spawnX	= source.getX() + randDist * cos(randAngle);
		float spawnY	= source.getY() + randDist * sin(randAngle);
		float speed		= randFloat(-100.0f, 30.0f);
		float spawnVelX = speed * cos(randAngle);
		float spawnVelY = speed * sin(randAngle);

		Position spawnPos = Position(spawnX, spawnY);
		spawnPos.setX_vel(spawnVelX);
		spawnPos.setY_vel(spawnVelY);

		Particle p = Particle(
			255, randInt(100,200), 0, randInt(170, 255),
			randFloat(0.5, 1.2),  
			spawnPos, 
			600);

		add(p);
	}
}

void ParticlePool::explodeFireballAt(const Position &source, float sourceRadius)
{
	int numParticles = randInt(500, 750);
	for (int i = 0; i < numParticles; i++)
	{
		float randAngle = randFloat(0.0f, M_PI * 2);
		float randDist  = randFloat(0, sourceRadius);
		float spawnX	= source.getX() + randDist * cos(randAngle);
		float spawnY	= source.getY() + randDist * sin(randAngle);
		float speed		= randFloat(-100.0f, 30.0f);
		float spawnVelX = speed * cos(randAngle);
		float spawnVelY = speed * sin(randAngle);

		Position spawnPos = Position(spawnX, spawnY);
		spawnPos.setX_vel(spawnVelX);
		spawnPos.setY_vel(spawnVelY);

		Particle p = Particle(
			255, 255, randInt(150, 255), randInt(170, 255),
			randFloat(0.5, 1.2),  
			spawnPos, 
			300);

		add(p);
	}
}

void ParticlePool::explodeMineAt(const Position &source, float sourceRadius)
{
	int numParticles = randInt(300, 400);
	for (int i = 0; i < numParticles; i++)
	{
		float randAngle = randFloat(0.0f, M_PI * 2);
		float randDist  = randFloat(0, sourceRadius);
		float spawnX	= source.getX() + randDist * cos(randAngle);
		float spawnY	= source.getY() + randDist * sin(randAngle);
		float speed		= randFloat(-100.0f, 30.0f);
		float spawnVelX = speed * cos(randAngle);
		float spawnVelY = speed * sin(randAngle);

		Position spawnPos = Position(spawnX, spawnY);
		spawnPos.setX_vel(spawnVelX);
		spawnPos.setY_vel(spawnVelY);

		Particle p = Particle(
			255, randInt(100,200), 255,  randInt(170, 255),
			randFloat(0.5, 1.2),  
			spawnPos, 
			400);

		add(p);
	}
}

void ParticlePool::explodeAsteroidAt(const Position &source, float sourceRadius)
{
	int numParticles = randInt(50, 100);
	for (int i = 0; i < numParticles; i++)
	{
		float randAngle = randFloat(0.0f, M_PI * 2);
		float spawnX	= source.getX() + sourceRadius * cos(randAngle);
		float spawnY	= source.getY() + sourceRadius * sin(randAngle);
		float speed		= randFloat(10.0f, 200.0f);
		float spawnVelX = speed * cos(randAngle);
		float spawnVelY = speed * sin(randAngle);

		Position spawnPos = Position(spawnX, spawnY);
		spawnPos.setX_vel(spawnVelX);
		spawnPos.setY_vel(spawnVelY);
		int color = randInt(50, 150);
		
		Particle p = Particle(
			color, color, color,
			255, 
			randFloat(0.1, 0.6),  
			spawnPos, 
			particleFadeSpeed);

		add(p);
	}
}

/*
void ParticlePool::shield(Position *source, short type, float timer, float radius) {
	int numParticles = 180;
	for (int i = 0; i < numParticles; i++) {
		float spawnX = source->getX() + radius*randFloat(0.0, M_PI*2);
		float spawnY = source->getY() + radius*randFloat(0.0, M_PI*2);

		Position spawnPos = Position(spawnX, spawnY);
		Particle p = Particle(128,128,255,200,0.2, spawnPos, 200/timer);
		add(p);
	}
}
*/

void ParticlePool::engineExhaust(int id, float radius, short selectEngines) {
	int numParticles;
	const Position *source = currentNode->getEntityPosition(id);
	if (selectEngines & 0x1) 
	{ 
		numParticles = randInt(10,15);
		for (int i = 0; i < numParticles; i++) {
			float randAngle = randFloat((M_PI) - (M_PI/9), (M_PI) + M_PI/9);
			float spawnX	= source->getX() - (radius + randFloat(3.0, 7.0)) * -cos(source->getR() + M_PI);
			float spawnY	= source->getY() - (radius + randFloat(3.0, 7.0)) * (/*0-*/sin(source->getR() + M_PI));
			float speed		= randFloat(75.0, 200.0);
			float spawnVelX = speed * cos(source->getR() + randAngle) + source->getX_vel();
			float spawnVelY = speed * (-sin(source->getR() + randAngle)) + source->getY_vel();

			Position spawnPos = Position(spawnX, spawnY);
			spawnPos.setX_vel(spawnVelX);
			spawnPos.setY_vel(spawnVelY);

			Particle p = Particle(255, randInt(64, 215), 0, 255, 
								randFloat(0.1, 0.3), spawnPos, particleFadeSpeed*4);
			add(p);
		}
	}

	if (selectEngines & 0x2) 
	{ 
		numParticles = randInt(5,8);
		for (int i = 0; i < numParticles; i++) {
			float randAngle = randFloat((M_PI) - (M_PI/9), (M_PI) + M_PI/9);
			float spawnX	= source->getX() - (radius + randFloat(3.0, 7.0) + 6.0) * cos(source->getR() + M_PI + M_PI/5.5);
			float spawnY	= source->getY() - (radius + randFloat(3.0, 7.0) + 6.0) * (-sin(source->getR() + M_PI + M_PI/5.5));
			float speed		= randFloat(75.0, 200.0);
			float spawnVelX = speed * -cos(source->getR() + randAngle) + source->getX_vel();
			float spawnVelY = speed * (sin(source->getR() + randAngle)) + source->getY_vel();

			Position spawnPos = Position(spawnX, spawnY);
			spawnPos.setX_vel(spawnVelX);
			spawnPos.setY_vel(spawnVelY);

			Particle p = Particle(255, randInt(64, 215), 0, 255, 
								randFloat(0.1, 0.3), spawnPos, particleFadeSpeed*8);
			add(p);
		}
		for (int i = 0; i < numParticles; i++) {
			float randAngle = randFloat((M_PI) - (M_PI/9), (M_PI) + M_PI/9);
			float spawnX	= source->getX() - (radius + randFloat(3.0, 7.0) + 6.0) * cos(source->getR() + M_PI - M_PI/5.5);
			float spawnY	= source->getY() - (radius + randFloat(3.0, 7.0) + 6.0) * (-sin(source->getR() + M_PI - M_PI/5.5));
			float speed		= randFloat(75.0, 200.0);
			float spawnVelX = speed * -cos(source->getR() + randAngle) + source->getX_vel();
			float spawnVelY = speed * (sin(source->getR() + randAngle)) + source->getY_vel();

			Position spawnPos = Position(spawnX, spawnY);
			spawnPos.setX_vel(spawnVelX);
			spawnPos.setY_vel(spawnVelY);

			Particle p = Particle(255, randInt(64, 215), 0, 255, 
								randFloat(0.1, 0.3), spawnPos, particleFadeSpeed*8);
			add(p);
		}
	}
	if (selectEngines & 0x4) 
	{ 
		numParticles = randInt(5,8);
		for (int i = 0; i < numParticles; i++) {
			float randAngle = randFloat((M_PI) - (M_PI/9), (M_PI) + M_PI/9);
			float spawnX	= source->getX() - (radius + randFloat(3.0, 7.0) + 20.0) * -cos(source->getR() + M_PI + M_PI/3);
			float spawnY	= source->getY() - (radius + randFloat(3.0, 7.0) + 20.0) * (sin(source->getR() + M_PI + M_PI/3));
			float speed		= randFloat(75.0, 200.0);
			float spawnVelX = speed * cos(source->getR() + randAngle + M_PI/2.5) + source->getX_vel();
			float spawnVelY = speed * (-sin(source->getR() + randAngle + M_PI/2.5)) + source->getY_vel();

			Position spawnPos = Position(spawnX, spawnY);
			spawnPos.setX_vel(spawnVelX);
			spawnPos.setY_vel(spawnVelY);

			Particle p = Particle(255, randInt(64, 215), 0, 255, 
								randFloat(0.1, 0.3), spawnPos, particleFadeSpeed*8);
			add(p);
		}
		for (int i = 0; i < numParticles; i++) {
			float randAngle = randFloat((M_PI) - (M_PI/9), (M_PI) + M_PI/9);
			float spawnX	= source->getX() - (radius + randFloat(3.0, 7.0) + 8.0) * cos(source->getR() + M_PI - M_PI/5.5);
			float spawnY	= source->getY() - (radius + randFloat(3.0, 7.0) + 8.0) * (-sin(source->getR() + M_PI - M_PI/5.5));
			float speed		= randFloat(75.0, 200.0);
			float spawnVelX = speed * -cos(source->getR() + randAngle - M_PI/2.5) + source->getX_vel();
			float spawnVelY = speed * (sin(source->getR() + randAngle - M_PI/2.5)) + source->getY_vel();

			Position spawnPos = Position(spawnX, spawnY);
			spawnPos.setX_vel(spawnVelX);
			spawnPos.setY_vel(spawnVelY);

			Particle p = Particle(255, randInt(64, 215), 0, 255, 
								randFloat(0.1, 0.3), spawnPos, particleFadeSpeed*8);
			add(p);
		}
	}
	if (selectEngines & 0x8) 
	{ 
		numParticles = randInt(5,8);
		for (int i = 0; i < numParticles; i++) {
			float randAngle = randFloat((M_PI) - (M_PI/9), (M_PI) + M_PI/9);
			float spawnX	= source->getX() - (radius + randFloat(3.0, 7.0) + 20.0) * -cos(source->getR() + M_PI - M_PI/3);
			float spawnY	= source->getY() - (radius + randFloat(3.0, 7.0) + 20.0) * (sin(source->getR() + M_PI - M_PI/3));
			float speed		= randFloat(75.0, 200.0);
			float spawnVelX = speed * cos(source->getR() + randAngle - M_PI/2.5) + source->getX_vel();
			float spawnVelY = speed * (-sin(source->getR() + randAngle - M_PI/2.5)) + source->getY_vel();

			Position spawnPos = Position(spawnX, spawnY);
			spawnPos.setX_vel(spawnVelX);
			spawnPos.setY_vel(spawnVelY);

			Particle p = Particle(255, randInt(64, 215), 0, 255, 
								randFloat(0.1, 0.3), spawnPos, particleFadeSpeed*8);
			add(p);
		}
		for (int i = 0; i < numParticles; i++) {
			float randAngle = randFloat((M_PI) - (M_PI/9), (M_PI) + M_PI/9);
			float spawnX	= source->getX() - (radius + randFloat(3.0, 7.0) + 8.0) * cos(source->getR() + M_PI + M_PI/5.5);
			float spawnY	= source->getY() - (radius + randFloat(3.0, 7.0) + 8.0) * (-sin(source->getR() + M_PI + M_PI/5.5));
			float speed		= randFloat(75.0, 200.0);
			float spawnVelX = speed * -cos(source->getR() + randAngle + M_PI/2.5) + source->getX_vel();
			float spawnVelY = speed * (sin(source->getR() + randAngle + M_PI/2.5)) + source->getY_vel();

			Position spawnPos = Position(spawnX, spawnY);
			spawnPos.setX_vel(spawnVelX);
			spawnPos.setY_vel(spawnVelY);

			Particle p = Particle(255, randInt(64, 215), 0, 255, 
								randFloat(0.1, 0.3), spawnPos, particleFadeSpeed*8);
			add(p);
		}
	}
}

void ParticlePool::spark(Position &pos)
{
	int numParticles = randInt(3,6);
	
	for(int ii=0; ii<numParticles; ii++)
	{
		Position particlePos(pos);
		particlePos.impulse( randFloat(-50, 50), randFloat(-50, 50) );
		float whiteness = randInt(0, 255);
		Particle p = Particle(whiteness, whiteness, 255, randInt(170, 255), 0.15, particlePos, 300);
		add(p);
	}
}

void ParticlePool::trail(Position &pos, float radius)
{
	int numParticles = randInt(12,16);
	
	for(int ii=0; ii<numParticles; ii++)
	{
		float r = randFloat(-M_PI, M_PI),
		      d = randFloat(0, radius);
		
		float dx = d * cos(r);
		float dy = d * sin(r);
		
		Position particlePos;
		
		particlePos.setX(pos.getX() + dx);
		particlePos.setY(pos.getY() + dy);
		particlePos.impulse( randFloat(-25, 25), randFloat(-25, 25) );
		Particle p = Particle(255, 255, randInt(150,255), randInt(170, 255), 0.15, particlePos, 300);
		add(p);
	}
}

void ParticlePool::moneyTrail(Position &pos, float radius)
{
	if(randInt(1,3) != 1) return;
	
	float r = randFloat(-M_PI, M_PI),
		    d = randFloat(0, radius);
	
	float dx = d * cos(r);
	float dy = d * sin(r);
	
	Position particlePos;
	
	particlePos.setX(pos.getX() + dx);
	particlePos.setY(pos.getY() + dy);
	//particlePos.impulse( randFloat(-25, 25), randFloat(-25, 25) );
	Particle p = Particle(255, 255, 255, randInt(140, 255), 0.8, particlePos, 150, "images/money-particle.png", true);
	add(p);
}

void ParticlePool::missileTrail(const Position &pos, float radius)
{
	radius *= 0.5;
	float entspeed = pos.getVelocityMagnitude();
	
	for (int i = 0; i < 6; i++)
	{
		float randAngle   = randFloat(M_PI-M_PI/60, M_PI+M_PI/60);
		float speed		  = randFloat(1000.0, 1700.0);
		float offset_time = randFloat(0, getDt()) * (entspeed-speed);
		
		float spawnX	= pos.getX() - (radius + offset_time) * -cos(pos.getR() + M_PI);
		float spawnY	= pos.getY() - (radius + offset_time) *  sin(pos.getR() + M_PI);
		float spawnVelX = speed * ( cos(pos.getR() + randAngle)) + pos.getX_vel();
		float spawnVelY = speed * (-sin(pos.getR() + randAngle)) + pos.getY_vel();

		Position spawnPos = Position(spawnX, spawnY);
		spawnPos.setX_vel(spawnVelX);
		spawnPos.setY_vel(spawnVelY);

		Particle p = Particle(
                         255, 
                         randInt(64, 215), 
                         0, 
                         randInt(128,255), 
						 randFloat(0.16, 0.24), 
                         spawnPos, 
                         900);
		add(p);
	}
}

void ParticlePool::lightningTrail(
    const Position &source, 
    const Position &dest)
{
    const int numParticleSteps = 500;

    float deltaX = dest.getX() - source.getX();
    float deltaY = dest.getY() - source.getY();

    //prevent divide by 0
    if (deltaX == 0.0f)
        deltaX = 0.01f;

    const float slope        = deltaY / deltaX;
    const float distance     = sqrtf(distSquaredBetween(source, dest));
    const float stepDistance = distance / numParticleSteps;
    const float angleBetween = atan2(-deltaY, deltaX);
    float xStep              = stepDistance * cos(angleBetween);

    //check for the correct sign, since it was lost when calculating distance
   /* if (deltaX < 0)
        xStep *= -1;*/

    float yStep = xStep * slope;

    for (int i = 0; i < numParticleSteps; i++)
    {
        float nextX = source.getX() + (i * xStep);
        float nextY = source.getY() + (i * yStep);

        Position spawnPos = Position(nextX, nextY);
        Particle p = Particle(
                         0, 
                         randInt(0, 40),  
                         randInt(220, 255), 
                         randInt(128, 255), 
                         randFloat(0.2f, 0.4f), 
                         spawnPos, 
                         500);

        add(p);
    }
}
