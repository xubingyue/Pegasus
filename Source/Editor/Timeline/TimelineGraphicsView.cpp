/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus Editor                                    */
/*                                                                                      */
/****************************************************************************************/

//! \file	TimelineGraphicsView.cpp
//! \author	Kevin Boulanger
//! \date	10th June 2013
//! \brief	Graphics view containing the graphics timeline representation

#include "Timeline/TimelineGraphicsView.h"
#include "Timeline/TimelineBackgroundBeatGraphicsItem.h"
#include "Timeline/TimelineBackgroundBeatLineGraphicsItem.h"
#include "Timeline/TimelineBlockGraphicsItem.h"
#include "Timeline/TimelineCursorGraphicsItem.h"
#include "Timeline/TimelineSizes.h"
#include <QWheelEvent>


TimelineGraphicsView::TimelineGraphicsView(QWidget *parent)
:   QGraphicsView(parent),
    mNumLanes(0),
    mHorizontalScale(1.0f),
    mZoom(1.0f)/*,
    timerId(0)*/
{
    // Set the scrollbars to be always visible
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // Create the scene containing the items to render
    QGraphicsScene *scene = new QGraphicsScene(this);
    
    // Set the indexing method of the items.
    // NoIndex is supposedly faster when items move a lot.
    // BspTreeIndex is faster for static scenes.
    // Adapt based on the performances.
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);

    // Set the initial bounds of the scene
    setScene(scene);
    UpdateSceneRect();

    // Set the cache mode
    setCacheMode(CacheBackground);

    // Set the update mode, when updating the content of the view
    //! \todo Change based on the performances
    setViewportUpdateMode(BoundingRectViewportUpdate);

    // Disable anti-aliasing (default value before reading from the settings)
    // but enable text antialiasing all the time
    EnableAntialiasing(false);
    setRenderHint(QPainter::TextAntialiasing);

    // Use the mouse position to focus the zoom
    setTransformationAnchor(AnchorUnderMouse);

    // Enable the selection of blocks using a drag box
    setDragMode(QGraphicsView::RubberBandDrag);

    //! \todo **** Temporary block to create items to render
    for (unsigned int b = 0; b < 64; ++b)
    {
        TimelineBackgroundBeatGraphicsItem * beatItem = new TimelineBackgroundBeatGraphicsItem(b, 4, mHorizontalScale);
        scene->addItem(beatItem);

        if (b > 0)
        {
            TimelineBackgroundBeatLineGraphicsItem * beatLineItem = new TimelineBackgroundBeatLineGraphicsItem(b, 4, mHorizontalScale);
            scene->addItem(beatLineItem);
        }
    }
    TimelineBlockGraphicsItem * item1 = new TimelineBlockGraphicsItem(0, 0.0f, 1.0f, QColor(192, 128, 128), mHorizontalScale);
    TimelineBlockGraphicsItem * item2 = new TimelineBlockGraphicsItem(2, 2.0f, 4.0f, QColor(128, 192, 128), mHorizontalScale);
    TimelineBlockGraphicsItem * item3 = new TimelineBlockGraphicsItem(3, 0.5f, 2.0f, QColor(128, 128, 192), mHorizontalScale);
    scene->addItem(item1);
    scene->addItem(item2);
    scene->addItem(item3);

    // Create the cursor
    mCursorItem = new TimelineCursorGraphicsItem(4, mHorizontalScale);
    scene->addItem(mCursorItem);
}

//----------------------------------------------------------------------------------------

