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


/**
 SphereWorld

 Contains and manages the agents on the world. 
 **/

#include "SphereWorld.h"
#include "Agent.h"
#include "UtilsRandom.h"
#include "SpherePointFinderSpaceDivison.h"

static BaseSpherePointFinder * pSpherePointFinder = NULL;
static SphereWorld * pInstance = NULL;


SphereWorld::SphereWorld()
{
    // = new SpherePointFinderKDTree();
    pSpherePointFinder = new SpherePointFinderSpaceDivision();

    mMaxLiveAgentIndex = -1;
    mNumAgents = 0;

    if (pInstance)
        throw "already inited";
    
    for (int i = 0; i < MAX_AGENTS; i++)
    {
        mAgents[i].mSegments = &this->mEntites[i*MAX_SEGMENTS];
    }
    
    pInstance = this;
}

/**
 get a nonexistent entity, or -1 if none are available
 */
int SphereWorld :: requestFreeAgentSlot()
{
    if (mNumAgents == MAX_AGENTS)
        return -1;
    
    int result;
    if (mFreeSlots.size())
    {
        result = *(mFreeSlots.begin());
        mFreeSlots.erase(mFreeSlots.begin());
    }
    else
    {
        for (result = 0; result < MAX_AGENTS; result++)
            if (mAgents[result].mStatus == eNonExistent)
                break;
    }
    
    if (result > mMaxLiveAgentIndex)
        mMaxLiveAgentIndex = result;
    ++mNumAgents;
    return result;
}

/** 
 Return a pointer to an empty agent. If killIfNecessary is true, and we've exceeded the maximum
 number of agents, kill the first one off.
 **/
Agent * SphereWorld :: createEmptyAgent(bool killIfNecessary /* = true */)
{
    if (killIfNecessary && mNumAgents == MAX_AGENTS)
    {
        for (int i = 0; i < mMaxLiveAgentIndex; i++)
        {
            if (mAgents[i].mStatus == eAlive)
            {
                killAgent(i);
                break;
            }
        }
    }
    
    Agent *pResult = NULL;
    int index = requestFreeAgentSlot();
    if (index != -1)
    {
        pResult = &mAgents[index];
        pResult->mIndex = index;
    }
    return pResult;
}

/**
 Add the agent by registering its segments
 **/
void SphereWorld :: addAgentToWorld(Agent *pAgent)
{
    for (int i = 0; i < pAgent->mNumSegments; i++)
        registerEntity(&pAgent->mSegments[i]);
    pAgent->mStatus = eAlive;
}

/**
 Kill the agent by unregistering its segments and marking its slot as free
 **/
void SphereWorld :: killAgent(int agentIndex)
{
    if (mAgents[agentIndex].mStatus == eNonExistent)
        throw "attempt to kill non-existent agent";
    
    Agent & agent = mAgents[agentIndex];
    for (int i = 0; i < agent.mNumSegments; i++)
        unregisterEntity(&agent.mSegments[i]);
    agent.mStatus = eNonExistent;
    
    mFreeSlots.insert(agentIndex);
    --mNumAgents;
}

/**
 Give all the agents in the world a chance to process. Also determines the highest
 live index (note that requestFreeAgentSlot() sets this as well)
 **/
int SphereWorld :: step()
{
	int result = 0;
    int lastLiveAgentIndex = -1;
    for (int i = 0; i <= mMaxLiveAgentIndex; i++)
    {
        Agent & agent = mAgents[i];
        if (agent.mStatus != eNonExistent)
        {
            lastLiveAgentIndex = i;
            agent.step(this);
			result += agent.mNumSegments;
        }
    }
    
    mMaxLiveAgentIndex = lastLiveAgentIndex;
	return result;
}

int SphereWorld::getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults /*= 16 */)
{
    return pSpherePointFinder->getNearbyEntities(pNearEntity, distance, pResultArray, maxResults);
}

int SphereWorld::getNearbyEntities(Vector3 location, float distance, SphereEntity **pResultArray, int maxResults /* = 16 */)
{
    return pSpherePointFinder->getNearbyEntities(location, distance, pResultArray, maxResults);
}

void SphereWorld :: registerEntity(SphereEntity *pEntity)
{
    pSpherePointFinder->insert(pEntity);
}

void SphereWorld :: unregisterEntity(SphereEntity *pEntity)
{
    pSpherePointFinder->remove(pEntity);
}

void SphereWorld :: moveEntity(SphereEntity *pEntity, Vector3 newLoc)
{
    pSpherePointFinder->moveEntity(pEntity, newLoc);
}

/**
 Not currently called, but used to test the point finding utility
 **/
void SphereWorld::test()
{
    std::vector<SphereEntity*> entities;
    size_t i;
    for (i = 0; i < 10000; i++)
    {
        Vector3 v(UtilsRandom::getUnitRandom(),UtilsRandom::getUnitRandom(),UtilsRandom::getUnitRandom());
        
        SphereEntity * pEntity = new SphereEntity();
        pEntity->mLocation = v;
        registerEntity(pEntity);
        
        entities.push_back(pEntity);
    }
    
    for (i = 0; i < entities.size(); i++)
    {
        SphereEntity * pEntity = entities[i];
        for (int j = 0; j < 4; j++)
        {
            float d = UtilsRandom::getRangeRandom(0.01f, .1f);
            for (int k = 0; k < 4; k++)
            {
                Vector3 testPt = pEntity->mLocation;
                testPt.x += UtilsRandom::getRangeRandom(-d, d);
                testPt.y += UtilsRandom::getRangeRandom(-d, d);
                testPt.z += UtilsRandom::getRangeRandom(-d, d);
                
                SphereEntityPtr entities[10000];
                int numEntities = getNearbyEntities(testPt, d, entities, 10000);
                
                bool foundEntity = false;
                for (int l = 0; l < numEntities; l++)
                    if (entities[l] == pEntity)
                    {
                        foundEntity = true;
                        break;
                    }
                
                if (! foundEntity)
                {
                    unregisterEntity(pEntity);
                    registerEntity(pEntity);
                    getNearbyEntities(testPt, d, entities, 10000);
                    throw "fail";
                }
            }
        }
    }
    
    for (i = 0; i < entities.size(); i++)
        unregisterEntity(entities[i]);
}

