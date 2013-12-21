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


Agent::Agent()
{
    mStatus = eNonExistent;
}
 
void Agent::initialize(Vector3 pt, const char * pGenome, bool allowMutation)
{
    if (strlen(pGenome) == 0)
        throw "error";

	mDefaultCondition = !(UtilsRandom::getRandom() % 2);
	Instruction instructions[MAX_SEGMENTS + 1];
	for (int i = 0; i < (MAX_SEGMENTS+1); i++)
	{
		instructions[i].instruction = pGenome[i];
		instructions[i].executeType = eAlways;
		if (!pGenome[i])
			break;
	}
	initialize(pt, instructions, allowMutation);
}

void Agent::initialize(Vector3 pt, const Instruction * pGenome, bool allowMutation)
{
	mNumSegments = mGenome.initialize(pGenome);
    if (mNumSegments == 0)
		throw "error";

    // life, energy etc
    mStatus = eAlive;
    mSleep = 0;
	mWasEaten = false;
	mCondition = false;
	mWasPreyedOn = false;
    mDormant = 0;

    // spawning
    mAllowMutate = allowMutation;
    mSpawnLocation = pt;
    
    // moving
    mWasBlocked = false;
    mIsHyper = false;

    // establish initial move vector
    float moveDistance = Parameters::getMoveDistance();

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
    mLifespan = Parameters::baseLifespan + mNumSegments * Parameters::extraLifespanPerSegment;
    mActiveSegment = 0;
    
    // conditional behavior
    
    float scaleLocation = 1;
    int i = 0;
    mIsMotile = false;

	while (i < mNumSegments)
    {
        SphereEntity &segment = mSegments[i];
        
		segment.mType = (*pGenome).instruction;
		++pGenome;
        if (segment.mType == eInstructionMove || segment.mType == eInstructionMoveAndEat)
            mIsMotile = true;

		segment.mSegmentIndex = i;
        segment.mLocation = pt * scaleLocation;
        segment.mAgent = this;
        segment.mIsOccluded = (mNumSegments != 1);//(i != (mNumSegments-1));
        segment.mIsAnchored = false;
        ++i;
    }
    turn(UtilsRandom::getRangeRandom(0, 359));
}

#define PHOTOSYNTHESIZE_ON_INSTRUCTION 0
#define DOUBLE_MEANING 1

/**
 Process the Agent's active segment
 **/
