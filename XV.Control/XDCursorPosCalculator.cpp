
#include <QPoint>
#include <QRect>
#include "XDCursorPosCalculator.h"

int XDCursorPosCalculator::m_nBorderWidth = 5;

int XDCursorPosCalculator::m_nTitleHeight = 30;
int XDCursorPosCalculator::m_nShadowWidth = 0;

XDCursorPosCalculator::XDCursorPosCalculator()
{
    reset();
}

void XDCursorPosCalculator::reset()
{
    m_bOnEdges = false;
    m_bOnLeftEdge = false;
    m_bOnRightEdge = false;
    m_bOnTopEdge = false;
    m_bOnBottomEdge = false;
    m_bOnTopLeftEdge = false;
    m_bOnBottomLeftEdge = false;
    m_bOnTopRightEdge  = false;
    m_bOnBottomRightEdge = false;
}

void XDCursorPosCalculator::recalculate(const QPoint &gMousePos, const QRect &frameRect)
{
    int globalMouseX = gMousePos.x();
    int globalMouseY = gMousePos.y();

    int frameX = frameRect.x();
    int frameY = frameRect.y();

    int frameWidth = frameRect.width();
    int frameHeight = frameRect.height();

    // 边框边缘触发光标变化
    //
    m_bOnLeftEdge = (globalMouseX >= frameX - m_nBorderWidth &&
                     globalMouseX <= frameX + m_nBorderWidth);
    m_bOnRightEdge = (globalMouseX >= frameX + frameWidth - m_nBorderWidth &&
                      globalMouseX <= frameX + frameWidth + m_nBorderWidth);
    m_bOnTopEdge = (globalMouseY >= frameY - m_nBorderWidth &&
                     globalMouseY <= frameY + 5);
    m_bOnBottomEdge = (globalMouseY >= frameY + frameHeight - m_nBorderWidth &&
                    globalMouseY <= frameY + frameHeight + m_nBorderWidth);

    m_bOnTopLeftEdge = m_bOnTopEdge && m_bOnLeftEdge;
    m_bOnBottomLeftEdge = m_bOnBottomEdge && m_bOnLeftEdge;
    m_bOnTopRightEdge = m_bOnTopEdge && m_bOnRightEdge;
    m_bOnBottomRightEdge = m_bOnBottomEdge && m_bOnRightEdge;

    m_bOnEdges = m_bOnLeftEdge || m_bOnRightEdge || m_bOnTopEdge || m_bOnBottomEdge;
}
