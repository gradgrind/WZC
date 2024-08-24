#include "canvas.h"
#include "tt_grid.h"
#include <QBoxLayout>
//#include <QStyleFactory>
#include "readxml.h"

int main(int argc, char *argv[])
{
    //qDebug() << QStyleFactory::keys();
    //QApplication::setStyle("fusion");
    QApplication a(argc, argv);
    QWidget mainwindow;
    QWidget right;
    right.setFixedWidth(200);

    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, QColor(255, 255, 200));
    right.setAutoFillBackground(true);
    right.setPalette(pal);

    QHBoxLayout hb(&mainwindow);

    QGraphicsView view;
    hb.addWidget(&view);
    hb.addWidget(&right);

//    Canvas canv(&view);
    TT_Grid grid(&view,
        {"Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag"},
        {"HU A", "HU B", "FS 1", "FS 2", "FS 3", "FS 4", "FS 5", "FS 6", "FS 7"},
        {2, 4, 6});

    qDebug() << "@ (3, 1): " << grid.cols[4][2]->rect();
    qDebug() << "  ... " << grid.cols[4][2]->pos();

    Tile *t = new Tile(&grid, QJsonObject
        {
            {"TEXT", "Centre"},
            {"TR", "rTop"},
            {"LENGTH", 2},
            {"DIV0", 2},
            {"DIVS", 1},
            {"NDIVS", 3},
        }
    );
    grid.place_tile(t, 3, 1);
    //t->place(200, 300, 150, 150);

    t = new Tile(&grid,
        QJsonObject {
            {"TEXT", "Second"},
            {"BL", "lBottom"},
        }
    );
    grid.place_tile(t, 4, 6);
    //t->place(500, 250, 150, 150);

    view.setFixedSize(A4.toSize());

    //readFet_test();
    readxml_test();

    mainwindow.show();

    return a.exec();
}