void Agent::step(SphereWorld * pWorld)
{
	if (mWasEaten || mNumSegments == 0)
	{
		pWorld->killAgent(mIndex);
		return;
	}

	if (mStatus != eAlive)
        return;
    
	extern bool gDecimateAgents;
	if (gDecimateAgents)
		if ((UtilsRandom::getRandom() % 10) == 0)
		{
			pWorld->killAgent(mIndex);
			return;
		}

	--mLifespan;
	if (mLifespan <= 0)
	{
	// death from age
		die(pWorld);
		return;
	}

	if (mDormant) {
		--mDormant;
		return;
	}

	if (mDelaySpawnCount)
		--mDelaySpawnCount;

#if ! PHOTOSYNTHESIZE_ON_INSTRUCTION
	int numPhotosynthesize = 0;
	for (int i = 0; i < mNumSegments; i++)
	{
		SphereEntity & s = mSegments[i];
		if (s.mType == eInstructionPhotosynthesize)
		{
			if (! s.mIsOccluded)
			{
				++numPhotosynthesize;
				mEnergy += (Parameters::photoSynthesizeEnergyGain - CYCLE_ENERGY_COST);
				//didPhotoSynthesize = true;
			}
		}
	}
	/*
	if (numPhotosynthesize) {
		mEnergy += (Parameters::photoSynthesizeEnergyGain - CYCLE_ENERGY_COST) (;//(Parameters::photoSynthesizeEnergyGain - 1.0f);
		if (numPhotosynthesize > 1)
		{
			mEnergy += (Parameters::photoSynthesizeEnergyGain - CYCLE_ENERGY_COST) * (numPhotosynthesize - 1);
		}
	}
	*/
#endif


	int numSteps = (mIsHyper ? 3 : 1);

	// lose some energy every turn
	if (mEnergy < 0 || mWasEaten)
	{
		die(pWorld, !mWasEaten);
		return;
	}				

	for (int step = 1; step <= numSteps; step++)
	{
		if (mStatus != eAlive)
			break;

		mEnergy -= CYCLE_ENERGY_COST;
		if (mSleep && mEnergy > 0)
		{
			if (mSleep == -1) // send me off to sleep forever more...
				return;

			// zzzz
			--mSleep;
			return;
		}

		// now process the active segment
		Instruction & i = mGenome[mActiveSegment];
		if (((i.executeType == eIf) && ! mCondition) || ((i.executeType == eNotIf) && mCondition))
		{
			mEnergy += CYCLE_ENERGY_COST;// / 2;
			numSteps = max(numSteps, 2);
		}
		else
		{
			//mEnergy -= CYCLE_ENERGY_COST;
			//SphereEntity & s = mSegments[mActiveSegment];
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
#if DOUBLE_MEANING
					mCondition = ! mCondition;
#endif
					break;
            
				case eInstructionTurnRight:
					turn(TURN_ANGLE);
#if DOUBLE_MEANING
					mCondition = ! mCondition;
#endif
					break;
            
				case eInstructionHardTurnLeft:
					turn(-HARD_TURN_ANGLE);
#if DOUBLE_MEANING
					mCondition = false;
#endif
					break;
            
				case eInstructionHardTurnRight:
					turn(HARD_TURN_ANGLE);
#if DOUBLE_MEANING
					mCondition = true;
#endif
					break;
            
				case eInstructionSleep:
				    mSleep += Parameters::sleepTime;
					mEnergy += ((Parameters::sleepTime-1) * CYCLE_ENERGY_COST) / 2;
#if DOUBLE_MEANING
					mCondition = false;
#endif
					break;
            
				case eInstructionTestSeeFood:
#if DOUBLE_MEANING
					if (testIsFacingFood(pWorld))
						mCondition = ! mCondition;
#else
					mCondition = testIsFacingFood(pWorld);
#endif
					numSteps = max(numSteps, 2);
					break;

				case eInstructionTestBlocked:
#if DOUBLE_MEANING
					if (mWasBlocked)
						mCondition = ! mCondition;
#else
					mCondition = mWasBlocked;
#endif
					numSteps = max(numSteps, 2);
					break;

				case eInstructionTestPreyedOn: {
					bool flag = (mWasPreyedOn || (mEnergy < Parameters::extraSpawnEnergyPerSegment));
#if DOUBLE_MEANING
					if (flag)
						mCondition = ! mCondition;
#else
					mCondition = flag;
#endif
					numSteps = max(numSteps, 2);
					mWasPreyedOn = false;
					break; }

				case eInstructionTestOccluded: {
					bool flag = false;
					for (int i = 0; i < mNumSegments; i++)
					{
						SphereEntity & s = mSegments[i];
						if (s.mIsOccluded)
						{
							flag = true;
							break;
						}
					}

#if DOUBLE_MEANING
					if (flag)
						mCondition = ! mCondition;
#else
					mCondition = flag;
#endif
					numSteps = max(numSteps, 2);
					break; }

											   
				/*
				case eInstructionClearCondition:
					mCondition = false;
					break;
				case eInstructionTestNotBlocked:
					//if (mWasBlocked)
					//	advanceOnTestFail();
					break;
				*/

				case eInstructionPhotosynthesize:
					mEnergy += CYCLE_ENERGY_COST;
#if PHOTOSYNTHESIZE_ON_INSTRUCTION
					if (! mSegments[mActiveSegment].mIsOccluded)
					{
						mEnergy += Parameters::photoSynthesizeEnergyGain;
						spawnIfAble(pWorld);
					}
#endif
					break;

				case eInstructionHyper:
					mIsHyper = mCondition;
					break;
			}
    
		}

		if (++mActiveSegment >= mNumSegments) {
			mActiveSegment = 0;
		}
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
		float energyPerSegment = UtilsRandom::getRangeRandom(1.0f, Parameters::extraLifespanPerSegment / 2);
        for (size_t i = 0; i < foodPoints.size(); i++)
        {
            Agent *pNewAgent = pWorld->createEmptyAgent();
            if (pNewAgent)
            {
                pNewAgent->initialize(foodPoints[i], "*", true); //mAllowMutate);
                pWorld->addAgentToWorld(pNewAgent);
                
                // The food left by a dead critter will be low energy Photosynthesize critters.
                //
                // They will initially be dormant, but will eventually spring to life
                // if they aren't eaten.
				pNewAgent->mEnergy = pNewAgent->getSpawnEnergy();///5;// / 4;
				
				pNewAgent->mDormant = Parameters::deadCellDormancy;
				/* mSleep = 
					UtilsRandom::getRangeRandom(Parameters::deadCellDormancy/2, Parameters::deadCellDormancy); */
				break;
            }
        }
    }
}

