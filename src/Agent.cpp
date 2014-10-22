/************************************************************************
 MutationPlanet
 Copyright (C) 2012, Scott Schafer, scott.schafer@gmail.com
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/


/*
Agent.cpp
 ---------
 This class defines an entity that has one or more segments which appear on
 the world. It can do basic processing in the step() method.
 
 Right now, the only entities are critters and barriers.
*/



#include "Agent.h"
#include "UtilsRandom.h"
#include "InstructionSet.h"
#include "SphereWorld.h"
//#include "UtilsGenome.h"
#include "Parameters.h"

#define RESET_CONDITION 0
#define MAX_CROWDING 4

Agent::Agent()
{
    mStatus = eNonExistent;
}

void Agent::initialize(Vector3 pt, const char * pGenome, bool allowMutation)
{
	mNumSegments = mGenome.initialize(pGenome);
	mNumMoveSegments = mNumMoveAndEatSegments = 0;
	for (int i = 0; i < mNumSegments; i++)
	{
		switch (mGenome.getInstruction(i)) {
		default: break;

		case eInstructionMove:
			++mNumMoveSegments;
			break;

		case eInstructionMoveAndEat:
			++mNumMoveAndEatSegments;
			break;
		}
	}

	if (mNumSegments == 0)
		throw "error";
	computeSpawnEnergy();

    // life, energy etc
    mStatus = eAlive;
    mSleep = 0;
	mFlags = 0;
    mDormant = 0;

    // spawning
	if (allowMutation)
		setAllowMutate();
    mSpawnLocation = pt;
    // establish initial move vector
    float moveDistance = Parameters::instance.getMoveDistance();

    mMoveVector.x = (float(rand()) / float(RAND_MAX)) * moveDistance;
    mMoveVector.y = (float(rand()) / float(RAND_MAX)) * moveDistance;
    mMoveVector.z = (float(rand()) / float(RAND_MAX)) * moveDistance;
    
    Vector3 newLocation = pt + mMoveVector;
    newLocation.normalize();
    mMoveVector = newLocation - pt;
    
    mMoveVector.normalize();
    mMoveVector.x *= moveDistance;
    mMoveVector.y *= moveDistance;
    mMoveVector.z *= moveDistance;

    // segments
    mEnergy = UtilsRandom::getRangeRandom(1.0f, mSpawnEnergy);
    mLifespan = UtilsRandom::getRangeRandom(.75f, 1.25f) *
		(Parameters::instance.baseLifespan + mNumSegments * Parameters::instance.extraLifespanPerSegment);
	mTurn = 0;
    mActiveSegment = 0;
    
    // conditional behavior
    float scaleLocation = 1;
    int i = 0;

	mNumOccludedPhotosynthesize = mNumNonOccludedPhotosynthesize = 0;

	while (i < mNumSegments)
    {
        SphereEntity &segment = mSegments[i];
        
		segment.mType = (*pGenome) & eInstructionMask;
		++pGenome;
        if (segment.mType == eInstructionMove || segment.mType == eInstructionMoveAndEat)
			setIsMotile();

		segment.mSegmentIndex = i;
        segment.mLocation = pt * scaleLocation;
        segment.mAgent = this;

        // all segments are initially occluded in the case of a critters with multiple segments,
        // where the segments are initially overlapping.
        segment.mIsOccluded = (mNumSegments > 1);
        if (segment.mType == eInstructionPhotosynthesize) {
            if (segment.mIsOccluded) {
				++mNumOccludedPhotosynthesize;
            }
            else {
				++mNumNonOccludedPhotosynthesize;
            }
        }
        ++i;
    }
    turn(UtilsRandom::getRangeRandom(0, 359));
}

void Agent :: computeSpawnEnergy()
{
#if 1
    float moveMult = .3f;
    mSpawnEnergy = Parameters::instance.baseSpawnEnergy + ((float)mNumSegments +
               mNumMoveSegments * Parameters::instance.moveEnergyCost * moveMult+
               mNumMoveAndEatSegments * Parameters::instance.moveAndEatEnergyCost * moveMult) *
                Parameters::instance.extraSpawnEnergyPerSegment;
#else
    mSpawnEnergy = Parameters::instance.baseSpawnEnergy;
	for (int i = 0; i < mNumSegments; i++)
	{
		float multiplier = 1.0;
		switch (mGenome.getInstruction(i)) {
		default:
			break;
		case eInstructionMove:
			multiplier = 5;
			break;
		case eInstructionMoveAndEat:
			multiplier = 20;
			break;
		}
		mSpawnEnergy += Parameters::instance.extraSpawnEnergyPerSegment * multiplier;
	}
#endif
}

