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
    eInanimate,
    eNonExistent
};

enum {
	BIT_CONDITION = 1,
	BIT_DEFAULT_CONDITION = 2,
	BIT_ALLOW_MUTATE = 4,
	BIT_WAS_BLOCKED = 8,
	BIT_IS_MOTILE = 16,
	BIT_IS_HYPER = 32,
	BIT_WAS_EATEN = 64,
	BIT_WAS_PREYED_ON = 128
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
    
private:
	void computeSpawnEnergy();

private:
	bool canEat(Agent *);

public:

	int		mNumMoveSegments;
	int		mNumMoveAndEatSegments;

	int     mIndex;
    
    // life, energy etc
    eStatus mStatus;
	int		mTurn;
    int     mLifespan;
    float   mEnergy;
	float	mSpawnEnergy;
    int     mSleep;
	int		mDormant;
	int		mDelaySpawnCount;
    Vector3 mSpawnLocation;
    int     mSleepTimeAfterBeingSpawned;

    // moving
    Vector3 mMoveVector;
	int		mFlags;

	int getCondition() { return mFlags & BIT_CONDITION; }
	void setCondition() { mFlags |= BIT_CONDITION; }
	void clearCondition() { mFlags &= ~BIT_CONDITION; }

	int getDefaultCondition() { return mFlags & BIT_DEFAULT_CONDITION; }
	void setDefaultCondition() { mFlags |= BIT_DEFAULT_CONDITION; }
	void clearDefaultCondition() { mFlags &= ~BIT_DEFAULT_CONDITION; }

	int getAllowMutate() { return mFlags & BIT_ALLOW_MUTATE; }
	void setAllowMutate() { mFlags |= BIT_ALLOW_MUTATE; }
	void clearAllowMutate() { mFlags &= ~BIT_ALLOW_MUTATE; }

	int getWasBlocked() { return mFlags & BIT_WAS_BLOCKED; }
	void setWasBlocked() { mFlags |= BIT_WAS_BLOCKED; }
	void clearWasBlocked() { mFlags &= ~BIT_WAS_BLOCKED; }

	int getIsMotile() { return mFlags & BIT_IS_MOTILE; }
	void setIsMotile() { mFlags |= BIT_IS_MOTILE; }
	void clearIsMotile() { mFlags &= ~BIT_IS_MOTILE; }

	int getIsHyper() { return mFlags & BIT_IS_HYPER; }
	void setIsHyper() { mFlags |= BIT_IS_HYPER; }
	void clearIsHyper() { mFlags &= ~BIT_IS_HYPER; }

	int getWasEaten() { return mFlags & BIT_WAS_EATEN; }
	void setWasEaten() { mFlags |= BIT_WAS_EATEN; }
	void clearWasEaten() { mFlags &= ~BIT_WAS_EATEN; }

	int getWasPreyedOn() { return mFlags & BIT_WAS_PREYED_ON; }
	void setWasPreyedOn() { mFlags |= BIT_WAS_PREYED_ON; }
	void clearWasPreyedOn() { mFlags &= ~BIT_WAS_PREYED_ON; }

	/*
	bool	mCondition;
	bool	mDefaultCondition;
    bool    mAllowMutate;
    bool    mWasBlocked;
    bool    mIsMotile;
	bool	mIsHyper;
	bool	mWasEaten;
	bool	mWasPreyedOn;
	*/

    // segments
    int     mNumSegments;
    int     mActiveSegment;
    SphereEntity *mSegments;
	Genome	mGenome;

	int		mNumOccludedPhotosynthesize;
	int		mNumNonOccludedPhotosynthesize;
};

#endif
