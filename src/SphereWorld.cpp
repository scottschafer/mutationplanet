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
#include "SpherePointFinderLinkedList.h"
#include "Parameters.h"

template<class V>
void writeBinary(V v, ostream & out)
{
	out.write((const char*)&v, sizeof(V));
}

template<class V>
void readBinary(V &v, istream & in)
{
	in.read((char*)&v, sizeof(V));
}

void writeBinary(const std::string &str, ostream & out);
void writeBinary(const std::string &str, ostream & out)
{
	const char * outStr = str.c_str();
	int len = strlen(outStr);
	writeBinary(len, out);
	if (len > 0)
		out.write(outStr,len);
}

void readBinary(std::string &str, istream & in);
void readBinary(std::string &str, istream & in)
{
	int len;
	readBinary(len, in);
	if (len > 0) {
		char * buf = new char[len+1];
		in.read(buf, len);
		buf[len] = 0;
		str = buf;
		delete[] buf;
	}
}

template<class K, class V>
void writeMap(std::map<K,V> & m, ostream & out) {
    int s = m.size();
	writeBinary(s, out);
    for (typename std::map<K,V>::iterator i = m.begin(); i != m.end(); i++ ) {
		writeBinary(i->first, out);
		writeBinary(i->second, out);
    }
}

template<class K, class V>
void readMap(std::map<K,V> & m, istream & in) {
    
    m.clear();
	int s;
	readBinary(s, in);
	
    while (s-- > 0) {
        K k;
        V v;
		readBinary(k, in);
		readBinary(v, in);
        m[k] = v;
    }
}

static SpherePointFinderLinkedList spherePointFinder;
//static BaseSpherePointFinder * pSpherePointFinder = NULL;
static SphereWorld * pInstance = NULL;


SphereWorld::SphereWorld()
{
    if (pInstance)
        throw "already inited";

//    pSpherePointFinder = new SpherePointFinderLinkedList();

    mMaxLiveAgentIndex = -1;
    mNumAgents = 0;
	mAllowFollow = false;
    mCurrentTurn = 0;
	mNumSegments = 0;
    
    for (int i = 0; i < MAX_AGENTS; i++)
    {
        mAgents[i].mSegments = &this->mEntites[i*MAX_SEGMENTS];
    }
    
    pInstance = this;
}

void SphereWorld :: clear()
{
    for (int i = 0; i < MAX_AGENTS; i++)
    {
        Agent & agent = mAgents[i];
        if (agent.mStatus == eAlive)
            killAgent(i);
    }
	mNumAgents = mMaxLiveAgentIndex = 0;
    mCurrentTurn = 0;
#if LL_FREE_SLOTS
#else
	mFreeSlots.clear();
#endif
	
    mChildToParentGenomes.clear();
    mGenomeToFirstTurn.clear();
}


/**
 get a nonexistent entity, or -1 if none are available
 */
