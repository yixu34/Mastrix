#include "mastrix.hpp"

AIPathfinding::AIPathfinding()
{}

AIPathfinding::~AIPathfinding()
{}

//finds the shortest path to a destination waypoint
//...once we arrive at the destination, we can follow the chain of nodes
//...back to the source.  We fill the traversal stack with these nodes, 
//...starting from the destination and ending with the source, and we pop
//...nodes from the stack as we arrive at each waypoint
void AIPathfinding::constructShortestPathTo(
	WaypointMarker *source, 
	WaypointMarker *destination)
{
	resetWaypoints(source);

	while (!unvisitedNodes.isEmpty())
	{
		WaypointMarker *currentWaypoint = unvisitedNodes.getNearestNode();
		
		if(currentWaypoint->getVisitStatus() == status_visited)
			continue;
		
		currentWaypoint->setVisitStatus(status_visited);

		//found the destination; stop searching and fill up the traversal stack
		if (currentWaypoint == destination)
		{
			constructTraversalSequenceFrom(currentWaypoint);
			return;
		}

		//just popped off the nearest node and marked it as visited.  Now, 
		//insert its -unvisited- neighbors into the unvisited nodes list
		WaypointMarker::EdgePool::const_iterator ii;
		const WaypointMarker::EdgePool &currentEdges = currentWaypoint->getEdges();
		for (ii = currentEdges.begin(); ii != currentEdges.end(); ++ii)
		{
			const WaypointMarker::Edge &currentEdge = (*ii);
			updateEdge(currentWaypoint, currentEdge);
		}
	}

	//by here, we know that no path exists to the destination.
	//to play it safe, we'll pick a random neighbor.  If somehow THAT 
	//fails, then it will just wander around its current waypoint.
	WaypointMarker *randomNeighbor = source->getRandomNeighbor();

	traversalSequence.push_front(randomNeighbor);
}

//empties the traversal sequence.  Useful for switching goals while in the
//middle of traversing an earlier sequence of nodes.
void AIPathfinding::clearTraversalSequence()
{
	traversalSequence.clear();
	//std::stack doesn't seem to have a .clear() method
//	while (!traversalSequence.empty())
//		traversalSequence.pop();
}

bool AIPathfinding::doesPathExistTo(WaypointMarker *destination)
{
    TraversalSequence::iterator ii;
    for (ii = traversalSequence.begin(); ii != traversalSequence.end(); ++ii)
    {
        if ((*ii)->getEntId() == destination->getEntId())
            return true;
    }

    return false;
}

//void AIPathfinding::printTraversalSequence()
//{
//	std::string currentWaypointString = "";
//
//	TraversalSequence::iterator ii;
//	for (ii = traversalSequence.begin(); ii != traversalSequence.end(); ++ii)
//	{
//		float x = (*ii)->getPosition().getX();
//		float y = (*ii)->getPosition().getY();
//		currentWaypointString += retprintf("(%f %f)", x, y);
//	}
//
//	printfToClient(-1, currentWaypointString.c_str());
//}


//if this path to the current neighbor is shorter than the old path, 
//update the current neighbor's weight and set the neighbor's parent
//to the current visited waypoint
void AIPathfinding::updateEdge(
	WaypointMarker *parent, 
	const WaypointMarker::Edge &currentEdge)
{	
	const visit_status neighborStatus = currentEdge.target->getVisitStatus();

	//skip if it's visited
	if (neighborStatus == status_visited)
		return;

	float distBetweenNodes	 = currentEdge.length;	
	WaypointMarker *neighbor = currentEdge.target;
	neighbor->setVisitStatus(status_enqueued);	

	float newWeight = parent->getWeight() + distBetweenNodes;

	//update the neighbor's edge weight and parent pointer if path is shorter
	if (neighbor->getWeight() > newWeight)
	{
		neighbor->setWeight(newWeight);
		neighbor->setParent(parent);
	}

	//now put it into the unvisited nodes list
	unvisitedNodes.addNode(neighbor);
}

void AIPathfinding::resetWaypoints(WaypointMarker *source)
{
	//reset any waypoint markings, so they don't interfere with calculations
	server->unvisitAllWaypoints();
    unvisitedNodes.clear();

	//start off by adding the source waypoint to the unvisited list
	source->setWeight(0);
	unvisitedNodes.addNode(source);
}

WaypointMarker *AIPathfinding::getNextWaypoint()
{
	WaypointMarker *nextWaypoint = &*traversalSequence.front();
	traversalSequence.pop_front();

	return nextWaypoint;
}

//once we know the shortest path, we trace back from the destination to 
//the source.  The bot can ask for intermediate destinations as it moves.
void AIPathfinding::constructTraversalSequenceFrom(WaypointMarker *destination)
{
    bool isEmpty = traversalSequence.empty();
    assert(isEmpty);
    
	traversalSequence.push_front(destination);
	WaypointMarker *currentWaypoint = destination->getParent();

	//now recreate the path by following parent pointers, 
	//starting from the destination
	while (currentWaypoint != 0)
	{
		WaypointMarker *next = currentWaypoint;
		currentWaypoint		 = next->getParent();

		traversalSequence.push_front(next);
	}
}

void AIPathfinding::UnvisitedNodes::addNode(WaypointMarker *node)
{
	unvisitedPQ.push(node);
}

//returns the waypoint with the lowest edge weight, and removes it 
//from the priority queue
WaypointMarker* AIPathfinding::UnvisitedNodes::getNearestNode()
{
	WaypointMarker *nearestWaypoint = unvisitedPQ.top();
	unvisitedPQ.pop();

	return nearestWaypoint;
}

bool AIPathfinding::UnvisitedNodes::isEmpty() const
{
	return unvisitedPQ.empty();
}

void AIPathfinding::UnvisitedNodes::clear()
{
	while (!unvisitedPQ.empty())
		unvisitedPQ.pop();
}
