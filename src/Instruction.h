#error no
#ifndef __MutationPlanet__Instruction__
#define __MutationPlanet__Instruction__

enum eSegmentExecutionType {
	eAlways = '1',
	eIf = 'Y',
	eNotIf = 'N'
};

class Instruction {
public:
	Instruction() {}
	Instruction(char instruction, eSegmentExecutionType executeType) : instruction(instruction), executeType(executeType) {}

	char instruction;
	eSegmentExecutionType executeType;

};

#endifm