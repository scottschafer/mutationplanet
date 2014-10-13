//
//  Header.h
//  MutationPlanet
//
//  Created by Scott Schafer on 9/21/12.
//
//

#ifndef MutationPlanet_Header_h
#define MutationPlanet_Header_h


enum {
    MAX_GENOME_LENGTH = 15,
    MAX_SEGMENTS = MAX_GENOME_LENGTH,
    MAX_AGENTS = 20000,
    TURN_ANGLE = 30,
    HARD_TURN_ANGLE = 90,
    NUM_SUBDIVISIONS = 64,
    
	MAX_TOTAL_SEGMENTS = 30000,
	MIN_TURNS_PER_SEC = 100,
	MIN_FPS = 15
};

const float CYCLE_ENERGY_COST = 1.0f;
#endif