int SphereWorld :: requestFreeAgentSlot()
{
    if (mNumAgents == MAX_AGENTS)
        return -1;
    
    int result;
#if LL_FREE_SLOTS
#else
    if (mFreeSlots.size())
    {
        result = *(mFreeSlots.begin());
        mFreeSlots.erase(result);
    }
    else
#endif
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

void SphereWorld :: reserveAgentCount(int numAgents) {
    while ((mNumAgents + numAgents) >= MAX_AGENTS) {
        for (int i = 0; i < mMaxLiveAgentIndex; i++)
        {
            if (mAgents[i].mStatus == eAlive)
            {
                killAgent(i);
                break;
            }
        }
    }
}

/**
 Return a pointer to an empty agent. If killIfNecessary is true, and we've exceeded the maximum
 number of agents, kill the first one off.
 **/
Agent * SphereWorld :: createEmptyAgent(bool killIfNecessary /* = true */)
{
    if (killIfNecessary && mNumAgents >= MAX_AGENTS)
    {
        reserveAgentCount(1);
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
	if (agentIndex == mTopCritterIndex) {
		mTopCritterIndex = -1;
		mAllowFollow = false;
	}

    if (mAgents[agentIndex].mStatus == eNonExistent)
        throw "attempt to kill non-existent agent";
    
    Agent & agent = mAgents[agentIndex];
    for (int i = 0; i < agent.mNumSegments; i++)
        unregisterEntity(&agent.mSegments[i]);
    agent.mStatus = eNonExistent;
	
#if LL_FREE_SLOTS
#else
	mFreeSlots.insert(agentIndex);
#endif
	
    --mNumAgents;
}

/**
 Give all the agents in the world a chance to process. Also determines the highest
 live index (note that requestFreeAgentSlot() sets this as well)
 **/
int SphereWorld :: step()
{
    ++mCurrentTurn;
    
	if (mTopCritterIndex != -1 && mAgents[mTopCritterIndex].mStatus != eAlive)
		mTopCritterIndex = -1;

	int topCritterIndex = -1;
	int topEnergy = -1;

	int result = 0;
    int lastLiveAgentIndex = -1;
    for (int i = 0; i <= mMaxLiveAgentIndex; i++)
    {
        Agent & agent = mAgents[i];

        if (agent.mStatus == eAlive)
        {
			if (agent.mEnergy > topEnergy) {
				topCritterIndex = i;
				topEnergy = agent.mEnergy;
			}
            lastLiveAgentIndex = i;
            agent.step(this);
			result += agent.mNumSegments;
        }
    }
    
    mMaxLiveAgentIndex = lastLiveAgentIndex;
	if (mTopCritterIndex ==-1)
		mTopCritterIndex = topCritterIndex;

    if (Parameters::instance.randomFood > 0) {
        if ((mCurrentTurn % 100) < Parameters::instance.randomFood) {
            extern Vector3 getRandomSpherePoint();
            addFood(getRandomSpherePoint(), true);
        }
    }

	mNumSegments = result;
	return result;
}

//int getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults = 16);
//int getNearbyEntities(const Vector3 & location, float distance, SphereEntity **pResultArray, int maxResults = 16);
//int getNearbyEntities(const Vector3 & location, float distance, SphereEntity **pResultArray, int maxResults, Agent *pExclude);


int SphereWorld::getNearbyEntities(SphereEntity * pNearEntity, float distance, SphereEntity **pResultArray, int maxResults /*= 16 */)
{
    return spherePointFinder.getNearbyEntities(pNearEntity, distance, pResultArray, maxResults);
}

int SphereWorld::getNearbyEntities(const Vector3 & location, float distance, SphereEntity **pResultArray, int maxResults /* = 16 */)
{
    return spherePointFinder.getNearbyEntities(location, distance, pResultArray, maxResults);
}

int SphereWorld::getNearbyEntities(const Vector3 & location, float distance, SphereEntity **pResultArray, int maxResults /* = 16 */, Agent *pExclude /* = null */)
{
    return spherePointFinder.getNearbyEntities(location, distance, pResultArray, maxResults, pExclude);
}

void SphereWorld :: registerEntity(SphereEntity *pEntity)
{
    spherePointFinder.insert(pEntity);
}

void SphereWorld :: unregisterEntity(SphereEntity *pEntity)
{
    spherePointFinder.remove(pEntity);
}

void SphereWorld :: moveEntity(SphereEntity *pEntity, Vector3 newLoc)
{
    spherePointFinder.moveEntity(pEntity, newLoc);
}

#define TRACE_TEST if(false)TRACE

/**
 Used to test the point finding utility
 **/
void SphereWorld::test()
{
	return;
	srand(0);
    std::vector<SphereEntity*> entities;
    size_t i;
	size_t testSize = 1000;
    SphereEntityPtr results[1000];

	//testPt = (0.310198,-0.572352,-0.980855), pEntity->mLocation = (0.296304,-0.528855,-0.997681), distance = 0.043497
#if 0
	SphereEntity * pEntity = new SphereEntity();
    pEntity->mLocation = Vector3(0.296304f,-0.528855f,-0.997681f);
    registerEntity(pEntity);

	Vector3 testPt(0.310198f,-0.572352f,-0.980855f);

	float distance = calcDistance(testPt, pEntity->mLocation);//.distance(pEntity->mLocation);

    int numEntities = getNearbyEntities(testPt, distance, results);
                
	if (! numEntities) {
		throw "blah";
	}
#else


    for (i = 0; i < testSize; i++)
    {
        Vector3 v(UtilsRandom::getUnitRandom(),UtilsRandom::getUnitRandom(),UtilsRandom::getUnitRandom());
        
        SphereEntity * pEntity = new SphereEntity();
        pEntity->mLocation = v;
        //registerEntity(pEntity);
        
        entities.push_back(pEntity);
    }
    
    for (i = 0; i < entities.size(); i++)
    {
		TRACE_TEST("looking for %d\n", (int)i);

        SphereEntity * pEntity = entities[i];
		registerEntity(pEntity);
		//spherePointFinder.insert(pEntity);

        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
		        float d = UtilsRandom::getRangeRandom(0.01f, .1f);
	    
				Vector3 testPt = pEntity->mLocation;
                testPt.x += UtilsRandom::getRangeRandom(-d, d);
                testPt.y += UtilsRandom::getRangeRandom(-d, d);
                testPt.z += UtilsRandom::getRangeRandom(-d, d);
				testPt.normalize();

				float distance = calcDistance(testPt, pEntity->mLocation);//.distance(pEntity->mLocation);

				TRACE_TEST("testPt = (%f,%f,%f), pEntity->mLocation = (%f,%f,%f), distance = %f\n",
					testPt.x, testPt.y, testPt.z,
					pEntity->mLocation.x, pEntity->mLocation.y, pEntity->mLocation.z,
					distance);

				memset(results,0,sizeof(results));
                int numEntities = getNearbyEntities(testPt, distance, results, testSize);
                
                bool foundEntity = false;
                for (int l = 0; l < numEntities; l++)
                    if (results[l] == pEntity)
                    {
                        foundEntity = true;
                        break;
                    }
                
                if (! foundEntity)
                {
					HEAPCHECK;

					unregisterEntity(pEntity);
                    registerEntity(pEntity);
                    getNearbyEntities(testPt, distance, results, testSize);
                    throw "fail";
                }
            }
        }
		unregisterEntity(pEntity);
    }
    
    //for (i = 0; i < entities.size(); i++)
    //    unregisterEntity(entities[i]);
#endif
}


