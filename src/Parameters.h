//
//  Parameters.h
//  BioSphere
//
//  Created by Scott Schafer on 12/24/12.
//
//

#ifndef __BioSphere__Parameters__
#define __BioSphere__Parameters__


/**
 * All the tweakable parameters
 */

class Parameters
{
public:
    static float getMoveDistance();
    static void reset();
    
    // global
    static int speed;
    static int mutationPercent;
    static float cellSize;
    
    // energy cost / gain
    static float moveAndEatEnergyCost;
    static float moveEnergyCost;
    static float photoSynthesizeEnergyGain;
    static float digestionEfficiency;
    static float biteStrength;
    static int deadCellDormancy;
    
    // spawning
    static float baseSpawnEnergy;
    static float extraSpawnEnergyPerSegment;
    static int sleepTimeAfterBeingSpawned;
    
    // lifespan
    static float baseLifespan;
    static float extraLifespanPerSegment;
    
    // moving
    static int extraCyclesForMove;
    static bool allowSelfOverlap;
    
    // looking
    static int lookDistance;
    
    // sleep
    static int sleepTime;
};
#endif /* defined(__BioSphere__Parameters__) */