/**
 Process the Agent's active segment
 **/
void Agent::step(SphereWorld * pWorld)
{
	if (getWasEaten() || mNumSegments == 0)
	{
		pWorld->killAgent(mIndex);
		return;
	}

	if (mStatus != eAlive)
        return;
    
    // if on the slow slide, only execute one per slowSideSpeed turns
	++mTurn;
    float poleDistance = mSpawnLocation.y*mSpawnLocation.y;
	if (poleDistance > .5f) {
        
        // from 100% to 10%
        int percentExecuting = (int) (100 - (poleDistance - .5f) * 180);
        
        if (percentExecuting > 50) {
            
            int dontExecute = (percentExecuting + percentExecuting - 80) / 10;
            if ((mTurn % dontExecute) == 0)
                return;
        }
        else {
            int doExecute = (120 - percentExecuting - percentExecuting) / 10;
            
            if ((mTurn % doExecute) != 0)
                return;
        }
	}

	if (mDormant) {
		if (mDormant > 0)
			--mDormant;
        if (mDormant == 0 && mSleep == -1)
            mSleep = 0;
		return;
	}

	--mLifespan;
	if (mLifespan <= 0)
	{
	// death from age
		die(pWorld);
		return;
	}

	if (mDelaySpawnCount)
		--mDelaySpawnCount;

    // gain energy for every non-occluded Photosynthesize cell every turn
    mEnergy += mNumNonOccludedPhotosynthesize * Parameters::instance.getPhotosynthesizeBonus();

	bool isHyper = getIsHyper();
	int speed = Parameters::instance.speed;

	// If we're running as fast as we can, then process multiple steps simulatenously
	// Otherwise, sleep if NOT hyper. This makes instructions easier to follow.
	int numSteps = isHyper ? (speed < 10 ? 1 : HYPER_NUM_STEPS) : 1;

	// lose some energy every turn
	if (mEnergy < 0 || getWasEaten())
	{
        die(pWorld, !getWasEaten());
		return;
	}				

    float cycleEnergyCost = CYCLE_ENERGY_COST;// + (mNumSegments-1) * CYCLE_ENERGY_COST / 5;
    
	for (int step = 1; step <= numSteps; step++)
	{
		if (mStatus != eAlive)
			break;

		if (mSleep)
		{
			if (mSleep > 0)
			{
				mEnergy -= cycleEnergyCost/3;
				--mSleep;
				continue;
			}
			else {
                // send me off to sleep forever more...
				return;
			}
		}

		// now process the active segment
        char instruction = mGenome.getInstruction(mActiveSegment);
        eSegmentExecutionType executeType = mGenome.getExecType(mActiveSegment);
        bool isOr = false;//(executeType == eAlways) && Parameters::instance.allowOr;
		bool saveCondition = getCondition() != 0;

		if (((executeType == eIf) && ! saveCondition) || ((executeType == eNotIf) && saveCondition))
		{
			mEnergy -= cycleEnergyCost * Parameters::instance.unexecutedTurnCost;
			++numSteps;
		}
		else
		{   
			mEnergy -= cycleEnergyCost;
			switch (instruction)
			{                
				default:
					break;
            
				case eInstructionMove:
				case eInstructionMoveAndEat: {
					move(pWorld, (instruction == eInstructionMoveAndEat));
					break; }
            
				case eInstructionTurnLeft:
					turn(-TURN_ANGLE);
					break;
            
				case eInstructionTurnRight:
					turn(TURN_ANGLE);
					break;
            
				case eInstructionHardTurnLeft:
					turn(-HARD_TURN_ANGLE);
					break;
            
				case eInstructionHardTurnRight:
					turn(HARD_TURN_ANGLE);
					break;
            
				case eInstructionSleep: {
					int sleepTime = Parameters::instance.sleepTime * ((mActiveSegment % 4)+1);
				    mSleep += sleepTime;
					break; }
            
				case eInstructionTestSeeFood:
					if (testIsFacingFood(pWorld, 1.0f + (float)mActiveSegment / 2))
						setCondition();
					else
						clearCondition();
					break;

				case eInstructionTestBlocked:
					if (getWasBlocked()) {
						setCondition();
						clearWasBlocked();
					}
					else {
						clearCondition();
					}
					break;

				case eInstructionTestPreyedOn: {
					if (getWasPreyedOn()) {
						setCondition(); 
						clearWasPreyedOn();
					}
					else {
						clearCondition();
					}
					break; }

				case eInstructionTestOccluded: {
					bool flag = (mNumOccludedPhotosynthesize > 0);

#if 0
                    SphereEntityPtr entities[20];
                    int numEntities = pWorld->getNearbyEntities(this->mSegments[0].mLocation,
                                                                Parameters::instance.getMoveDistance()*10, entities, sizeof(entities)/sizeof(entities[0]));
                    for (int i = 0; i < numEntities; i++) {
                        if (entities[i]->mAgent != this) {
                            if (entities[i]->mType == eInstructionMoveAndEat)
                            {
                                flag = true;
                                break;
                            }
                        }
                    }
#endif
					if (flag)
						setCondition();
					else
						clearCondition();
					break; }

				case eInstructionTestTouchedSelf:
					if (getTouchedSelf()) {
						setCondition();
						clearTouchedSelf();
					}
					else {
						clearCondition();
					}
					break;

				case eInstructionPhotosynthesize:
					++numSteps;
					break;

				case eInstructionHyper:
					setIsHyper();
					numSteps = HYPER_NUM_STEPS - 1;
					break;
                    
                case eInstructionSetAnchored:
                    setIsAnchored();
                    break;
                    
                case eInstructionClearAnchored:
                    clearIsAnchored();
                    break;
			}

			if (isOr && saveCondition)
				setCondition();
		}

		if (++mActiveSegment >= mNumSegments) {
			mActiveSegment = 0;
			clearIsHyper();
            clearIsAnchored();

#if RESET_CONDITION
			clearCondition();
#endif
            spawnIfAble(pWorld);
			break;
		}
	}

	if (speed < 10 && !isHyper) {
		mDormant += HYPER_NUM_STEPS-1;
	}

}

