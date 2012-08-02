#ifndef CLIENTEDGE_HPP
#define CLIENTEDGE_HPP

#include "position.hpp"

//the client's visual representation of an edge between two waypoints
class ClientEdge
{
public:
	//default constructor to make std::map happy
	ClientEdge()
	{
		point1entID = 0;
		point2entID = 0;
	}

	ClientEdge(int ent1ID, int ent2ID)
	{
		point1entID = ent1ID;
		point2entID = ent2ID;
	}	

	inline int getEnt1ID() const
	{
		return point1entID;
	}

	inline int getEnt2ID() const
	{
		return point2entID;
	}

private:
	int point1entID;
	int point2entID;
};

#endif	//CLIENTEDGE_HPP

