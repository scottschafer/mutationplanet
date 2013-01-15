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
    strcpy(mGenome, pGenome);
    if (strlen(pGenome) == 0)
        throw "error";
    
    // life, energy etc
    mStatus = eAlive;
    mEnergy = UtilsRandom::getRangeRandom(1.0f, getSpawnEnergy()-1.0f);
    mSleep = 0;
    
    // spawning
    mAllowMutate = allowMutation;
    mSpawnLocation = pt;
    
    // moving
    mWasBlocked = false;
    
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
    mNumSegments = strlen(mGenome);
    mLifespan = 1000 * mNumSegments + UtilsRandom::getRangeRandom(-500, 500);
    mActiveSegment = 0;
    
    // conditional behavior
    
    float scaleLocation = 1;
    int i = 0;
    mIsMotile = false;
    while (i < mNumSegments)
    {
        SphereEntity &segment = mSegments[i];
        
        segment.mType = *pGenome++;
        if (segment.mType == eInstructionMove || segment.mType == eInstructionMoveAndEat)
            mIsMotile = true;
        
        segment.mSegmentIndex = i;
        segment.mLocation = pt * scaleLocation;
        segment.mAgent = this;
        segment.mIsOccluded = (i != 0);
        segment.mIsAnchored = false;
        ++i;
    }
    turn(UtilsRandom::getRangeRandom(0, 359));
}

/**
 Process the Agent's active segment
 **/
void Agent::step(SphereWorld * pWorld)
{
    int numSteps = 1; // for now, just process one segment at a time
    
    while (numSteps--)
    {
        if (mStatus != eAlive)
            return;
        
        if (mSleep == -1) // send me off to sleep forever more...
            return;
        
        --mLifespan;
        if (mLifespan <= 0)
        {
            // death from age
            die(pWorld);
            return;
        }
        
        if (mSleep)
        {
            // zzzz
            --mSleep;
            return;
        }

        // lose some energy every turn
        mEnergy -= Parameters::cycleEnergyCost;
        
        // now process the active segment
        SphereEntity & s = mSegments[mActiveSegment];
        switch (s.mType)
        {                
            default:
                break;
                
            case eInstructionMove:
            case eInstructionMoveAndEat: {
                move(pWorld, (s.mType == eInstructionMoveAndEat));
                int extraCycles = Parameters::cyclesForMove - 1;
                mSleep += extraCycles;
                mEnergy -= extraCycles * Parameters::cycleEnergyCost;
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
                
            case eInstructionSleep:
                sleep();
                break;
                
            case eInstructionTestSeeFood:
                if (! testIsFacingFood(pWorld))
                    ++mActiveSegment;
                break;
                
            case eInstructionTestNotSeeFood:
                if (testIsFacingFood(pWorld))
                    ++mActiveSegment;
                break;
                
            case eInstructionTestBlocked:
                if (! mWasBlocked)
                    ++mActiveSegment;
                break;
                
            case eInstructionTestNotBlocked:
                if (mWasBlocked)
                    ++mActiveSegment;
                break;
                
            case eInstructionPhotosynthesize:
                if (! s.mIsOccluded)
                {
                    mEnergy += Parameters::photoSynthesizeEnergyGain;
                    spawnIfAble(pWorld);
                }
                break;
        }
        
        if (++mActiveSegment >= mNumSegments)
            mActiveSegment = 0;
        
        if ((mStatus == eAlive) && mEnergy < 0)
            die(pWorld);
    }
}

void Agent :: die(SphereWorld *pWorld)
{
    // after dying, turn into food
    vector<Vector3> foodPoints;
    for (int i = 0; i < mNumSegments; i += 1)
        if (! mSegments[i].mIsOccluded)
            foodPoints.push_back(mSegments[i].mLocation);
    pWorld->killAgent(mIndex);
    
    for (int i = 0; i < foodPoints.size(); i++)
    {
        Agent *pNewAgent = pWorld->createEmptyAgent();
        if (pNewAgent)
        {
            pNewAgent->initialize(foodPoints[i], "*", mAllowMutate);
            pWorld->addAgentToWorld(pNewAgent);
            
            // The food left by a dead critter will be low energy Photosynthesize critters.
            //
            // They will initially be dormant, but will eventually spring to life
            // if they aren't eaten.
            pNewAgent->mEnergy = pNewAgent->getSpawnEnergy() / 3;
            pNewAgent->mSleep = 1000;
        }
    }
}

float Agent::getSpawnEnergy()
{
    return Parameters::baseSpawnEnergy + mNumSegments * Parameters::extraSpawnEnergyPerSegment;
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

    mWasBlocked = false;

    Vector3 headLocation = mSegments[0].mLocation;
    Vector3 newLocation = headLocation + mMoveVector;
    newLocation.normalize();
    mMoveVector = newLocation - headLocation;

    SphereEntityPtr entities[16];
    float moveDistance = Parameters::getMoveDistance();
    int numEntities = pWorld->getNearbyEntities(newLocation, moveDistance, entities);

    for (int i = 0; i < numEntities; i++)
    {
        Agent *pAgent = entities[i]->mAgent;
        SphereEntity * pEntity = entities[i];
        
        // check that the agent is alive, since we might have killed while looping over entities
        if (pAgent->mStatus == eNonExistent)
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
            if (andEat && pEntity->mType == eInstructionPhotosynthesize)
            {
                // we moved onto a photosynthesize segment through a move and eat instruction, so chomp!
                mEnergy += pAgent->mEnergy;
                pWorld->killAgent(pAgent->mIndex);
            }
            else
            {
                mWasBlocked = true;
                return; // can't move over other critter
            }
        }
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
            mSegments[i].mIsOccluded = false;
            pWorld->moveEntity(&mSegments[i], newLocations[i]);
        }
        
        // if we allow self-overlap, mark any photosynthesize segments as occluded if they are overlapping.
        // this prevents an exploit where a critter can protect a photosynthesize segment by curling another
        // segment on top of it
        if (Parameters::allowSelfOverlap && mSegments[i].mType == eInstructionPhotosynthesize && ! mSegments[i].mIsOccluded)
        {
            int numEntities = pWorld->getNearbyEntities(newLocations[i], moveDistance / 2, entities);
            mSegments[i].mIsOccluded = (numEntities > 1);
        }
    }

    // we might be able to spawn
    spawnIfAble(pWorld);
}