void Agent :: die(SphereWorld *pWorld, bool andBecomeFood)
{
	if (!Parameters::instance.turnToFoodAfterDeath)// || ! getIsMotile())
		andBecomeFood = false;

    // after dying, turn into food    
    vector<Vector3> foodPoints;
	if (andBecomeFood)
		for (int i = 0; i < mNumSegments; i += 1)
            /*if (! mSegments[i].mIsOccluded || mGenome.getInstruction(i) != eInstructionPhotosynthesize) */ {
				foodPoints.push_back(mSegments[i].mLocation);
            }
    bool allowMutation = getAllowMutate();
    pWorld->killAgent(mIndex);
    
    if (andBecomeFood)
    {
		float energyPerSegment = Parameters::instance.baseSpawnEnergy * mNumSegments / 2;
        for (size_t i = 0; i < foodPoints.size(); i++)
        {
			float distance = Parameters::instance.getMoveDistance() / 2; 
            SphereEntityPtr entities[5];
			int nearbyResults = pWorld->getNearbyEntities(foodPoints[i], distance, entities, sizeof(entities)/sizeof(entities[0]));
			if (nearbyResults > 0)
				continue;

			Agent *pNewAgent = pWorld->createEmptyAgent();
            if (pNewAgent)
            {
                char photogenome[] = {eInstructionPhotosynthesize, 0};
                pNewAgent->initialize(foodPoints[i], photogenome, allowMutation);
                pWorld->addAgentToWorld(pNewAgent);
                
                // The food left by a dead critter will be low energy Photosynthesize critters.
                //
                // They will initially be dormant, but will eventually spring to life
                // if they aren't eaten.
				pNewAgent->mEnergy = energyPerSegment;
				pNewAgent->mDormant = Parameters::instance.deadCellDormancy;
                pNewAgent->mSleep = -1;
            }
        }
    }
}

