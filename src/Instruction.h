#ifndef __MutationPlanet__Instruction__
#define __MutationPlanet__Instruction__

enum eSegmentExecutionType {
	eAlways = '0',
	eIf = 'y',
	eNotIf = 'n'
};

class Instruction {
public:
	Instruction() {}
	Instruction(char instruction, eSegmentExecutionType executeType) : instruction(instruction), executeType(executeType) {}

	char instruction;
	eSegmentExecutionType executeType;

};

#endif