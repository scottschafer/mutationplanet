//
//  SphereWorld.h
//  MutationPlanet
//
//  Created by Scott Schafer on 9/10/12.
//  Copyright (c) 2012 SlideRocket. All rights reserved.
//

#ifndef MutationPlanet_SphereWorld_h
#define MutationPlanet_SphereWorld_h

#include "SphereEntity.h"
#include <vector>
#include <set>
#include "Constants.h"
#include "Agent.h"

using namespace gameplay;
using namespace std;

class SphereWorld
{
public:
   
    SphereWorld();
    
    int requestFreeAgentSlot();
    Agent * createEmptyAgent(bool killIfNecessary = false);
    void addAgentToWorld(Agent *);
    void killAgent(int agentIndex);
    
    void step();
    
    int getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults = 16);
    int getNearbyEntities(Vector3 location, float distance, SphereEntity **pResultArray, int maxResults = 16);
    
    void registerEntity(SphereEntity *);
    void unregisterEntity(SphereEntity *);
    void moveEntity(SphereEntity *, Vector3);
    
    void test();
        
public:
    Agent mAgents[MAX_AGENTS];
    SphereEntity mEntites[MAX_AGENTS*MAX_SEGMENTS];
    int mMaxLiveAgentIndex;
    int mNumAgents;

private:
    std::set<int> mFreeSlots;
    std::vector<SphereEntity*> mSortedEntities;
};

#endif
