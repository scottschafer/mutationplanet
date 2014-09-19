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
#include "UtilsGenome.h"
#include "Parameters.h"

#define HYPER_NUM_STEPS 5
#define RESET_CONDITION 0
#define MAX_CROWDING 3

//extern float energyGainMultiplier;

#define energyGainMultiplier 1

Agent::Agent()
{
    mStatus = eNonExistent;
}
 
void Agent::initialize(Vector3 pt, const char * pGenome, bool allowMutation)
{
    if (strlen(pGenome) == 0)
        throw "error";

//	mDefaultCondition = false;//!(UtilsRandom::getRandom() % 2);
	Instruction instructions[MAX_SEGMENTS + 1];
	for (int i = 0; i < (MAX_SEGMENTS+1); i++)
	{
		instructions[i].instruction = pGenome[i];
		instructions[i].executeType = eAlways;
		if (!pGenome[i])
			break;
	}
	initialize(pt, instructions, allowMutation);
	//mCondition = mDefaultCondition;
}

void Agent::initialize(Vector3 pt, const Instruction * pGenome, bool allowMutation)
{
	mNumSegments = mGenome.initialize(pGenome);
	mNumMoveSegments = mNumMoveAndEatSegments = 0;
	for (int i = 0; i < mNumSegments; i++)
	{
		switch (mGenome.mInstructions[i].instruction) {
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
	//getWasEaten() = false;
	//mCondition = false;
	//mWasPreyedOn = false;
    mDormant = 0;

    // spawning
	if (allowMutation)
		setAllowMutate();
    //mAllowMutate = allowMutation;
    mSpawnLocation = pt;
    
    // moving
    //getWasBlocked() = false;
    //mIsHyper = false;

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
    mEnergy = UtilsRandom::getRangeRandom(1.0f, getSpawnEnergy());
    mLifespan = UtilsRandom::getRangeRandom(.75f, 3.0f) *
		(Parameters::instance.baseLifespan + mNumSegments * Parameters::instance.extraLifespanPerSegment);
	mTurn = 0;
    mActiveSegment = 0;
    
    // conditional behavior
    
    float scaleLocation = 1;
    int i = 0;
//    getIsMotile() = false;

	mNumOccludedPhotosynthesize = mNumNonOccludedPhotosynthesize = 0;

	while (i < mNumSegments)
    {
        SphereEntity &segment = mSegments[i];
        
		segment.mType = (*pGenome).instruction;
		++pGenome;
        if (segment.mType == eInstructionMove || segment.mType == eInstructionMoveAndEat)
			setIsMotile();

		segment.mSegmentIndex = i;
        segment.mLocation = pt * scaleLocation;
        segment.mAgent = this;
        segment.mIsOccluded = (mNumSegments > 1);
		if (segment.mType == eInstructionPhotosynthesize)
			if (segment.mIsOccluded)
				++mNumOccludedPhotosynthesize;
			else
				++mNumNonOccludedPhotosynthesize;

        segment.mIsAnchored = false;
        ++i;
    }
    turn(UtilsRandom::getRangeRandom(0, 359));
}

void Agent :: computeSpawnEnergy()
{
	mSpawnEnergy = Parameters::instance.baseSpawnEnergy;
	for (int i = 0; i < mNumSegments; i++)
	{
		float multiplier = 1.0;
		switch (mGenome[i].instruction) {
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
    
	++mTurn;
	if (mSpawnLocation.y > 0) {
		if ((mTurn % Parameters::instance.slowSideSpeed) != 0)
			return;
	}

	if (mDormant) {
		if (mDormant > 0)
			--mDormant;
		return;
	}

	--mLifespan;
	if (mLifespan <= 0)
	{
	// death from age
		die(pWorld);
		return;
	}


	extern bool gDecimateAgents;
	if (gDecimateAgents)
		if ((UtilsRandom::getRandom() % 10) == 0)
		{
			pWorld->killAgent(mIndex);
			return;
		}

	if (mDelaySpawnCount)
		--mDelaySpawnCount;

	mEnergy += mNumNonOccludedPhotosynthesize * Parameters::instance.getPhotosynthesizeBonus() * energyGainMultiplier;

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

	for (int step = 1; step <= numSteps; step++)
	{
		if (mStatus != eAlive)
			break;

		if (mSleep)
		{
			if (mSleep > 0) // send me off to sleep forever more...
			{
				mEnergy -= CYCLE_ENERGY_COST/10;
				--mSleep;
				continue;
			}
			else {
				return;
			}
		}

		// now process the active segment
		Instruction & i = mGenome[mActiveSegment];
		bool isOr = (i.executeType == eAlways) && Parameters::instance.allowOr;
		bool saveCondition = getCondition() != 0;

		if (((i.executeType == eIf) && ! saveCondition) || ((i.executeType == eNotIf) && saveCondition))
		{
			mEnergy -= CYCLE_ENERGY_COST * Parameters::instance.unexecutedTurnCost;
			++numSteps;
		}
		else
		{   
			mEnergy -= CYCLE_ENERGY_COST;
			switch (i.instruction)
			{                
				default:
					break;
            
				case eInstructionMove:
				case eInstructionMoveAndEat: {
					move(pWorld, (i.instruction == eInstructionMoveAndEat));
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
					if (testIsFacingFood(pWorld))
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
					// if this has photo cells, this tests whether it was preyed on
					// else it tests whether is hungry
					//bool flag = /*(mNumOccludedPhotosynthesize+mNumNonOccludedPhotosynthesize) ? mWasPreyedOn : */
					//	(mEnergy < getSpawnEnergy()/4);
					if (getWasPreyedOn()) {
						setCondition(); 
						clearWasPreyedOn();
					}
					else {
						clearCondition();
					}
					break; }

				case eInstructionTestOccluded: {
					bool flag = false;

					if (true)
					{
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
					}
					else
					{
						for (int i = 0; i < mNumSegments; i++)
						{
							SphereEntity & s = mSegments[i];
							if (s.mIsOccluded)
							{
								flag = true;
								break;
							}
						}
					}

					if (flag)
						setCondition();
					else
						clearCondition();
					break; }

				case eInstructionPhotosynthesize:
					if (! mSegments[mActiveSegment].mIsOccluded)
					{
						mEnergy += Parameters::instance.photoSynthesizeEnergyGain * energyGainMultiplier;
					}
					++numSteps;
					break;

				case eInstructionHyper:
					setIsHyper();
					numSteps = HYPER_NUM_STEPS - 1;

					break;
			}

			if (isOr && saveCondition)
				setCondition();
		}

		if (++mActiveSegment >= mNumSegments) {
			mActiveSegment = 0;
			clearIsHyper();

#if RESET_CONDITION
			clearCondition();
#endif
			break;
		}
	}

	if (speed < 10 && !isHyper) {
		mDormant += HYPER_NUM_STEPS-1;
	}

	spawnIfAble(pWorld);
}

void Agent :: advanceOnTestFail()
{
	++mActiveSegment;
	/*
	int nextEndif = mActiveSegment;
	while (nextEndif < mNumSegments)
	{
		if (mSegments[nextEndif].mType == eInstructionEndIf)
		{
			mActiveSegment = nextEndif + 1;
			break;
		}
		++nextEndif;
	}
	*/
}

void Agent :: die(SphereWorld *pWorld, bool andBecomeFood)
{
	if (!Parameters::instance.turnToFoodAfterDeath || ! getIsMotile())
		andBecomeFood = false;

    // after dying, turn into food    
    vector<Vector3> foodPoints;
	if (andBecomeFood)
		for (int i = 0; i < mNumSegments; i += 1)
			if (! mSegments[i].mIsOccluded)
				foodPoints.push_back(mSegments[i].mLocation);
    pWorld->killAgent(mIndex);
    
    if (andBecomeFood)
    {
		float energyPerSegment = getSpawnEnergy() / mNumSegments / 6;
        for (size_t i = 0; i < foodPoints.size(); i++)
        {
            Agent *pNewAgent = pWorld->createEmptyAgent();
            if (pNewAgent)
            {
                pNewAgent->initialize(foodPoints[i], "*", true);
                pWorld->addAgentToWorld(pNewAgent);
                
                // The food left by a dead critter will be low energy Photosynthesize critters.
                //
                // They will initially be dormant, but will eventually spring to life
                // if they aren't eaten.
				pNewAgent->mEnergy = energyPerSegment / 2;
				//pNewAgent->mSleep = -1;
				pNewAgent->mDormant = Parameters::instance.deadCellDormancy;
				/* mSleep = 
					UtilsRandom::getRangeRandom(Parameters::instance.deadCellDormancy/2, Parameters::instance.deadCellDormancy); */
				//break;
            }
        }
    }
}

float Agent::getSpawnEnergy()
{
#if 0
	float moveMult = 2.0f;
	float result = Parameters::instance.baseSpawnEnergy + (float)mNumSegments * Parameters::instance.extraSpawnEnergyPerSegment +
		(float)mNumMoveSegments * Parameters::instance.moveEnergyCost * moveMult +
		(float)mNumMoveAndEatSegments * Parameters::instance.moveAndEatEnergyCost * moveMult;

	return result;
#else
	float moveMult = .3f;
	return Parameters::instance.baseSpawnEnergy + ((float)mNumSegments + 
		mNumMoveSegments * Parameters::instance.moveEnergyCost * moveMult+ 
		mNumMoveAndEatSegments * Parameters::instance.moveAndEatEnergyCost * moveMult) *
		Parameters::instance.extraSpawnEnergyPerSegment;
#endif
}

bool Agent::canEat(Agent * rhs)
{
	if (rhs == this)
		return false;

	if (rhs->mStatus == eNonExistent || rhs->getWasEaten())
		return false;

	if (! Parameters::instance.cannibals && mGenome == rhs->mGenome)
		return false;

	return true;
}

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

	int numEntities = pWorld->getNearbyEntities(newLocation, headSize, entities);

	float biteStrength = Parameters::instance.biteStrength;
	float digestion = Parameters::instance.digestionEfficiency;
	/*
	if (! andEat) {
		biteStrength /= 2;
		digestion = 0;
	}
	*/

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
                break;
            }
        }
        else
        {
			if (andEat && pEntity->mType == eInstructionPhotosynthesize && canEat(pAgent))
            {
				//bool saveBlock = getWasBlocked();
				setWasBlocked();
				if (pAgent->mStatus == eInanimate)
					break;

				// we moved onto a photosynthesize segment through a move and eat instruction, so chomp!
                if (! ate) // only gain the energy from one eating per turn
				{					
					int energyLoss = min(Parameters::instance.extraSpawnEnergyPerSegment * biteStrength, pAgent->mEnergy+1);
					//int energyLoss = min(getSpawnEnergy() * biteStrength, pAgent->mEnergy+1);

					pAgent->mEnergy -= energyLoss;
					float gain = (pAgent->mNumNonOccludedPhotosynthesize + pAgent->mNumOccludedPhotosynthesize);
					gain *= Parameters::instance.extraSpawnEnergyPerSegment * biteStrength * digestion * energyGainMultiplier;
					if (gain > energyLoss) gain = energyLoss;
					mEnergy += gain;

//                    mEnergy += energyLoss * digestion * energyGainMultiplier;
					pAgent->setWasPreyedOn();
					if (pAgent->mEnergy <= 0) {
						pAgent->setWasEaten();
						//pWorld->killAgent(pAgent->mIndex);
		                //getWasBlocked() = saveBlock;
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
    }

	mSleep += Parameters::instance.extraCyclesForMove;

	float cost = andEat ? Parameters::instance.moveAndEatEnergyCost : Parameters::instance.moveEnergyCost;
	mEnergy -= cost;
	if (getWasBlocked()) {
		return;
	}

    // now move
    mSpawnLocation = mSegments[mNumSegments-1].mLocation; // old tail location
    Vector3 oldHeadLocation = mSegments[0].mLocation;

    Vector3 newLocations[MAX_SEGMENTS];
    for (int i = (mNumSegments - 1); i > 0; i--)
        newLocations[i] = mSegments[i-1].mLocation;
    newLocations[0] = newLocation;

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
    
    // offset the spawn location a bit so the child isn't right up against the parent's tail
	if (true) {
		Vector3 spawnLocationOffset = mSpawnLocation - mSegments[mNumSegments-1].mLocation;
		mSpawnLocation.x += spawnLocationOffset.x / 10.0f;
		mSpawnLocation.y += spawnLocationOffset.y / 10.0f;
		mSpawnLocation.z += spawnLocationOffset.z / 10.0f;
		mSpawnLocation.normalize();
	}
	/*
    if (andEat)
		mEnergy -= Parameters::instance.moveAndEatEnergyCost * Parameters::instance.extraCyclesForMove;
    else
        mEnergy -= Parameters::instance.moveEnergyCost * Parameters::instance.extraCyclesForMove;
		*/
    // we might be able to spawn
//    spawnIfAble(pWorld);
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
bool Agent::testIsFacingFood(SphereWorld *pWorld)
{
    int visionDistance = Parameters::instance.lookDistance;
    
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
                        
						case eInstructionFakePhotosynthesize:
							if (canEat(pAgent))
								if (pEntity->mAgent->mGenome != mGenome)
									return true;
							break;

						default:
							isBlocked = true;
					}
				}
			}
			if (isBlocked)
				return false;
		}


		lookVector *= Parameters::instance.lookSpread;
		lookRadius *= Parameters::instance.lookSpread;
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

    if (mEnergy < getSpawnEnergy())
        return;
        
    Vector3 ptLocation = mSpawnLocation;
	if (getIsMotile())
		printf("hey");
    bool neverMoved = ptLocation == mSegments[0].mLocation;
    // if we've never moved, find a nearby spot for the new child. 

	// if we haven't moved and we have multiple segments OR if we're motile, then spawning = death
	/*
	if (neverMoved && (mNumSegments > 1 || getIsMotile()))
	{
		mEnergy = -1000;
		return;
	}
	*/

	int numAttempts = 1;
	SphereEntityPtr entities[24];

	float maxSpawnSpread = 2.0f;
	float minSpawwnSpread = 1.0f;
	float spawnSpread = Parameters::instance.getMoveDistance();

	if (neverMoved) {
		spawnSpread *= maxSpawnSpread;
		// if it hasn't moved, search for a spawn spot
		while (true)
		{
			ptLocation.x = mSpawnLocation.x + UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
			ptLocation.y = mSpawnLocation.y + UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
			ptLocation.z = mSpawnLocation.z + UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
			ptLocation.normalize();
			if (ptLocation.distance(mSpawnLocation) > (spawnSpread/minSpawwnSpread))
				break;
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
			if (getIsMotile()) {
				mEnergy = -1000;
			}
			else {
				mEnergy = getSpawnEnergy() - 1;
				mDormant = Parameters::instance.deadCellDormancy;
			}
			return;
		}
	}
    
    // the child might have a mutant genome...
	Instruction *pInstructions = mGenome.mInstructions;
	Genome mutantGenome;
    bool mutate = getAllowMutate() && Parameters::instance.mutationPercent && UtilsRandom::getRangeRandom(1, 100) <= Parameters::instance.mutationPercent;
    if (mutate)
    {
		mutantGenome = mGenome.mutate();
		pInstructions = mutantGenome.mInstructions;
    }

    Agent *pNewAgent = pWorld->createEmptyAgent();
	if (pNewAgent != NULL && pInstructions[0].instruction != 0)
    {
        pNewAgent->initialize(ptLocation, pInstructions, getAllowMutate());
        // split the energy with the offspring
		mEnergy = getSpawnEnergy() / 2;
		pNewAgent->mEnergy = pNewAgent->getSpawnEnergy() / 2;

        pWorld->addAgentToWorld(pNewAgent);
        //if (! pNewAgent->getIsMotile())
        //    pNewAgent->mEnergy = 0;
		if (pNewAgent->getIsMotile()) {
			//pNewAgent->mDormant = Parameters::instance.sleepTimeAfterBeingSpawned;
			this->mSleep += Parameters::instance.sleepTimeAfterBeingSpawned;
			pNewAgent->mSleep += Parameters::instance.sleepTimeAfterBeingSpawned;
		}
    }
}