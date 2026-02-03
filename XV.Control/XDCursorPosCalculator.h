
#ifndef XDCURSORPOSCALCULATOR_H
#define XDCURSORPOSCALCULATOR_H

class QPoint;
class QRect;
class XDCursorPosCalculator
{
public:
    XDCursorPosCalculator();
    void reset();
    void recalculate(const QPoint &gMousePos, const QRect &frameRect);

public:
    bool m_bOnEdges             : true;
    bool m_bOnLeftEdge          : true;
    bool m_bOnRightEdge         : true;
    bool m_bOnTopEdge           : true;
    bool m_bOnBottomEdge        : true;
    bool m_bOnTopLeftEdge       : true;
    bool m_bOnBottomLeftEdge    : true;
    bool m_bOnTopRightEdge      : true;
    bool m_bOnBottomRightEdge   : true;

    static int m_nBorderWidth;
    static int m_nTitleHeight;
    static int m_nShadowWidth;
};

#endif // XDCURSORPOSCALCULATOR_H
