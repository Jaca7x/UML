#ifndef RELATIONS_H
#define RELATIONS_H
#include "../../lib/raylib.h"

#define MULTIPLICITY_LEN 8

typedef enum
{
    RELATION_ASSOCIATION = 0,
    RELATION_INHERITANCE,
    RELATION_AGGREGATION,
    RELATION_COMPOSITION,
    RELATION_DEPENDENCY,
    RELATION_REALIZATION,
    RELATION_TYPE_COUNT
}RelationType;

typedef struct
{
    int fromId;
    int toId;
    RelationType type;
    char fromMultiplicity[MULTIPLICITY_LEN];
    char toMultiplicity[MULTIPLICITY_LEN];
}UMLRelation;

void ArmRelationMode(void);
void CancelRelationMode(void);
bool IsRelationModeArmed(void);
bool IsConnectingRelation(void);
bool DidRelationsConsumeClick(void);
void UpdateAndDrawRelations(Camera2D camera, int *cursor);

bool HasSelectedRelation(void);
void ClearRelationSelection(void);
void DrawRelationProperties(Rectangle area, int *cursor);

#endif
