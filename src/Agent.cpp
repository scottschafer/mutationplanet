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
 the world. It can do basic processing in the 
 ) method.
 
 Right now, the only entities are critters and barriers.
 */



#include "Agent.h"
#include "UtilsRandom.h"
#include "InstructionSet.h"
#include "SphereWorld.h"
//#include "UtilsGenome.h"
#include "Parameters.h"

#define RESET_CONDITION 0
#define MAX_CROWDING 5

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
	float moveDistance = Parameters::instance.getCellSize();
	
	mMoveVector.x = (float(rand()) / float(RAND_MAX)) * moveDistance;
	mMoveVector.y = (float(rand()) / float(RAND_MAX)) * moveDistance;
	mMoveVector.z = (float(rand()) / float(RAND_MAX)) * moveDistance;
	
	Vector3 newLocation = pt + mMoveVector;
	newLocation.normalize();
	mMoveVector = newLocation - pt;
	
	mMoveVector.normalize();
	mMoveVector *= moveDistance;
	
	// segments
	mEnergy = UtilsRandom::getRangeRandom(1.0f, mSpawnEnergy);
	mLifespan = UtilsRandom::getRangeRandom(.75f, 1.25f) *
	(Parameters::instance.baseLifespan + (mNumSegments-1) * Parameters::instance.extraLifespanPerSegment);
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
	float moveMult = 0;//.2f;
	mSpawnEnergy = Parameters::instance.baseSpawnEnergy + ((float)mNumSegments +
														   mNumMoveSegments * Parameters::instance.moveEnergyCost * moveMult+
														   mNumMoveAndEatSegments * Parameters::instance.moveAndEatEnergyCost * moveMult) *
	Parameters::instance.extraSpawnEnergyPerSegment;
	
	//    mSpawnEnergy = Parameters::instance.extraSpawnEnergyPerSegment * mNumSegments;
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