bool Agent::canEat(Agent * rhs)
{
	if (rhs == this)
		return false;

	if (rhs->mStatus != eAlive || rhs->getWasEaten())\
		return false;

	if (! Parameters::instance.cannibals && mGenome == rhs->mGenome)
		return false;

	return true;
}

#define USE_SQUARED_DISTANCE 0
/**
 Move a critter and optionally eat if we can
 */
void Agent::move(SphereWorld * pWorld, bool andEat)
{
	clearWasBlocked();

    Vector3 headLocation = mSegments[0].mLocation;
    Vector3 newLocation = headLocation + mMoveVector;
    newLocation.normalize();
    mMoveVector = newLocation - headLocation;

	mMoveVector.normalize();
	mMoveVector *= Parameters::instance.getMoveDistance();

    SphereEntityPtr entities[16];
    float moveDistance = Parameters::instance.getMoveDistance();

	float headSize = moveDistance;
	if (andEat)
		headSize *= Parameters::instance.mouthSize;

	float occludeDist = moveDistance * .8f;
    
    Vector3 oldHeadLocation = mSegments[0].mLocation;
    Vector3 oldTailLocation = mSegments[mNumSegments-1].mLocation;

    Vector3 newLocations[MAX_SEGMENTS];
    
#if 1
    
#if USE_SQUARED_DISTANCE
    float segmentMaxFriedenberg = mMoveVector.lengthSquared() * 2.0f;
#else
    float segmentMaxFriedenberg = mMoveVector.length();
#endif
    float segmentMinFriedenberg = segmentMaxFriedenberg / 20;
    
    newLocations[0] = newLocation;
    for (int i = 1; i < mNumSegments; i++) {
        Vector3 delta = mSegments[i].mLocation - newLocations[i-1];
#if USE_SQUARED_DISTANCE
        float deltaDistance = delta.lengthSquared();
#else
        float deltaDistance = delta.length();
#endif
        if (deltaDistance > segmentMaxFriedenberg) {
            delta *= segmentMaxFriedenberg / deltaDistance;
            float newDistance = delta.length();
            newLocations[i] = newLocations[i-1] + delta;
        }
        else {
            newLocations[i] = mSegments[i].mLocation;
        }
    }
    
    if (getIsAnchored()) {
#if USE_SQUARED_DISTANCE
        float deltaTail = (newLocations[mNumSegments-1] - oldTailLocation).lengthSquared();
#else
        float deltaTail = (newLocations[mNumSegments-1] - oldTailLocation).length();
#endif
        
        if (deltaTail > segmentMinFriedenberg) {
            newLocations[mNumSegments-1] = oldTailLocation;
            
            for (int i = (mNumSegments-2); i >= 0 ; i--) {
                Vector3 delta = newLocations[i] - newLocations[i+1];
    #if USE_SQUARED_DISTANCE
                float deltaDistance = delta.lengthSquared();
    #else
                float deltaDistance = delta.length();
    #endif
                
                if (deltaDistance > segmentMaxFriedenberg) {
                    delta *= segmentMaxFriedenberg / deltaDistance;
                    float newDistance = delta.length();
                    newLocations[i] = newLocations[i+1] + delta;
                }
            }
            newLocation = newLocations[0];

            if ((newLocation - mSegments[0].mLocation).lengthSquared() < segmentMinFriedenberg) {
                return;
            }
        }
    }

	for (int i = 1; i < mNumSegments; i++) {
		if (newLocation.distance(newLocations[i]) < occludeDist) {
			setWasBlocked();
			break;
		}
	}
#else
    for (int i = (mNumSegments - 1); i > 0; i--)
        newLocations[i] = mSegments[i-1].mLocation;
    newLocations[0] = newLocation;
#endif
    
	if (! getWasBlocked()) {
	int numEntities = pWorld->getNearbyEntities(newLocation, headSize, entities);

    float biteStrength = Parameters::instance.biteStrength;// * (1 + mNumSegments / 2);
	float digestion = Parameters::instance.digestionEfficiency;

    bool ate = false;
    for (int i = 0; i < numEntities; i++)
    {
        SphereEntity * pEntity = entities[i];
        Agent *pAgent = pEntity->mAgent;
        
        // check that the agent is alive, since we might have killed while looping over entities
		if (pAgent->mStatus == eNonExistent || pAgent->getWasEaten())
            continue;
        
        if (pAgent == this)
        {
            // the segment we hit is our own
            
            // if we allow moving over ourself, or this is the head segment, or the segment is already occluded
            // (which it will be if the segment has never moved), ignore it
            if (Parameters::instance.allowSelfOverlap || pEntity->mSegmentIndex == 0 || pEntity->mIsOccluded)
                continue;

            // otherwise, verify that we really hit it, and stop if so
            float trueDistance = calcDistance(newLocation, pEntity->mLocation);
            
            if (trueDistance < headSize)
            {
				setWasBlocked();
				setTouchedSelf();
                break;
            }
        }
        else
        {
           
			if (andEat && pEntity->mType == eInstructionPhotosynthesize && !pEntity->mIsOccluded && canEat(pAgent))
            {
				//bool saveBlock = getWasBlocked();
				setWasBlocked();

                // we moved onto a photosynthesize segment through a move and eat instruction, so chomp!
                if (! ate) // only gain the energy from one eating per turn
				{
                    float energyLoss = min(Parameters::instance.extraSpawnEnergyPerSegment * biteStrength, pAgent->mEnergy+1);
                    if (pAgent->getIsAnchored()) {
                        energyLoss /= 2;
                    }
                    pAgent->mEnergy -= energyLoss;
                    mEnergy += energyLoss * digestion;
                    /*
					int energyLoss = min(Parameters::instance.extraSpawnEnergyPerSegment * biteStrength, pAgent->mEnergy+1);

					pAgent->mEnergy -= energyLoss;
					float gain = (pAgent->mNumNonOccludedPhotosynthesize + pAgent->mNumOccludedPhotosynthesize);
					gain *= Parameters::instance.extraSpawnEnergyPerSegment * biteStrength * digestion;
					if (gain > energyLoss) gain = energyLoss;
					mEnergy += gain;
                    */
                    if ((mNumOccludedPhotosynthesize+mNumNonOccludedPhotosynthesize) == 0)
                        setWasPreyedOn();

                    pAgent->setWasPreyedOn();
					if (pAgent->mEnergy <= 0) {
						pAgent->setWasEaten();
					}
				}
                ate = true;
				break;
            }
            else
            {
                setWasBlocked(); // can't move over other critter
				if (! andEat)
					break;
            }
        }

		/*
		if (mSegments[i].mLocation != newLocations[i])
        {
			if (mSegments[i].mIsOccluded) {
	            mSegments[i].mIsOccluded = false;
				if (mSegments[i].mType == eInstructionPhotosynthesize)
				{
					++mNumNonOccludedPhotosynthesize;
					--mNumOccludedPhotosynthesize;
				}
			}
            pWorld->moveEntity(&mSegments[i], newLocations[i]);
        }
		*/
    }
	}

	mSleep += Parameters::instance.extraCyclesForMove;

	float cost = andEat ? Parameters::instance.moveAndEatEnergyCost : Parameters::instance.moveEnergyCost;
	mEnergy -= cost;
	if (getWasBlocked()) {
		return;
	}

    // now move
    mSpawnLocation = mSegments[mNumSegments-1].mLocation; // old tail location

#if 1
    for (int i = 0; i < mNumSegments; i++)
    {
        if (mSegments[i].mLocation != newLocations[i])
        {
            pWorld->moveEntity(&mSegments[i], newLocations[i]);
        }
	}

    for (int i = 0; i < mNumSegments; i++)
    {
        if (mSegments[i].mType == eInstructionPhotosynthesize)
        {
			bool isOccluded = pWorld->getNearbyEntities(newLocations[i], occludeDist, entities, 1, NULL) > 0;
			if (isOccluded != mSegments[i].mIsOccluded) {
				mSegments[i].mIsOccluded = isOccluded;
				if (isOccluded) {
					--mNumNonOccludedPhotosynthesize;
					++mNumOccludedPhotosynthesize;
				}
				else {
					++mNumNonOccludedPhotosynthesize;
					--mNumOccludedPhotosynthesize;
				}
			}
        }
	}
#else
    for (int i = 0; i < mNumSegments; i++)
    {
        if (mSegments[i].mLocation != newLocations[i])
        {
			if (mSegments[i].mIsOccluded) {
	            mSegments[i].mIsOccluded = false;
				if (mSegments[i].mType == eInstructionPhotosynthesize)
				{
					++mNumNonOccludedPhotosynthesize;
					--mNumOccludedPhotosynthesize;
				}
			}
            pWorld->moveEntity(&mSegments[i], newLocations[i]);
        }
        
        // if we allow self-overlap, mark any photosynthesize segments as occluded if they are overlapping.
        // this prevents an exploit where a critter can protect a photosynthesize segment by curling another
        // segment on top of it
        if (Parameters::instance.allowSelfOverlap && mSegments[i].mType == eInstructionPhotosynthesize && ! mSegments[i].mIsOccluded)
        {
            int numEntities = pWorld->getNearbyEntities(&mSegments[i], moveDistance / 2, entities);
            if (numEntities) {
                mSegments[i].mIsOccluded = true;
				if (mSegments[i].mType == eInstructionPhotosynthesize)
				{
					--mNumNonOccludedPhotosynthesize;
					++mNumOccludedPhotosynthesize;
				}
			}
        }
    }
#endif

    // offset the spawn location a bit so the child isn't right up against the parent's tail
	if (true) {
		Vector3 spawnLocationOffset = mSpawnLocation - mSegments[mNumSegments-1].mLocation;
		mSpawnLocation.x += spawnLocationOffset.x / 10.0f;
		mSpawnLocation.y += spawnLocationOffset.y / 10.0f;
		mSpawnLocation.z += spawnLocationOffset.z / 10.0f;
		mSpawnLocation.normalize();
	}

}

