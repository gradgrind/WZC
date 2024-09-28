#ifndef TT_GRID_H
#define TT_GRID_H

#include <QApplication>
#include "canvas.h"
#include "chip.h"
//#include "database.h"
#include <QStringList>

class Cell : public Chip
{
public:
    enum { Type = UserType + 3 };
    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

    Cell(int x, int y);

    int cellx;
    int celly;
};

class Tile; // forward declaration

class TT_Grid
{
public:
    TT_Grid(
        QGraphicsView *view,
        QStringList days,
        QStringList hours,
        QList<int> breaks);
    ~TT_Grid();

    void setup_grid();
    void place_tile(Tile *tile, int col, int row);
    void setClickHandler(
        std::function<void (int day, int hour, Tile *tile)> handler);
    void select_tile(Tile *tile);
    void setCellOK(int day, int hour);
    void clearCellOK();

    Canvas *canvas;
    Scene *scene;
    QStringList daylist;
    QStringList hourlist;
    QList<int> breaklist;

    QList<QList<Cell *>> cols;

    qreal DAY_WIDTH = 140.0;
    qreal HOUR_HEIGHT = 60.0;
    qreal VHEADERWIDTH = 80.0;
    qreal HHEADERHEIGHT = 40.0;
    qreal GRIDLINEWIDTH = 2.0;
    QString GRIDLINECOLOUR = "EDAB9A";
    QString BREAKLINECOLOUR = "404080";
    qreal FONT_CENTRE_SIZE = 12.0;
    qreal FONT_CORNER_SIZE = 8.0;
    QString SELECTIONCOLOUR = "FF0000";
    QBrush OKBRUSH = QColor("#FFDFFF99");

    QJsonObject settings;

    std::function<void (HoverRectItem*, bool)> hover_handler;

private:
    void handle_click(QList<QGraphicsItem *> items, int keymod);
    void handle_context_menu(QList<QGraphicsItem *> items);
    void handle_hover(HoverRectItem*, bool);
    std::function<void (int day, int hour, Tile *tile)> click_handler;
    QGraphicsRectItem *selection_rect = nullptr;
    QList<Cell*> ok_cells;
};

class Tile : public Chip
{
public:
    enum { Type = UserType + 4 };
    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

    Tile(
        TT_Grid *grid,
        QJsonObject data,
        int lesson_id);

    void place(qreal x, qreal y, qreal w, qreal h);

    int lid;
    QString ref;
    int length;
    int divs;
    int div0;
    int ndivs;
    QString middle;
    QString tl;
    QString tr;
    QString bl;
    QString br;

    const qreal TILE_BORDER_WIDTH = 1.0;
    const QString TILE_BORDER_COLOUR = "6060FF";
    const bool TEXT_BOLD = true;
    const int TEXT_ALIGN = 0; // centred
};

#endif // TT_GRID_H

