//
//  Agent.h
//  MutationPlanet
//
//  Created by Scott Schafer on 9/13/12.
//
//

#ifndef MutationPlanet_Agent_h
#define MutationPlanet_Agent_h

#include "constants.h"
#include "gameplay.h"
#include "instruction.h"
#include "Genome.h"

using namespace gameplay;

class SphereWorld;

enum eStatus {
    eAlive,
    eNonExistent,
    eInanimate
};

class SphereEntity;

class Agent
{
public:
    Agent();
    void initialize(Vector3 pt, const char *pGenome, bool allowMutation);
    void initialize(Vector3 pt, const Instruction *pGenome, bool allowMutation);

    void step(SphereWorld * pWorld);

    void die(SphereWorld *pWorld, bool andBecomeFood = true);
    
    void move(SphereWorld *pWorld, bool andEat);
    void turn(int angle);
    void sleep();
    bool testIsFacingFood(SphereWorld *pWorld);
    void spawnIfAble(SphereWorld * pWorld);
    float  getSpawnEnergy();
	void advanceOnTestFail();    
    
	int     mIndex;
    
    // life, energy etc
    eStatus mStatus;
    int     mLifespan;
    float   mEnergy;
    int     mSleep;
	int		mDormant;
	int		mDelaySpawnCount;
	bool	mCondition;
	bool	mDefaultCondition;
    // spawning
    bool    mAllowMutate;
    Vector3 mSpawnLocation;
    int     mSleepTimeAfterBeingSpawned;

    // moving
    Vector3 mMoveVector;
    bool    mWasBlocked;
    bool    mIsMotile;
	bool	mIsHyper;
	bool	mWasEaten;
	bool	mWasPreyedOn;

    // segments
    int     mNumSegments;
    int     mActiveSegment;
    SphereEntity *mSegments;
	Genome	mGenome;
};

#endif