void SphereWorld :: killAtLeastNumSegments(int toKill, int excludingAgent) {
    while (toKill > 0) {
        
        int i = UtilsRandom::getRangeRandom(0, getMaxLiveAgentIndex());
        if (i == excludingAgent) continue;
        Agent & agent = getAgent(i);
        if (agent.mStatus == eAlive) {
            toKill -= agent.mNumSegments;
            killAgent(i);
        }
    }
}

void SphereWorld::read(istream & in)
{
	spherePointFinder.clear();
	mTopSpecies.clear();
	
	in.read((char*)&mAgents,sizeof(mAgents));
	in.read((char*)&mEntites,sizeof(mEntites));

#if LL_FREE_SLOTS
#else
	mFreeSlots.empty();
#endif
	mNumAgents = mMaxLiveAgentIndex = 0;

	for (int i = 0; i < MAX_AGENTS; i++)
	{
		Agent & agent = mAgents[i];
		agent.mSegments = &this->mEntites[i*MAX_SEGMENTS];
		for (int j = 0; j < MAX_SEGMENTS; j++)
			agent.mSegments[j].mAgent = &agent;

		if (agent.mStatus == eNonExistent) {
#if LL_FREE_SLOTS
#else
			mFreeSlots.insert(i);
#endif
		}
		else {
			++mNumAgents;

			if (agent.mStatus == eAlive) {
				mMaxLiveAgentIndex = i;
			}
		
			for (int j = 0; j < agent.mNumSegments; j++) {
			
				SphereEntity & entity = agent.mSegments[j];
				entity.mSphereNext = NULL;
				entity.mSpherePrev = NULL;
				entity.mWorld = this;
				entity.mSpherePoint.x = -9999; // force registration
				registerEntity(&entity);
			}
		}
	}
    readMap(mChildToParentGenomes, in);
    readMap(mGenomeToFirstTurn, in);
	
}

std::map<std::string, std::string> mChildToParentGenomes;
std::map<std::string, long> mGenomeToFirstTurn;


void SphereWorld::write(ostream & out)
{
    pruneTree();

    out.write((char*)&mAgents,sizeof(mAgents));
	out.write((char*)&mEntites,sizeof(mEntites));
    writeMap(mChildToParentGenomes, out);
    writeMap(mGenomeToFirstTurn, out);
}

void SphereWorld::registerMutation(const char * newGenome, const char * parentGenome)
{
    std::string genome(newGenome);
    map<string,string>::iterator it = mChildToParentGenomes.find(genome);
    if (it == mChildToParentGenomes.end()) {
        
        for (it = mChildToParentGenomes.begin(); it != mChildToParentGenomes.end(); it++)
            if (it->second == newGenome)
                return;
        
        mChildToParentGenomes[genome] = parentGenome;
        mGenomeToFirstTurn[genome] = mCurrentTurn;
    }
}

