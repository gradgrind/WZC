#ifndef VIEWHANDLERS_H
#define VIEWHANDLERS_H

#include "basicconstraints.h"
#include "database.h"
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
    ~ViewHandler();

private:
    QWidget *viewtype;
    QListWidget *choice = nullptr;
    QRadioButton *rb_class;
    QRadioButton *rb_teacher;
    QRadioButton *rb_room;
    QList<int> indexmap;
    DBData *dbdata = nullptr;
    QGraphicsView *view;
    TT_Grid *grid = nullptr;
    BasicConstraints *basic_constraints = nullptr;

private slots:
    void handle_load_file();
    void handle_rb_class();
    void handle_rb_teacher();
    void handle_rb_room();
    void handle_item_chosen(int index);
    void onClick(int day, int hour, Tile *tile);
};

#endif // VIEWHANDLERS_H
