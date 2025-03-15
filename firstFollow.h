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

void                   computeFirstSets(int mapIndex, int** firstVector);
struct FirstAndFollow* computeFirstAndFollowSets(struct Grammar* parsedGrammar);
void                   verifyFirstAndFollow(struct FirstAndFollow* firstAndFollowSets);

#endif