std::string SphereWorld::getParentGenome(const char * pGenome)
{
    string genome(pGenome);
    map<string,string>::iterator it = mChildToParentGenomes.find(genome);
    if (it == mChildToParentGenomes.end())
        return "";
    else
        return it->second;
}

bool SphereWorld::hasChildGenomes(const char *genome)
{
	for (map<string,string>::iterator i = mChildToParentGenomes.begin(); i != mChildToParentGenomes.end(); i++) {
		if (i->second == genome) {
			return true;
		}
	}
	
	return false;
}

long SphereWorld::getFirstTurn(const char * pGenome)
{
    string genome(pGenome);
    map<string, long>::iterator it = mGenomeToFirstTurn.find(genome);
    if (it == mGenomeToFirstTurn.end())
        return 0;
    else
        return it->second;
}

static bool compareTopSpeciesFunc(const pair<string,int> &p1, const pair<string,int> &p2);
static bool compareTopSpeciesFunc(const pair<string,int> &p1, const pair<string,int> &p2)
{
    return p1.second > p2.second;
}

void SphereWorld::sampleTopSpecies()
{
    map<string, int> mapSpeciesToCount;
    pruneTree(mapSpeciesToCount);
    
    mTopSpecies.clear();

    for (map<string, int>::iterator i = mapSpeciesToCount.begin(); i != mapSpeciesToCount.end(); i++)
        mTopSpecies.push_back(*i);
    
    sort(mTopSpecies.begin(), mTopSpecies.end(), compareTopSpeciesFunc);
}
    
void SphereWorld :: pruneTree() {
    map<string, int> mapSpeciesToCount;
    pruneTree(mapSpeciesToCount);
}

static int pruned = 0;

void SphereWorld::pruneTree(map<string, int> & mapSpeciesToCount) {
	
    // first collect all living genomes
    mLivingGenomes.clear();
    set<string> unprunableGenomes;
    
    for (int i = 0; i <= mMaxLiveAgentIndex; i++)
    {
        Agent & agent = mAgents[i];
        
        if (agent.mStatus == eAlive && agent.mSleep != -1) {
            string genome (agent.mGenome);
            mLivingGenomes.insert(genome);
            
            int count = mapSpeciesToCount[genome];
            ++count;
            mapSpeciesToCount[genome] = count;
            
            string unprunableGenome = genome;
            while (unprunableGenomes.find(unprunableGenome) == unprunableGenomes.end()) {
                unprunableGenomes.insert(unprunableGenome);
                unprunableGenome = mChildToParentGenomes[unprunableGenome];
            }
        }
    }

    std::list<std::string> listErase;
    for (map<string,string>::iterator i = mChildToParentGenomes.begin(); i != mChildToParentGenomes.end(); i++) {
        if (unprunableGenomes.find(i->first) == unprunableGenomes.end() && unprunableGenomes.find(i->second) == unprunableGenomes.end()) {
            listErase.push_back(i->first);
        }
    }
    
	for (std::list<std::string>::iterator i = listErase.begin(); i != listErase.end(); i++) {
        mChildToParentGenomes.erase(*i);
        ++pruned;
    }
	
    if (listErase.size() > 0)
        printf("---- total pruned count = %d\n", pruned);
}

void SphereWorld :: addFood(Vector3 point, bool canSprout /*= true */, float energy /* = 0 */, bool allowMutation /* = false */)
{
    allowMutation = true;
    float distance = Parameters::instance.getCellSize() / 2;
    SphereEntityPtr entities[5];
    int nearbyResults = getNearbyEntities(point, distance, entities, sizeof(entities)/sizeof(entities[0]));
    if (nearbyResults > 0)
        return;
    
    Agent *pNewAgent = createEmptyAgent();
    if (pNewAgent)
    {
        char photogenome[] = {eInstructionPhotosynthesize, 0};
        pNewAgent->initialize(point, photogenome, allowMutation);
        addAgentToWorld(pNewAgent);
        
        if (energy == 0) {
            energy = pNewAgent->getSpawnEnergy() * .5f;
        }
        
        // The food left by a dead critter will be low energy Photosynthesize critters.
        //
        // They will initially be dormant, but will eventually spring to life
        // if they aren't eaten.
        pNewAgent->mEnergy = energy;
        pNewAgent->mDormant = canSprout ? Parameters::instance.deadCellDormancy : -1;
        pNewAgent->mSleep = -1;
    }
}
