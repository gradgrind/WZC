#ifndef VIEWHANDLERS_H
#define VIEWHANDLERS_H

#include "database.h"
#include "tt_grid.h"
#include <QWidget>
#include <QListWidget>

class ViewHandler : public QWidget
{
    Q_OBJECT

public:
    ViewHandler();
    void set_data(DBData *db_data, TT_Grid *gridref);

private:
    QListWidget *choice;
    DBData *dbdata;
    TT_Grid *grid;

private slots:
    void handle_load_file();
    void handle_rb_class();
    void handle_class_chosen(int index);
};

#endif // VIEWHANDLERS_H
