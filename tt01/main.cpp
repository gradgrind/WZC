#include "canvas.h"
#include "tt_grid.h"
#include <QBoxLayout>

int main(int argc, char *argv[])
{
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
        {"HU A", "HU B", "FS 1", "FS 2", "FS 3", "FS 4", "FS 5", "FS 6", "FS 7"});

    view.setFixedSize(A4.toSize());

    mainwindow.show();

    return a.exec();
}
