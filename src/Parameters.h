//
//  Parameters.h
//  BioSphere
//
//  Created by Scott Schafer on 12/24/12.
//
//

#ifndef __BioSphere__Parameters__
#define __BioSphere__Parameters__

#include <string>
/**
 * All the tweakable parameters
 */

class Parameters
{
public:
	static Parameters instance;

	Parameters();
	//static void serialize(JSONSerializer &);

    float getCellSize();
	float getPhotosynthesizeBonus();
    void reset();
    
    // global
    int speed;
    int mutationPercent;
    int randomFood;
    float cellSize;

    // energy cost / gain
    float moveAndEatEnergyCost;
    float moveEnergyCost;
    float photoSynthesizeEnergyGain;
    //float photoSynthesizeBonus;
    float digestionEfficiency;
    float biteStrength;
	float unexecutedTurnCost;
    int deadCellDormancy;
    
    // spawning
    float baseSpawnEnergy;
    float extraSpawnEnergyPerSegment;
    int sleepTimeAfterBeingSpawned;
    
    // lifespan
    float baseLifespan;
    float extraLifespanPerSegment;
    
    // moving
    int extraCyclesForMove;
    bool allowSelfOverlap;
    
    // looking
    int lookDistance;
    
    // sleep
    int sleepTime;

	int turnToFoodAfterDeath;

	float mouthSize;
	float lookSpread;

	int cannibals;
	int allowOr;

};
#endif /* defined(__BioSphere__Parameters__) */
