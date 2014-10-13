//
//  InstructionSet.h
//  MutationPlanet
//
//  Created by Scott Schafer on 12/11/12.
//
//

#ifndef __MutationPlanet__InstructionSet__
#define __MutationPlanet__InstructionSet__

#include <iostream>
#include <set>

enum eSegmentExecutionType {
    eExecTypeMask = 224,
    
    eAlways = 128,
    eIf = 64,
    eNotIf = 0,
    eOr = 224
};

enum eFixed {
    eFixed = 32
};

enum eInstructions
{
    eInstructionMask = 31,
    
    eBarrier1 = 5,
    eBarrier2,
    eBarrier3,
    eBarrier4,
    
    eInstructionPhotosynthesize,
    eInstructionMoveAndEat,
    eInstructionMove,
    eInstructionHyper,
    eInstructionSleep,
    eInstructionTurnLeft,
    eInstructionTurnRight,
    eInstructionHardTurnLeft,
    eInstructionHardTurnRight,
    eInstructionTestSeeFood,
    eInstructionTestBlocked,
    eInstructionTestPreyedOn,
    eInstructionTestOccluded,
    eInstructionSetAnchored,
    eInstructionClearAnchored
};


class InstructionSet
{
public:
    static void reset();
    static char getRandomInstruction();
    static void setInstructionAvailable(char instruction, bool isAvailable);
	static bool instructionSupportsConditions(char instruction);

    static std::set<char> getAllAvailableInstructions();
};

#endif /* defined(__MutationPlanet__InstructionSet__) */
