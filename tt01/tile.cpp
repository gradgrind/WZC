#include "tile.h"

Tile::Tile(QGraphicsScene *scene, QJsonObject settings, QJsonObject data) : Chip()
{
    scene->addItem(this);
    tag = data.value("TAG").toString();
    length = data.value("LENGTH").toInt(1);
    divs = data.value("DIVS").toInt(1);
    div0 = data.value("DIV0").toInt(0);
    ndivs = data.value("NDIVS").toInt(1);
    middle = data.value("TEXT").toString();
    tl = data.value("TL").toString();
    tr = data.value("TR").toString();
    bl = data.value("BL").toString();
    br = data.value("BR").toString();

    QString bg = data.value("BACKGROUND").toString("FFFFFF");
    set_background(bg);
//    QJsonObject settings = grid->settings;
    set_border(settings.value(
                           "TILE_BORDER_WIDTH").toDouble(TILE_BORDER_WIDTH));

    config_text(
        settings.value("TEXT_SIZE").toDouble(),
        settings.value("TEXT_BOLD").toBool(TEXT_BOLD),
        settings.value("TEXT_ALIGN").toInt(TEXT_ALIGN),
        settings.value("TEXT_COLOUR").toString());
    set_subtext_size(settings.value("SUBTEXT_SIZE").toDouble());
}

void Tile::place(qreal x, qreal y, qreal w, qreal h)
{
    /* The QGraphicsItem method "setPos" takes "float" coordinates,
     * either as setPos(x, y) or as setPos(QPointF). It sets the position
     * of the item in parent coordinates. For items with no parent, scene
     * coordinates are used.
     * The position of the item describes its origin (local coordinate
     * (0, 0)) in parent coordinates.
     * The size of the tile is set by means of the width and height values.
     * If no change of size is desired, just call the "setPos" method.
     */
    setRect(0, 0, w, h);
    setPos(x, y);
    // Handle the text field placement and potential shrinking
    set_text(middle);
    set_toptext(tl, tr);
    set_bottomtext(bl, br);
}