#define USE_TWO_CONDITIONS 0

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
	
	// if close to tjhe poles, slow it down
	++mTurn;
	float poleDistance = mSpawnLocation.y*mSpawnLocation.y;
	if (poleDistance > .5f) {
		
		// from 100% to 10%
		int percentExecuting = (int) (100 - (poleDistance - .5f) * 195);
		
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
	
	float photoBonus = (Parameters::instance.getPhotosynthesizeBonus() - 1.0f);
	
	for (int step = 1; step <= numSteps; step++)
	{
		if (mStatus != eAlive)
			break;
		
		mEnergy += ((float)mNumNonOccludedPhotosynthesize - (float)mNumOccludedPhotosynthesize/2.0f) * photoBonus;
		
		if (mSleep)
		{
			if (mSleep > 0)
			{
				mEnergy -= cycleEnergyCost/3;
				--mSleep;
//				return;
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
		
#if USE_TWO_CONDITIONS
		bool saveCondition = ((mActiveSegment&1) ? getCondition() : getCondition2()) != 0;
#else
		bool saveCondition = getCondition() != 0;
#endif
		
		bool instructionSetCondition = false;
		bool setConditionVal = false;
		
		
		if (((executeType == eIf) && ! saveCondition) || ((executeType == eNotIf) && saveCondition))
		{
			if (mNumSegments == 1) {
				mEnergy -= cycleEnergyCost;
			}
			else {
				mEnergy -= cycleEnergyCost * Parameters::instance.unexecutedTurnCost;
			}
			++numSteps;
		}
		else
		{
			mEnergy -= cycleEnergyCost;
			
			bool doSetCondition = false;
			bool doClearCondition = false;
			
			switch (instruction)
			{
				default:
					break;
					
				case eInstructionMove:
				case eInstructionMoveAndEat: {
					move(pWorld, (instruction == eInstructionMoveAndEat));
					break; }
					
				case eInstructionOrientTowardsPole: {
					orientTowardsPole();
					break; }
					
				case eInstructionTestFacingSibling: {
					if (getIsMotile()) {
						if (testIsFacingSibling(pWorld, 1.0f))
							doSetCondition = true;
						else
							doClearCondition = true;
					}
					break; }
				
				case eInstructionTurnLeft:
					turn(-TURN_ANGLE);
					break;
					
				case eInstructionTurnRight:
					//printf("turn right, energy = %f\n", mEnergy);
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
					if (getIsMotile()) {
						if (testIsFacingFood(pWorld, 1.0f ))
							doSetCondition = true;
						else
							doClearCondition = true;
					}
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
						doSetCondition = true;
						clearWasPreyedOn();
					}
					else {
						doClearCondition = true;
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
						doSetCondition = true;
					else
						doClearCondition = true;
					break; }
					
				case eInstructionPhotosynthesize:
					if (! mSegments[mActiveSegment].mIsOccluded) {
						mEnergy += Parameters::instance.getPhotosynthesizeBonus();
					}
					++numSteps;
					break;
					
				case eInstructionHyper:
					setIsHyper();
					numSteps = HYPER_NUM_STEPS - 1;
					break;
					
				case eInstructionSetAnchored:
					setIsAnchored();
					break;
			}
			
			
#if USE_TWO_CONDITIONS
			if (doSetCondition) {
				if (mActiveSegment&1) setCondition(); else setCondition2();
			}
			if (doClearCondition) {
				if (mActiveSegment&1) clearCondition(); else clearCondition2();
			}
#else
			if (doSetCondition)
				setCondition();
			if (doClearCondition)
				clearCondition();
#endif
		}

		if (++mActiveSegment >= mNumSegments) {
			
			if (mSegments[0].mIsOccluded) {
				this->mEnergy -= cycleEnergyCost * 2;
//				this->die(pWorld);
//				return;
			}
			
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
		for (int i = 0; i < mNumSegments; i += 1) {
			foodPoints.push_back(mSegments[i].mLocation);
		}
	bool allowMutation = getAllowMutate();
	pWorld->killAgent(mIndex);
	
	if (andBecomeFood)
	{
		for (size_t i = 0; i < foodPoints.size(); i++)
		{
			pWorld->addFood(foodPoints[i]);
			
			float distance = Parameters::instance.getCellSize() / 2;
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

#define EAT_IS_ALSO_MOVE 1

/**
 Move a critter and optionally eat if we can
 */
void Agent::move(SphereWorld * pWorld, bool andEat)
{
	bool modifyBlockedFlag = EAT_IS_ALSO_MOVE || ! andEat;
	
	if (modifyBlockedFlag)
		clearWasBlocked();
	
	Vector3 headLocation = mSegments[0].mLocation;

	Vector3 newLocation;
	Vector3 moveVector;
    
    float cellSize = Parameters::instance.getCellSize();
    
	if (mNumSegments > 1 && mSegments[mNumSegments-2].mLocation == mSegments[mNumSegments-1].mLocation) {
		moveVector = mMoveVector;
    }
	else {
		moveVector = mMoveVector / 2;
    }
	
	newLocation = headLocation + moveVector;
	
	newLocation.normalize();
	mMoveVector = newLocation - headLocation;
	mMoveVector.normalize();
	mMoveVector *= cellSize;
	
	SphereEntityPtr entities[16];
	
	float headSize = cellSize;
	if (andEat)
		headSize *= Parameters::instance.mouthSize;
	
	float occludeDist = cellSize * .8f;
	
	Vector3 oldHeadLocation = mSegments[0].mLocation;
	Vector3 oldTailLocation = mSegments[mNumSegments-1].mLocation;
	
	Vector3 newLocations[MAX_SEGMENTS];
	
#if 1
	float segmentMaxFriedenberg = cellSize;
	float segmentMinFriedenberg = segmentMaxFriedenberg / 20;
	
	newLocations[0] = newLocation;
	for (int i = 1; i < mNumSegments; i++) {
        
        /*
          (0,0)
          (0,3)
         
         delta = (0,-3), deltaDistance = 3
         */
        
		Vector3 delta = newLocations[i-1] - mSegments[i].mLocation;
		float deltaDistance = delta.length();
		if (deltaDistance > cellSize) {
            delta.normalize();
            delta *= cellSize;
//            delta *= segmentMaxFriedenberg / deltaDistance;
            //delta.normalize();
            //delta *= segmentMaxFriedenberg;
 			///delta *= segmentMaxFriedenberg / deltaDistance;
            
			newLocations[i] = newLocations[i-1] - delta;
            newLocations[i].normalize();
		}
		else {
			newLocations[i] = mSegments[i].mLocation;
		}
	}
	
#if 0
	if (getIsAnchored()) {
		float deltaTail = (newLocations[mNumSegments-1] - oldTailLocation).length();
		
		if (deltaTail > segmentMinFriedenberg) {
			newLocations[mNumSegments-1] = oldTailLocation;
			
			for (int i = (mNumSegments-2); i >= 0 ; i--) {
				Vector3 delta = newLocations[i] - newLocations[i+1];
				float deltaDistance = delta.length();
				
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
#endif
    
#else
	for (int i = (mNumSegments - 1); i > 0; i--)
		newLocations[i] = mSegments[i-1].mLocation;
	newLocations[0] = newLocation;
#endif
	
	bool ate = false;
	
	float biteStrength = Parameters::instance.biteStrength * (1 + mNumSegments / 5);
	float digestion = Parameters::instance.digestionEfficiency;
	
	int numEntities = pWorld->getNearbyEntities(newLocation, headSize, entities);
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
			if (Parameters::instance.allowSelfOverlap || pEntity->mSegmentIndex < 2 || (pEntity->mLocation == mSegments[0].mLocation))
				continue;
			
			// otherwise, verify that we really hit it, and stop if so
			float trueDistance = calcDistance(newLocation, pEntity->mLocation);
			
			if (trueDistance < headSize)
			{
				if (modifyBlockedFlag)
					setWasBlocked();
				setTouchedSelf();
				break;
			}
		}
		else
		{
			
			if (andEat && pEntity->mType == eInstructionPhotosynthesize && /*!pEntity->mIsOccluded && */ canEat(pAgent))
			{
				//bool saveBlock = getWasBlocked();
				if (modifyBlockedFlag)
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
				if (modifyBlockedFlag)
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
	
//	mSleep += Parameters::instance.extraCyclesForMove;
	
	float cost = andEat ? Parameters::instance.moveAndEatEnergyCost : Parameters::instance.moveEnergyCost;

//	if (andEat && ! ate)
//		cost += cost; // double the cost of eating if it didn't actually eat (maybe)

	/*
	if (mNumSegments > 2) {
		float testDist = moveDistance * .25f;
		for (int i = 1; i < mNumSegments; i++)
		{
			if (mSegments[i].mLocation != newLocations[i])
			{
				
				if (pWorld->getNearbyEntities(newLocations[i], testDist, entities, 1) > 0)
				{
					if (entities[0] != &mSegments[i]) {
						setWasBlocked();
						break;
					}
				}
			}
		}
	}
*/
	
	if (modifyBlockedFlag && getWasBlocked()) {
#if ! EAT_IS_ALSO_MOVE
		mEnergy -= cost * .5f;
#endif
		return;
	}
	mEnergy -= cost;
	
#if ! EAT_IS_ALSO_MOVE
	if (andEat)
		return;
#endif
	
	mSleep += Parameters::instance.extraCyclesForMove;
	if (mNumSegments > 2) {
		float testDist = cellSize * .75f;
		bool useOldMoveMethod = false;
		//useOldMoveMethod = true;
		for (int i = 1; i < mNumSegments; i++)
		{
			if (mSegments[i].mLocation != newLocations[i])
			{
				
				if (pWorld->getNearbyEntities(newLocations[i], testDist, entities, 1, this) > 0)
				{
					//useOldMoveMethod = true;
					break;
				}
			}
		}
		
		if (useOldMoveMethod) {
			for (int i = (mNumSegments-1); i > 0; i--)
			{
				newLocations[i] = mSegments[i-1].mLocation;
			}
		}
	}
	
	// now move
	if (mSegments[mNumSegments-1].mLocation != mSegments[mNumSegments-1].mLocation)
		mSpawnLocation = mSegments[mNumSegments-1].mLocation; // old tail location
	
#if 0
	bool useOldMoveMethod = false;
	Vector3 prevLocation;
	for (int i = 0; i < mNumSegments; i++)
	{
//		Vector3 newLoc = useOldMoveMethod ? prevLocation : newLocations[i];
		Vector3 newLoc;
		if (i == 0) {
			newLoc = newLocations[0];
		}
		else {
			newLoc = (prevLocation + newLocations[i])/2;
		}//= useOldMoveMethod ? prevLocation : newLocations[i];
		prevLocation = mSegments[i].mLocation;

		if (mSegments[i].mLocation != newLoc)
		{
			pWorld->moveEntity(&mSegments[i], newLoc);
			mSegments[0].mIsOccluded = false;
		}
	}
	
	for (int i = 0; i < mNumSegments; i++)
	{
		if (mSegments[i].mType == eInstructionPhotosynthesize)
		{
			int numEntitiesAtLocation = pWorld->getNearbyEntities(newLocations[i], occludeDist, entities, 1, NULL) > 0;
			bool isOccluded = false;//numEntitiesAtLocation > 1;
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
			int numEntities = pWorld->getNearbyEntities(&mSegments[i], cellSize / 2, entities);
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
	mMoveVector *= Parameters::instance.getCellSize();
}

void Agent::orientTowardsPole()
{
	float moveDistance = Parameters::instance.getCellSize();
	
	Vector3 head = mSegments[0].mLocation;
	Vector3 pole(0,(head.y > 0) ? 1 : -1, 0);
	
	Vector3 dif = pole - head;
	dif.normalize();
	dif *= moveDistance;
	
	Vector3 newPos = head + dif;
	newPos.normalize();
	
	dif = newPos - head;
	dif.normalize();
	dif *= moveDistance;
	
	mMoveVector = dif;
}

// Test if we're facing food in the direction that the head is pointing.
// This returns true if we find either a Photosynthesize segment or a FakePhotosynthesize segment
// in the direct line of sight.
bool Agent::testIsFacingFood(SphereWorld *pWorld, float distMultiplier)
{
	float lookspread = Parameters::instance.lookSpread;
	int visionDistance = Parameters::instance.lookDistance * distMultiplier;
	
	Vector3 lookLocation = mSegments[0].mLocation;
	Vector3 lookVector = mMoveVector;
	float lookDistance = lookVector.length();
	
	float lookRadius = Parameters::instance.getCellSize();// * Parameters::instance.mouthSize;
	
	for (int i = 0; i < visionDistance; i++)
	{
		Vector3 oldLocation = lookLocation;
		lookLocation = oldLocation + lookVector;
		lookLocation.normalize();
		lookVector = lookLocation - oldLocation;
		lookVector.normalize();
		lookVector *= lookDistance;
		
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
				if (pAgent && pAgent->mStatus != eNonExistent)
				{
					switch (pEntity->mType)
					{
						case eInstructionPhotosynthesize:
							if (canEat(pAgent))
								return true;
							break;
							
						default:
							if (pAgent == this) {
								if (pEntity != &pAgent->mSegments[0]) {
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
		
		if (i > 3) {
			lookDistance *= lookspread;
			lookRadius *= lookspread;
		}
	}
	
	return false;
}

bool Agent::testIsFacing(SphereWorld *pWorld, float distMultiplier, facingFunction func)
{
	float lookspread = Parameters::instance.lookSpread;
	int visionDistance = Parameters::instance.lookDistance * distMultiplier;
	
	Vector3 lookLocation = mSegments[0].mLocation;
	Vector3 lookVector = mMoveVector;
	float lookDistance = lookVector.length();
	
	float lookRadius = Parameters::instance.getCellSize();// * Parameters::instance.mouthSize;
	
	for (int i = 0; i < visionDistance; i++)
	{
		Vector3 oldLocation = lookLocation;
		lookLocation = oldLocation + lookVector;
		lookLocation.normalize();
		lookVector = lookLocation - oldLocation;
		lookVector.normalize();
		lookVector *= lookDistance;
		
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
				if (pAgent && pAgent->mStatus != eNonExistent)
				{
					switch (func(pAgent, pEntity)) {
						case eFacingTrue:
							return true;
						case eFacingFalse:
							isBlocked = true;
							break;
						case eFacingIgnore:
							break;
					}
				}
			}
				
			if (isBlocked)
				return false;
		}
		
		if (i > 3) {
			lookDistance *= lookspread;
			lookRadius *= lookspread;
		}
	}
	
	return false;
}


static eFacing facingSiblingFunction(Agent *pAgent, SphereEntity *pEntity)
{
	if (pAgent == pEntity->mAgent) {
		return eFacingIgnore;
	}
	if (pAgent->mGenome == pEntity->mAgent->mGenome) {
		return eFacingTrue;
	}
	return eFacingFalse;
}

bool Agent :: testIsFacingSibling(SphereWorld *pWorld, float distMultiplier)
{
	return testIsFacing(pWorld, distMultiplier, facingSiblingFunction);
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
	
/*
	if (pWorld->mNumAgents > (MAX_AGENTS - 100) || pWorld->mNumSegments > (KILL_SEGMENT_THRESHHOLD - 1000)) {
		mEnergy *= .75f;
		return;
	}
*/
	
	Vector3 ptLocation = mSpawnLocation;
	
	bool neverMoved = ptLocation == mSegments[0].mLocation;
	// if we've never moved, find a nearby spot for the new child.
	int numAttempts = 1;
	SphereEntityPtr entities[24];
	
	float spawnSpread = Parameters::instance.getCellSize();
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
				mDormant = 10;
				mEnergy /= 2;
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