// turn by a degress by rotating the head's move vector
void Agent::turn(int a)
{
    double r = double(a) / double(360) * MATH_PI * 2;
    Quaternion q(mSegments[0].mLocation, r);
    Matrix rotMat;
    Matrix::createRotation(q, &rotMat);
    
    rotMat.transformPoint(&mMoveVector);
	mMoveVector.normalize();
	mMoveVector *= Parameters::instance.getMoveDistance();
}

// Test if we're facing food in the direction that the head is pointing.
// This returns true if we find either a Photosynthesize segment or a FakePhotosynthesize segment
// in the direct line of sight.
bool Agent::testIsFacingFood(SphereWorld *pWorld, float distMultiplier)
{
	distMultiplier = 1;
	float lookspread = 1.02f; // Parameters::instance.lookSpread
    int visionDistance = Parameters::instance.lookDistance * distMultiplier;
    
    Vector3 lookLocation = mSegments[0].mLocation;
    Vector3 lookVector = mMoveVector;
    float lookRadius = Parameters::instance.getMoveDistance() * Parameters::instance.mouthSize;
    
    for (int i = 0; i < visionDistance; i++)
    {
        Vector3 oldLocation = lookLocation;
        lookLocation = oldLocation + lookVector;
        lookLocation.normalize();
        lookVector = lookLocation - oldLocation;
    
        float moveDistance = Parameters::instance.getMoveDistance();
        SphereEntityPtr entities[16];
		int numEntities = pWorld->getNearbyEntities(lookLocation, lookRadius, entities);

		if (numEntities > 0)
		{
			bool isBlocked = false;
			for (int j = 0; j < numEntities; j++)
			{
				SphereEntity *pEntity = entities[j];
				Agent *pAgent = pEntity->mAgent;
            
				// check that the agent is alive, since we might have killed while looping over entities
				if (pAgent && pAgent != this && pAgent->mStatus != eNonExistent)
				{
					switch (pEntity->mType)
					{
						case eInstructionPhotosynthesize:
							if (canEat(pAgent))
								return true;
							break;
                            
						default:
							if (pAgent == this) {
								if (getWasBlocked()) {
									isBlocked = true;
								}
							}
							else {
								isBlocked = true;
							}
						break;
					}
				}
			}
			if (isBlocked)
				return false;
		}

		if (i > 8) {
			lookVector *= lookspread;
			lookRadius *= lookspread;
		}
    }

    return false;
}

