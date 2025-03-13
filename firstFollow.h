#ifndef FIRST_FOLLOW_H
#define FIRST_FOLLOW_H

#include "dataStructures.h"
#include "grammar.h"

struct FirstAndFollow* initializeFirstAndFollow();
void calculateFirst(int** firstVector, int enumId);
void populateFirst(int** firstVector, struct Grammar* g);
void populateFollow(int** followVector, int** firstVector, struct Grammar* g);
void populateFollowTillStable(int** followVector, int** firstVector, struct Grammar* g);
struct FirstAndFollow* computeFirstAndFollowSets(struct Grammar* g);
void printFirstSets(struct FirstAndFollow* fafl);
void printFollowSets(struct FirstAndFollow* fafl);
void verifyFirstAndFollow(struct FirstAndFollow* fafl);
void initialiseCheckIfDone();

#endif 