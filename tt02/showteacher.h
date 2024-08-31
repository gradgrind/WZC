#ifndef SHOWTEACHER_H
#define SHOWTEACHER_H

#include "database.h"
#include "tt_grid.h"

class ShowTeacher
{
public:
    ShowTeacher(TT_Grid *grid, DBData *db_data, int teacher_id);
};

#endif // SHOWTEACHER_H