void Agent::sleep()
{
    mSleep += Parameters::instance.sleepTime;
}

/**
 * If we have enough energy, spawn an offspring
 */
void Agent::spawnIfAble(SphereWorld * pWorld)
{
	if (mDelaySpawnCount)
		return;

    if (mEnergy < mSpawnEnergy)
        return;
        
    Vector3 ptLocation = mSpawnLocation;

    bool neverMoved = ptLocation == mSegments[0].mLocation;
    // if we've never moved, find a nearby spot for the new child. 
	int numAttempts = 1;
	SphereEntityPtr entities[24];

    float spawnSpread = Parameters::instance.getMoveDistance();
    float maxSpawnSpread = 2.0f;
    float minSpawnSpread = 1.0f;

    if (neverMoved) {
        float minDistance = spawnSpread * minSpawnSpread;
        float maxDistance = spawnSpread * maxSpawnSpread;
        
        minDistance *= minDistance;
        maxDistance *= maxDistance;
        
        spawnSpread *= maxSpawnSpread;
        
		// if it hasn't moved, search for a spawn spot
        for (int i = 0; i < 10; i++)
		{
			ptLocation.x = mSpawnLocation.x + UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
			ptLocation.y = mSpawnLocation.y + UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
			ptLocation.z = mSpawnLocation.z + UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
			ptLocation.normalize();
            float d = ptLocation.distanceSquared(mSpawnLocation);
            if (d > minDistance && d < maxDistance) {
				break;
            }
		}
	}


	int numEntities = pWorld->getNearbyEntities(ptLocation, spawnSpread, entities, sizeof(entities)/sizeof(entities[0]), this);
    if (numEntities > MAX_CROWDING) {
		if (getIsMotile()) {
			std::set<Agent*> agents;
			for (int i = 0; i < numEntities; i++) {
				Agent * pAgent = entities[i]->mAgent;
				if (pAgent->mStatus == eAlive && getIsMotile() == pAgent->getIsMotile() &&
					(pAgent->mNumSegments > 1 || !pAgent->mDormant)) {
					agents.insert(entities[i]->mAgent);
				}
			}
			numEntities = agents.size();

		}

		if (numEntities > MAX_CROWDING) {
			/* if (! getIsMotile() || neverMoved) */ {
				mDormant = 1000;
				return;
			}

		}
    }

	/*
    if (numEntities > MAX_CROWDING) {


		if (numEntities > MAX_CROWDING) {
            if (! getIsMotile() || neverMoved) {
                mDormant = 1000;
                return;
            }
            else {
//                die(pWorld, false);
//                mDormant = 1000;
//                mEnergy /= 2;
            }
            
            //mDormant = 100;
//            mEnergy /= 2;
            
		}
	}
      */

    // the child might have a mutant genome...
    const char *pInstructions = mGenome;
	Genome mutantGenome;
    bool mutate = getAllowMutate() && Parameters::instance.mutationPercent && UtilsRandom::getRangeRandom(1, 100) <= Parameters::instance.mutationPercent;
    if (mutate)
    {
		mutantGenome = mGenome.mutate();
        pInstructions = mutantGenome;
    }

    Agent *pNewAgent = pWorld->createEmptyAgent();
	if (pNewAgent != NULL && pInstructions[0] != 0)
    {
        pNewAgent->initialize(ptLocation, pInstructions, getAllowMutate());
        // split the energy with the offspring
		mEnergy = mSpawnEnergy / 2;
		pNewAgent->mEnergy = pNewAgent->mSpawnEnergy / 2;
        /*
        if (UtilsRandom::getRangeRandom(0, 20) == 0) {
            pNewAgent->mDormant = 1000;
        }
         */

        pWorld->addAgentToWorld(pNewAgent);
		if (pNewAgent->getIsMotile()) {
			pNewAgent->mDormant = Parameters::instance.sleepTimeAfterBeingSpawned;
			this->mSleep += Parameters::instance.sleepTimeAfterBeingSpawned;
			pNewAgent->mSleep += Parameters::instance.sleepTimeAfterBeingSpawned;
		}

        string parentGenome(mParentGenome);
        if (parentGenome.length() == 0) {
            parentGenome = mGenome;
        }
        string newGenome(pNewAgent->mGenome);
        
        if (parentGenome != newGenome) {
            pWorld->registerMutation(pNewAgent->mGenome, parentGenome.c_str());
            pNewAgent->mParentGenome = mGenome;
            parentGenome = pNewAgent->mParentGenome;
        }
        else {
            pNewAgent->mParentGenome = this->mParentGenome;
        }
    }
}