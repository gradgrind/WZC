#ifndef VIEWHANDLERS_H
#define VIEWHANDLERS_H

#include "basicconstraints.h"
#include "database.h"
#include "timetabledata.h"
#include "tt_grid.h"
#include <QWidget>
#include <QListWidget>
#include <QGraphicsView>
#include <QRadioButton>

class ViewHandler : public QWidget
{
    Q_OBJECT

public:
    ViewHandler(QGraphicsView *gview);
    ~ViewHandler()
    {
        if (dbdata) delete dbdata;
        if (ttdata) delete ttdata;
        if (grid) delete grid;
        if (basic_constraints) delete basic_constraints;
    }

private:
    QWidget *viewtype;
    QListWidget *choice = nullptr;
    QRadioButton *rb_class;
    QRadioButton *rb_teacher;
    QRadioButton *rb_room;
    QList<int> indexmap;
    DBData *dbdata = nullptr;
    TimetableData *ttdata = nullptr;
    QGraphicsView *view;
    TT_Grid *grid = nullptr;
    BasicConstraints *basic_constraints = nullptr;
    int selected_lid{0};

    bool insertion_unblocked(int lesson_index, std::set<ClashItem> clashes);

private slots:
    void handle_load_file();
    void new_timetable_data();
    void handle_rb_class();
    void handle_rb_teacher();
    void handle_rb_room();
    void handle_item_chosen(int index);
    void onClick(int day, int hour, Tile *tile, int keymod);
    void show_available(Tile *tile);
};

#endif // VIEWHANDLERS_H
