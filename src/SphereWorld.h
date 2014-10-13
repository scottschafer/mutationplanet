//
//  SphereWorld.h
//  MutationPlanet
//
//  Created by Scott Schafer on 9/10/12.
//  Copyright (c) 2012 Scott Schafer. All rights reserved.
//

#ifndef MutationPlanet_SphereWorld_h
#define MutationPlanet_SphereWorld_h

#include "SphereEntity.h"
#include <vector>
#include <set>
#include <fstream>
#include "Constants.h"
#include "Agent.h"

using namespace gameplay;
using namespace std;

class SphereWorld
{
public:
   
    SphereWorld();

	void clear();
    
    int requestFreeAgentSlot();
    Agent * createEmptyAgent(bool killIfNecessary = false);
    void addAgentToWorld(Agent *);
    void killAgent(int agentIndex);
	void killAtLeastNumSegments(int minSegments, int excludingAgent = -1);
    
    int step();
	int getTopCritterIndex() { return mAllowFollow ? mTopCritterIndex : -1; }
	int	getNumAgents() { return mNumAgents; }
	int getMaxLiveAgentIndex() { return mMaxLiveAgentIndex; }
    int getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults = 16);
    int getNearbyEntities(const Vector3 & location, float distance, SphereEntity **pResultArray, int maxResults = 16);
    int getNearbyEntities(const Vector3 & location, float distance, SphereEntity **pResultArray, int maxResults, Agent *pExclude);
	
	Agent & getAgent(int i) { return mAgents[i]; }

    void registerEntity(SphereEntity *);
    void unregisterEntity(SphereEntity *);
    void moveEntity(SphereEntity *, Vector3);
    
    void test();
        
	void read(istream &);
	void write(ostream &);

	void setAllowFollow(bool allowFollow) { mAllowFollow = allowFollow; mTopCritterIndex = -1;}
	bool isFollowing() { return mAllowFollow; }

    void registerMutation(const char * newGenome, const char * parentGenome);
    std::string getParentGenome(const char * genome);
    long getFirstTurn(const char * genome);
    std::vector<std::pair<std::string,int> > & getTopSpecies() { return mTopSpecies; }
    std::set<std::string> & getLivingGenomes() { return mLivingGenomes; }
    void sampleTopSpecies();
    
    void pruneTree(map<string, int> & mapSpeciesToCount);
    void pruneTree();
    
public:
    Agent mAgents[MAX_AGENTS];
    SphereEntity mEntites[MAX_AGENTS*MAX_SEGMENTS];
    int mMaxLiveAgentIndex;
    int mNumAgents;
	int mTopCritterIndex;
	bool mAllowFollow;
    long mCurrentTurn;
    
    std::map<std::string, std::string> mChildToParentGenomes;
    std::map<std::string, long> mGenomeToFirstTurn;
    std::map<std::string, int> mMapSpeciesToCount;
    std::set<std::string> mLivingGenomes;

    std::vector<std::pair<std::string,int> > mTopSpecies;
    
    
private:
    std::set<int> mFreeSlots;

};

#endif
