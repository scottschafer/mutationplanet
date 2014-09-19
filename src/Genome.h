#ifndef MutationPlanet_Genome_h
#define MutationPlanet_Genome_h

#include "Constants.h"
#include "InstructionSet.h"
#include "Instruction.h"

class Genome {
public:
	Genome();

	int initialize(const char *pInstructions);
	int initializeHasExecutionType(const char *pInstructions);
	int initialize(const Instruction *pInstructions);

	Genome mutate();

	Instruction & operator [](int i) { return mInstructions[i]; }
	operator std::string() { return toString(); }
	const std::string toString();

    Instruction mInstructions[MAX_SEGMENTS + 1];
};

inline const bool operator == (Genome const& lhs, Genome const& rhs) { return memcmp(lhs.mInstructions, rhs.mInstructions, sizeof(lhs.mInstructions)) == 0; }
inline const bool operator != (Genome const& lhs, Genome const& rhs) { return memcmp(lhs.mInstructions, rhs.mInstructions, sizeof(lhs.mInstructions)) != 0; }
inline const bool operator < (Genome const& lhs, Genome const& rhs) { return memcmp(lhs.mInstructions, rhs.mInstructions, sizeof(lhs.mInstructions)) == -1; }
#endif