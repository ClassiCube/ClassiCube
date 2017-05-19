#ifndef CS_ENTITYEVENTS_H
#define CS_ENTITYEVENTS_H
#include "EventHandler.h"
#include "Typedefs.h"
/* Events related to the spawning/despawning of entities, and the creation/removal of tab list entries.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Raised when an entity is spawned in the current world. */
Event_EntityID EntityEvents_Added[EventHandler_Size];
Int32 EntityEvents_AddedCount;
#define EntityEvents_RaiseAdded(id)\
EventHandler_Raise_EntityID(EntityEvents_Added, EntityEvents_AddedCount, id);

/* Raised when an entity is despawned from the current world. */
Event_EntityID EntityEvents_Removed[EventHandler_Size];
Int32 EntityEvents_RemovedCount;
#define EntityEvents_RaiseRemoved(id)\
EventHandler_Raise_EntityID(EntityEvents_Removed, EntityEvents_RemovedCount, id);


/* Raised when a tab list entry is created. */
Event_EntityID TabListEvents_Added[EventHandler_Size];
Int32 TabListEvents_AddedCount;
#define TabListEvents_RaiseAdded(id)\
EventHandler_Raise_EntityID(TabListEvents_Added, TabListEvents_AddedCount, id);

/* Raised when a tab list entry is modified. */
Event_EntityID TabListEvents_Changed[EventHandler_Size];
Int32 TabListEvents_ChangedCount;
#define TabListEvents_RaiseChanged(id)\
EventHandler_Raise_EntityID(TabListEvents_Changed, TabListEvents_ChangedCount, id);

/* Raised when a tab list entry is removed. */
Event_EntityID TabListEvents_Removed[EventHandler_Size];
Int32 TabListEvents_RemovedCount;
#define TabListEvents_RaiseRemoved(id)\
EventHandler_Raise_EntityID(TabListEvents_Removed, TabListEvents_RemovedCount, id);
#endif