// turn by a degress by rotating the head's move vectore
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

        for (int i = 0; i < numEntities; i++)
        {
            SphereEntity *pEntity = entities[i];
            Agent *pAgent = pEntity->mAgent;
            
            // check that the agent is alive, since we might have killed while looping over entities
            if (pAgent && pAgent != this && pAgent->mStatus == eAlive)
            {
                switch (pEntity->mType)
                {
                    case eInstructionPhotosynthesize:
                        return true;
                        
                    case eInstructionFakePhotosynthesize:
                        return strcmp(pEntity->mAgent->mGenome, mGenome) != 0;
                        
                  default:
                    return false;
                }
            }
        }
        lookVector *= 1.05;
        lookRadius *= 1.05;
    }

    return false;
}

void Agent::sleep()
{
    mSleep += SLEEP_TIME;
}

/**
 * If we have enough energy, spawn an offspring
 */
void Agent::spawnIfAble(SphereWorld * pWorld)
{
    if (mEnergy < getSpawnEnergy())
        return;
    
    mEnergy = max(mEnergy, getSpawnEnergy());
    
    Vector3 ptLocation = mSpawnLocation;
    
    // if we've never moved, find a nearby spot for the new child. 
    if (ptLocation == mSegments[0].mLocation)
    {
        while (true)
        {
            ptLocation.x += UtilsRandom::getRangeRandom(-SPAWN_SPREAD, SPAWN_SPREAD);
            ptLocation.y += UtilsRandom::getRangeRandom(-SPAWN_SPREAD, SPAWN_SPREAD);
            ptLocation.z += UtilsRandom::getRangeRandom(-SPAWN_SPREAD, SPAWN_SPREAD);
            ptLocation.normalize();
            if (ptLocation.distance(mSpawnLocation) > (SPAWN_SPREAD/2))
                break;
        }

        SphereEntityPtr entities[16];
        int numEntities = pWorld->getNearbyEntities(ptLocation, SPAWN_SPREAD, entities);
        
        if (numEntities > 8)
        {
            // if there are a lot of entities in the vicinity, then we are shaded and cannot
            // reproduce right now...
            
            for (int i = 0; i < mNumSegments; i++)
                mSegments[i].mIsOccluded = true; // all segments are shaded until they move

            mEnergy = getSpawnEnergy() - 1;
            return;
        }
    }
    
    // the child might have a mutant genome...
    char mutantGenome[MAX_GENOME_LENGTH];
    char * genome = mGenome;

    bool mutate = mAllowMutate && Parameters::mutationPercent && UtilsRandom::getRangeRandom(1, 100) <= Parameters::mutationPercent;
    if (mutate)
    {
        genome = mutantGenome;
        UtilsGenome::mutate(mGenome, mutantGenome);
    }
    
    Agent *pNewAgent = pWorld->createEmptyAgent();
    if (pNewAgent)
    {
        pNewAgent->initialize(ptLocation, genome, mAllowMutate);
        // split the energy with the offspring
        mEnergy = pNewAgent->mEnergy = getSpawnEnergy() / 2;
        pWorld->addAgentToWorld(pNewAgent);
    }
}