float Agent::getSpawnEnergy()
{
    return /*Parameters::baseSpawnEnergy + */ mNumSegments * Parameters::extraSpawnEnergyPerSegment;
}

/**
 Move a critter and optionally eat if we can
 */
void Agent::move(SphereWorld * pWorld, bool andEat)
{
    if (andEat)
		mEnergy -= Parameters::moveAndEatEnergyCost;
    else
        mEnergy -= Parameters::moveEnergyCost;
	mSleep += mIsHyper ? (Parameters::extraCyclesForMove/2) : Parameters::extraCyclesForMove;

    mWasBlocked = false;

    Vector3 headLocation = mSegments[0].mLocation;
    Vector3 newLocation = headLocation + mMoveVector;
    newLocation.normalize();
    mMoveVector = newLocation - headLocation;

    SphereEntityPtr entities[16];
    float moveDistance = Parameters::getMoveDistance();
    int numEntities = pWorld->getNearbyEntities(newLocation, moveDistance, entities);

    bool ate = false;
    for (int i = 0; i < numEntities; i++)
    {
        Agent *pAgent = entities[i]->mAgent;
        SphereEntity * pEntity = entities[i];
        
        // check that the agent is alive, since we might have killed while looping over entities
		if (pAgent->mStatus == eNonExistent || pAgent->mWasEaten)
            continue;
        
        if (pAgent == this)
        {
            // the segment we hit is our own
            
            // if we allow moving over ourself, or this is the head segment, or the segment is already occluded
            // (which it will be if the segment has never moved), ignore it
            if (Parameters::allowSelfOverlap || pEntity->mSegmentIndex == 0 || pEntity->mIsOccluded)
                continue;

            // otherwise, verify that we really hit it, and stop if so
            float trueDistance = calcDistance(newLocation, pEntity->mLocation);
            
            if (trueDistance < moveDistance)
            {
                mWasBlocked = true;
                return;
            }
        }
        else
        {
			float biteStrength = Parameters::biteStrength;
			float digestion = Parameters::digestionEfficiency;

            if (andEat && pEntity->mType == eInstructionPhotosynthesize)
            {
				//bool saveBlock = mWasBlocked;
                mWasBlocked = true;
				/*
				if (pEntity->mAgent->mStatus == eInanimate)
					return;

				if (pEntity->mType != eInstructionPhotosynthesize) {
					biteStrength /= 5;
					digestion = 0;
				}
				*/

                // we moved onto a photosynthesize segment through a move and eat instruction, so chomp!
                //if (! ate) // only gain the energy from one eating per turn
				{					
					int energyLoss = min(Parameters::extraSpawnEnergyPerSegment * biteStrength, pAgent->mEnergy+1);

                    mEnergy += energyLoss * digestion;
					pAgent->mEnergy -= energyLoss;
					pAgent->mWasPreyedOn = true;
					if (pAgent->mEnergy <= 0) {
						pAgent->mWasEaten = true;					
						//pWorld->killAgent(pAgent->mIndex);
		                //mWasBlocked = saveBlock;
					}
				}
                //ate = true;
				break;
				//return;
            }
            else
            {
                mWasBlocked = true; // can't move over other critter
				return;
                //return; 
            }
        }
    }

	if (mWasBlocked)
		return;

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
            mSegments[i].mIsOccluded = false;
            pWorld->moveEntity(&mSegments[i], newLocations[i]);
        }
        
        // if we allow self-overlap, mark any photosynthesize segments as occluded if they are overlapping.
        // this prevents an exploit where a critter can protect a photosynthesize segment by curling another
        // segment on top of it
        if (Parameters::allowSelfOverlap && mSegments[i].mType == eInstructionPhotosynthesize && ! mSegments[i].mIsOccluded)
        {
            int numEntities = pWorld->getNearbyEntities(&mSegments[i], moveDistance / 2, entities);
            if (numEntities)
                mSegments[i].mIsOccluded = true;
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
		mEnergy -= Parameters::moveAndEatEnergyCost * Parameters::extraCyclesForMove;
    else
        mEnergy -= Parameters::moveEnergyCost * Parameters::extraCyclesForMove;
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
}

