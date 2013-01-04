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

using namespace gameplay;

class SphereWorld;

enum eConditionalBehavior {
    eSkipToEnd,
    eSkipNextInstruction
    };

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
    void step(SphereWorld * pWorld);

    void die(SphereWorld *pWorld);
    
    void move(SphereWorld *pWorld, bool andEat);
    void turn(int angle);
    void sleep();
    bool testIsFacingFood(SphereWorld *pWorld);
    void spawnIfAble(SphereWorld * pWorld);
    float  getSpawnEnergy();
    
    void handleConditional(bool condition);
    int     mIndex;
    
    // life, energy etc
    eStatus mStatus;
    int     mLifespan;
    float   mEnergy;
    int     mSleep;
    int     mNumMoves;
    
    // spawning
    bool    mAllowMutate;
    Vector3 mSpawnLocation;

    // moving
    int     mAngle;
    Vector3 mMoveVector;
    bool    mWasBlocked;
    
    // segments
    int     mNumSegments;
    int     mActiveSegment;
    int     mCondition;
    SphereEntity *mSegments;
    char    mGenome[MAX_SEGMENTS + 1];
    
    //logic
    eConditionalBehavior mConditionalBehavior;
};

#endif
