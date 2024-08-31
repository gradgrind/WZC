#ifndef SHOWROOM_H
#define SHOWROOM_H

#include "database.h"
#include "tt_grid.h"

class ShowRoom
{
public:
    ShowRoom(TT_Grid *grid, DBData *db_data, int room_id);
};

#endif // SHOWROOM_H