TimelineGraphicsView::~TimelineGraphicsView()
{
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::EnableAntialiasing(bool enable)
{
    setRenderHint(QPainter::Antialiasing, enable);

    // Invalidate the cache of the view, so that the background does not keep
    // ghosts of the previous blocks
    resetCachedContent();

    // Update every items to take antialiasing into account
    foreach (QGraphicsItem *item, scene()->items())
    {
        item->update();
    }
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::AddLane()
{
    mNumLanes++;

    // Invalidate the cache of the view, so that the background does not keep
    // ghosts of the previous blocks
    resetCachedContent();

    // Update the elements that depend on the number of lanes
    // (this invalidates the cache of the block graphics items)
    foreach (QGraphicsItem *item, scene()->items())
    {
        TimelineBackgroundBeatGraphicsItem * backgroundBeatItem = qgraphicsitem_cast<TimelineBackgroundBeatGraphicsItem *>(item);
        if (backgroundBeatItem != nullptr)
        {
            backgroundBeatItem->SetNumLanes(mNumLanes);
            continue;
        }

        TimelineBackgroundBeatLineGraphicsItem * backgroundBeatLineItem = qgraphicsitem_cast<TimelineBackgroundBeatLineGraphicsItem *>(item);
        if (backgroundBeatLineItem != nullptr)
        {
            backgroundBeatLineItem->SetNumLanes(mNumLanes);
            continue;
        }

        TimelineCursorGraphicsItem * cursorItem = qgraphicsitem_cast<TimelineCursorGraphicsItem *>(item);
        if (cursorItem != nullptr)
        {
            cursorItem->SetNumLanes(mNumLanes);
            continue;
        }
    }

    // Update the bounding box of the scene
    UpdateSceneRect();
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::SetHorizontalScale(float scale)
{
    ED_ASSERTSTR((scale >= TIMELINE_GRAPHICS_VIEW_HORIZONTAL_SCALE_MIN) && (scale <= TIMELINE_GRAPHICS_VIEW_HORIZONTAL_SCALE_MAX),
                 "Invalid horizontal scale for the timeline graphics view.");
    mHorizontalScale = scale;

    // Invalidate the cache of the view, so that the background does not keep
    // ghosts of the previous blocks
    resetCachedContent();

    // Update the horizontal scale of all the blocks
    // (this invalidates the cache of the block graphics items)
    foreach (QGraphicsItem *item, scene()->items())
    {
        TimelineBackgroundBeatGraphicsItem * backgroundBeatItem = qgraphicsitem_cast<TimelineBackgroundBeatGraphicsItem *>(item);
        if (backgroundBeatItem != nullptr)
        {
            backgroundBeatItem->SetHorizontalScale(mHorizontalScale);
            continue;
        }

        TimelineBackgroundBeatLineGraphicsItem * backgroundBeatLineItem = qgraphicsitem_cast<TimelineBackgroundBeatLineGraphicsItem *>(item);
        if (backgroundBeatLineItem != nullptr)
        {
            backgroundBeatLineItem->SetHorizontalScale(mHorizontalScale);
            continue;
        }

        TimelineBlockGraphicsItem * blockItem = qgraphicsitem_cast<TimelineBlockGraphicsItem *>(item);
        if (blockItem != nullptr)
        {
            blockItem->SetHorizontalScale(mHorizontalScale);
            continue;
        }

        TimelineCursorGraphicsItem * cursorItem = qgraphicsitem_cast<TimelineCursorGraphicsItem *>(item);
        if (cursorItem != nullptr)
        {
            cursorItem->SetHorizontalScale(mHorizontalScale);
            continue;
        }
    }

    // Update the bounding box of the scene
    UpdateSceneRect();
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::MultiplyHorizontalScale(float scaleFactor)
{
    if (scaleFactor <= 0.0f)
    {
        ED_FAILSTR("Invalid horizontal scale factor (<= 0.0f) for the timeline graphics view.");
        return;
    }

    const float newHorizontalScale = scaleFactor * mHorizontalScale;
    if (   (newHorizontalScale >= TIMELINE_GRAPHICS_VIEW_HORIZONTAL_SCALE_MIN)
        && (newHorizontalScale <= TIMELINE_GRAPHICS_VIEW_HORIZONTAL_SCALE_MAX))
    {
        // Range valid. Apply the scale and redraw the view
        SetHorizontalScale(newHorizontalScale);
    }
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::SetZoom(float zoom)
{
    if (zoom < TIMELINE_GRAPHICS_VIEW_ZOOM_MIN)
    {
        zoom = TIMELINE_GRAPHICS_VIEW_ZOOM_MIN;
        ED_FAILSTR("Invalid zoom factor (< TIMELINE_GRAPHICS_VIEW_ZOOM_MIN) for the timeline graphics view.");
    }
    else if (zoom > TIMELINE_GRAPHICS_VIEW_ZOOM_MAX)
    {
        zoom = TIMELINE_GRAPHICS_VIEW_ZOOM_MAX;
        ED_FAILSTR("Invalid zoom factor (> TIMELINE_GRAPHICS_VIEW_ZOOM_MAX) for the timeline graphics view.");
    }

    mZoom = zoom;

    // Apply the new zoom level to the view
    QTransform newTransform;
    newTransform.scale(mZoom, mZoom);
    setTransform(newTransform);
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::MultiplyZoom(float zoomFactor)
{
    if (zoomFactor <= 0.0f)
    {
        ED_FAILSTR("Invalid zoom multiplier (<= 0.0f) for the timeline graphics view.");
        return;
    }

    const float newZoom = zoomFactor * mZoom;
    if (   (newZoom >= TIMELINE_GRAPHICS_VIEW_ZOOM_MIN)
        && (newZoom <= TIMELINE_GRAPHICS_VIEW_ZOOM_MAX))
    {
        // Range valid. Apply the zoom and redraw the view
        SetZoom(newZoom);
    }
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::mousePressEvent(QMouseEvent * event)
{
    if (event->buttons() & Qt::RightButton)
    {
        // When right-clicking, move the cursor
        UpdateCursorFromMouse(event);
    }
    else
    {
        // Regular mouse handling
        QGraphicsView::mousePressEvent(event);
    }
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::mouseMoveEvent(QMouseEvent * event)
{
    if (event->buttons() & Qt::RightButton)
    {
        // When right-dragging, move the cursor
        UpdateCursorFromMouse(event);
    }
    else
    {
        // Regular mouse handling
        QGraphicsView::mouseMoveEvent(event);
    }
}

//----------------------------------------------------------------------------------------

#ifndef QT_NO_WHEELEVENT
void TimelineGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        // Global zoom
        MultiplyZoom(pow(2.0, event->delta() / 240.0));
    }
    else
    {
        MultiplyHorizontalScale(pow(2.0, event->delta() / 240.0));
    }
}
#endif

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);
    
    //// Shadow
    //QRectF sceneRect = this->sceneRect();
    //QRectF rightShadow(sceneRect.right(), sceneRect.top() + 5, 5, sceneRect.height());
    //QRectF bottomShadow(sceneRect.left() + 5, sceneRect.bottom(), sceneRect.width(), 5);
    //if (rightShadow.intersects(rect) || rightShadow.contains(rect))
    //    painter->fillRect(rightShadow, Qt::darkGray);
    //if (bottomShadow.intersects(rect) || bottomShadow.contains(rect))
    //    painter->fillRect(bottomShadow, Qt::darkGray);

    //// Fill
    //QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
    //gradient.setColorAt(0, Qt::white);
    //gradient.setColorAt(1, Qt::lightGray);
    //painter->fillRect(rect.intersected(sceneRect), gradient);
    //painter->setBrush(Qt::NoBrush);
    //painter->drawRect(sceneRect);

    // Text
    /*QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4,
                    sceneRect.width() - 4, sceneRect.height() - 4);
    QString message(tr("Click and drag the nodes around, and zoom with the mouse "
                       "wheel or the '+' and '-' keys"));

    QFont font = painter->font();
    font.setBold(true);
    font.setPointSize(14);
    painter->setFont(font);
    painter->setPen(Qt::lightGray);
    painter->drawText(textRect.translated(2, 2), message);
    painter->setPen(Qt::black);
    painter->drawText(textRect, message);*/
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::UpdateSceneRect()
{
    scene()->setSceneRect(0.0f,
                          -TIMELINE_BEAT_NUMBER_BLOCK_HEIGHT,
                          /***/64.0f * TIMELINE_BEAT_WIDTH * mHorizontalScale,
                          TIMELINE_BEAT_NUMBER_BLOCK_HEIGHT + static_cast<float>(mNumLanes) * TIMELINE_LANE_HEIGHT);
}

//----------------------------------------------------------------------------------------

void TimelineGraphicsView::UpdateCursorFromMouse(QMouseEvent * event)
{
    ED_ASSERT(event != nullptr);

    // Compute the beat selected by the mouse
    float beat = mapToScene(event->pos()).x() / (mHorizontalScale * TIMELINE_BEAT_WIDTH);
    if (beat < 0.0f)
    {
        beat = 0.0f;
    }
    //! \todo Handle out-of-bounds at the end of the timeline

    // Reposition the cursor
    if (mCursorItem != nullptr)
    {
        mCursorItem->SetBeat(beat);
    }
    else
    {
        ED_FAIL();
    }

    // Update the other parts of the user interface
    emit BeatUpdated(beat);

    //! \todo Handle the Pegasus update
}

//----------------------------------------------------------------------------------------

//void GraphWidget::keyPressEvent(QKeyEvent *event)
//{
//    switch (event->key()) {
//    case Qt::Key_Up:
//        centerNode->moveBy(0, -20);
//        break;
//    case Qt::Key_Down:
//        centerNode->moveBy(0, 20);
//        break;
//    case Qt::Key_Left:
//        centerNode->moveBy(-20, 0);
//        break;
//    case Qt::Key_Right:
//        centerNode->moveBy(20, 0);
//        break;
//    case Qt::Key_Plus:
//        zoomIn();
//        break;
//    case Qt::Key_Minus:
//        zoomOut();
//        break;
//    case Qt::Key_Space:
//    case Qt::Key_Enter:
//        shuffle();
//        break;
//    default:
//        QGraphicsView::keyPressEvent(event);
//    }
//}

//----------------------------------------------------------------------------------------

//void TimelineGraphicsView::timerEvent(QTimerEvent *event)
//{
//    Q_UNUSED(event);
//
//    //QList<Node *> nodes;
//    //foreach (QGraphicsItem *item, scene()->items()) {
//    //    if (Node *node = qgraphicsitem_cast<Node *>(item))
//    //        nodes << node;
//    //}
//
//    //foreach (Node *node, nodes)
//    //    node->calculateForces();
//
//    bool itemsMoved = false;
//    //foreach (Node *node, nodes) {
//    //    if (node->advance())
//    //        itemsMoved = true;
//    //}
//
//    if (!itemsMoved) {
//        killTimer(timerId);
//        timerId = 0;
//    }
//}

//----------------------------------------------------------------------------------------

//void TimelineGraphicsView::ScaleView(qreal scaleFactor)
//{
//    qreal factor = transform().scale(scaleFactor, /*scaleFactor*/1.0f).mapRect(QRectF(0, 0, 1, 1)).width();
//    if (   (factor < TIMELINE_GRAPHICS_VIEW_ZOOM_MIN)
//        || (factor > TIMELINE_GRAPHICS_VIEW_ZOOM_MAX))
//        return;
//
//    scale(scaleFactor, /*scaleFactor*/1.0f);
//}

//----------------------------------------------------------------------------------------

//void TimelineGraphicsView::shuffle()
//{
//    //foreach (QGraphicsItem *item, scene()->items()) {
//    //    if (qgraphicsitem_cast<Node *>(item))
//    //        item->setPos(-150 + qrand() % 300, -150 + qrand() % 300);
//    //}
//}

//----------------------------------------------------------------------------------------

//void TimelineGraphicsView::zoomIn()
//{
//    ScaleView(qreal(1.2));
//}
//
////----------------------------------------------------------------------------------------
//
//void TimelineGraphicsView::zoomOut()
//{
//    ScaleView(1 / qreal(1.2));
//}