// Test if we're facing food in the direction that the head is pointing.
// This returns true if we find either a Photosynthesize segment or a FakePhotosynthesize segment
// in the direct line of sight.
bool Agent::testIsFacingFood(SphereWorld *pWorld)
{
    int visionDistance = Parameters::lookDistance;
    
    Vector3 lookLocation = mSegments[0].mLocation;
    Vector3 lookVector = mMoveVector;
    float lookRadius = Parameters::getMoveDistance();
    
    for (int i = 0; i < visionDistance; i++)
    {
        Vector3 oldLocation = lookLocation;
        lookLocation = oldLocation + lookVector;
        lookLocation.normalize();
        lookVector = lookLocation - oldLocation;
    
        float moveDistance = Parameters::getMoveDistance();
        SphereEntityPtr entities[16];
        int numEntities = pWorld->getNearbyEntities(lookLocation, lookRadius, entities);

		bool result = false;
        for (int j = 0; j < numEntities; j++)
        {
            SphereEntity *pEntity = entities[j];
            Agent *pAgent = pEntity->mAgent;
            
            // check that the agent is alive, since we might have killed while looping over entities
            if (pAgent && pAgent != this && pAgent->mStatus == eAlive)
            {
                switch (pEntity->mType)
                {
                    case eInstructionPhotosynthesize:
						return true;
                        result = true;
						break;
                        
                    case eInstructionFakePhotosynthesize:
                        result |= pEntity->mAgent->mGenome != mGenome;
                        break;
                }
            }
        }

		if (numEntities > 0)
			return result;

		if (i > 3) {
			lookVector *= 1.03f;
			lookRadius *= 1.03f;
		}
    }

    return false;
}

void Agent::sleep()
{
    mSleep += Parameters::sleepTime;
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
    
    //mEnergy = min(mEnergy, getSpawnEnergy());
    
    Vector3 ptLocation = mSpawnLocation;
    
    // if we've never moved, find a nearby spot for the new child. 
    if (ptLocation == mSegments[0].mLocation)
    {
		if (mNumSegments > 1)
		{
			mEnergy = -1000;
			//die(pWorld,false);
			return;
		}

		
		float spawnSpread = Parameters::getMoveDistance() * 4.0f;
        while (true)
        {
            ptLocation.x += UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
            ptLocation.y += UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
            ptLocation.z += UtilsRandom::getRangeRandom(-spawnSpread, spawnSpread);
            ptLocation.normalize();
            if (ptLocation.distance(mSpawnLocation) > (spawnSpread/2))
                break;
        }

		{
        SphereEntityPtr entities[16];
        int numEntities = pWorld->getNearbyEntities(ptLocation, spawnSpread, entities);
        
        if (numEntities > 5)
        {
            // if there are a lot of entities in the vicinity, then we are shaded and cannot
            // reproduce right now.
            
	        mEnergy = getSpawnEnergy() - 1;
			mDormant = 10000;

            //mDelaySpawnCount = mIsMotile ? 1000 : 50000;
            
            //die(pWorld,false);
            
			//mEnergy = -1000;
			//mWasEaten = true;
            /*
            
            for (int i = 0; i < mNumSegments; i++)
                mSegments[i].mIsOccluded = true;

             */
            return;
        }
		}
    }
    
    // the child might have a mutant genome...
	Instruction *pInstructions = mGenome.mInstructions;
	Genome mutantGenome;
    bool mutate = mAllowMutate && Parameters::mutationPercent && UtilsRandom::getRangeRandom(1, 100) <= Parameters::mutationPercent;
    if (mutate)
    {
		mutantGenome = mGenome.mutate();
		pInstructions = mutantGenome.mInstructions;
    }

	if ((pInstructions[0].instruction == eInstructionPhotosynthesize) && (! pInstructions[1].instruction) &&
		(pWorld->getNumAgents() > MAX_SIMPLE_PHOTOSYNTHESIZE))
		return;
    
    Agent *pNewAgent = pWorld->createEmptyAgent();
	if (pNewAgent != NULL && pInstructions[0].instruction != 0)
    {
        pNewAgent->initialize(ptLocation, pInstructions, mAllowMutate);
        // split the energy with the offspring
		mEnergy = getSpawnEnergy();
        mEnergy = pNewAgent->mEnergy = mEnergy * .5;
        pWorld->addAgentToWorld(pNewAgent);
        //if (! pNewAgent->mIsMotile)
        //    pNewAgent->mEnergy = 0;
		if (pNewAgent->mIsMotile)
			pNewAgent->mDormant = Parameters::sleepTimeAfterBeingSpawned;
    }
}