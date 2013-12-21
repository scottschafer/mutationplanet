#ifndef __MutationPlanet__Instruction__
#define __MutationPlanet__Instruction__

enum eSegmentExecutionType {
	eAlways = '0',
	eIf = 'y',
	eNotIf = 'n'
};

typedef struct {
	char instruction;
	eSegmentExecutionType executeType;

} Instruction;

#endif