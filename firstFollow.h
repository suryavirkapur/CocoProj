/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/


#ifndef FIRST_FOLLOW_H
#define FIRST_FOLLOW_H

#include "dataStructures.h"
#include "grammar.h"

struct FirstAndFollow* createFirstAndFollowSets();
void computeFirstSets(int** firstVector, int enumId);
void populateFirst(int** firstVector, struct Grammar* parsedGrammar);
void populateFollow(int** followVector, int** firstVector, struct Grammar* parsedGrammar);
void populateFollowTillStable(int** followVector, int** firstVector, struct Grammar* parsedGrammar);
struct FirstAndFollow* computeFirstAndFollowSets(struct Grammar* parsedGrammar);
void verifyFirstAndFollow(struct FirstAndFollow* firstAndFollowSets);
void initialiseCheckIfDone();

#endif 