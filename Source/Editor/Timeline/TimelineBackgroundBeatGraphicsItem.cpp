/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus Editor                                    */
/*                                                                                      */
/****************************************************************************************/

//! \file	TimelineBackgroundBeatGraphicsItem.cpp
//! \author	Karolyn Boulanger
//! \date	21st July 2013
//! \brief	Graphics item representing one beat of the background in the timeline

#include "Timeline/TimelineBackgroundBeatGraphicsItem.h"
#include "Timeline/TimelineSizes.h"
#include <QAbstractScrollArea>
#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOption>

#define  TIMELINE_BEAT_BG_Z_VALUE 1.0f

//! Multiplier for the horizontal scale to increase the width of the drawn blocks a bit,
//! to avoid white space between them when zooming in/out.
static const float HORIZONTAL_SCALE_MARGIN = 1.0f;

TimelineBackgroundBeatGraphicsItem::TimelineBackgroundBeatGraphicsItem(unsigned int beat,
                                                                       unsigned int numLanes,
                                                                       float horizontalScale)
:   QGraphicsItem(),
    mBeat(static_cast<float>(beat))
{
    if (numLanes == 0)
    {
        ED_FAILSTR("Invalid number of lanes (0) for the timeline graphics item. It should be > 0.");
        mNumLanes = 1;
    }
    else
    {
        mNumLanes = numLanes;
    }

    if (horizontalScale <= 0.0f)
    {
        ED_FAILSTR("Invalid horizontal scale (%f) for the timeline graphics item. It should be > 0.0f.", horizontalScale);
        mHorizontalScale = 1.0f;
    }
    else
    {
        mHorizontalScale = horizontalScale;
    }

    // Set the initial scaled position
    SetPositionFromBeat();

    // Caching performed at paint device level, best quality and lower memory usage
    setCacheMode(DeviceCoordinateCache);

    // Set the depth of the block to be the most in the background
    setZValue(TIMELINE_BEAT_BG_Z_VALUE);
}

//----------------------------------------------------------------------------------------

TimelineBackgroundBeatGraphicsItem::~TimelineBackgroundBeatGraphicsItem()
{
}

//----------------------------------------------------------------------------------------

void TimelineBackgroundBeatGraphicsItem::SetNumLanes(unsigned int numLanes)
{
    if (numLanes == 0)
    {
        ED_FAILSTR("Invalid number of lanes (0) for the timeline graphics item. It should be > 0.");
        mNumLanes = 1;
    }
    else
    {
        mNumLanes = numLanes;
    }

    // Invalidate the cache of the graphics item.
    // If not done manually here, setCacheMode(NoCache) would be necessary
    // in the constructor, resulting in poor performances
    update(boundingRect());
}

//----------------------------------------------------------------------------------------

void TimelineBackgroundBeatGraphicsItem::SetHorizontalScale(float scale)
{
    // No assertion for invalid scale (performance issue).
    // The caller performs the assertion test.
    mHorizontalScale = scale;

    //! Update the scaled position
    SetPositionFromBeat();

    // Invalidate the cache of the graphics item.
    // If not done manually here, setCacheMode(NoCache) would be necessary
    // in the constructor, resulting in poor performances
    update(boundingRect());
}

//----------------------------------------------------------------------------------------

QRectF TimelineBackgroundBeatGraphicsItem::boundingRect() const
{
    return QRectF(0.0f,
                  0.0f,
                  HORIZONTAL_SCALE_MARGIN * mHorizontalScale * TIMELINE_BEAT_WIDTH,
                  mNumLanes * TIMELINE_LANE_HEIGHT);
}

//----------------------------------------------------------------------------------------

void TimelineBackgroundBeatGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget)
{
    Q_UNUSED(widget);

    // Draw the even lanes (lighter)
    painter->setPen(Qt::NoPen);
    const QColor fillColorEven(224, 224, 224);
    painter->setBrush(fillColorEven);
    unsigned int lane;
    for (lane = 0; lane < mNumLanes; lane += 2)
    {
        painter->drawRect(0.0f,
                          static_cast<float>(lane) * TIMELINE_LANE_HEIGHT,
                          HORIZONTAL_SCALE_MARGIN * mHorizontalScale * TIMELINE_BEAT_WIDTH,
                          TIMELINE_LANE_HEIGHT);
    }

    // Draw the odd lanes (darker)
    const QColor fillColorOdd = fillColorEven.darker(110);
    painter->setBrush(fillColorOdd);
    for (lane = 1; lane < mNumLanes; lane += 2)
    {
        painter->drawRect(0.0f,
                          static_cast<float>(lane) * TIMELINE_LANE_HEIGHT,
                          HORIZONTAL_SCALE_MARGIN * mHorizontalScale * TIMELINE_BEAT_WIDTH,
                          TIMELINE_LANE_HEIGHT);
    }
}

//----------------------------------------------------------------------------------------

void TimelineBackgroundBeatGraphicsItem::SetPositionFromBeat()
{
    float position = mBeat * (mHorizontalScale * TIMELINE_BEAT_WIDTH);
    setPos(position, 0.0f);
}
