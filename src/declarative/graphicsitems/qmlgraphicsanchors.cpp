/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmlgraphicsanchors_p_p.h"

#include "qmlgraphicsitem.h"
#include "qmlgraphicsitem_p.h"

#include <qmlinfo.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

//TODO: should we cache relationships, so we don't have to check each time (parent-child or sibling)?
//TODO: support non-parent, non-sibling (need to find lowest common ancestor)

//### const item?
//local position
static qreal position(QmlGraphicsItem *item, QmlGraphicsAnchorLine::AnchorLine anchorLine)
{
    qreal ret = 0.0;
    switch(anchorLine) {
    case QmlGraphicsAnchorLine::Left:
        ret = item->x();
        break;
    case QmlGraphicsAnchorLine::Right:
        ret = item->x() + item->width();
        break;
    case QmlGraphicsAnchorLine::Top:
        ret = item->y();
        break;
    case QmlGraphicsAnchorLine::Bottom:
        ret = item->y() + item->height();
        break;
    case QmlGraphicsAnchorLine::HCenter:
        ret = item->x() + item->width()/2;
        break;
    case QmlGraphicsAnchorLine::VCenter:
        ret = item->y() + item->height()/2;
        break;
    case QmlGraphicsAnchorLine::Baseline:
        ret = item->y() + item->baselineOffset();
        break;
    default:
        break;
    }

    return ret;
}

//position when origin is 0,0
static qreal adjustedPosition(QmlGraphicsItem *item, QmlGraphicsAnchorLine::AnchorLine anchorLine)
{
    int ret = 0;
    switch(anchorLine) {
    case QmlGraphicsAnchorLine::Left:
        ret = 0;
        break;
    case QmlGraphicsAnchorLine::Right:
        ret = item->width();
        break;
    case QmlGraphicsAnchorLine::Top:
        ret = 0;
        break;
    case QmlGraphicsAnchorLine::Bottom:
        ret = item->height();
        break;
    case QmlGraphicsAnchorLine::HCenter:
        ret = item->width()/2;
        break;
    case QmlGraphicsAnchorLine::VCenter:
        ret = item->height()/2;
        break;
    case QmlGraphicsAnchorLine::Baseline:
        ret = item->baselineOffset();
        break;
    default:
        break;
    }

    return ret;
}

/*!
    \internal
    \class QmlGraphicsAnchors
    \ingroup group_layouts
    \brief The QmlGraphicsAnchors class provides a way to lay out items relative to other items.

    \warning Currently, only anchoring to siblings or parent is supported.
*/

QmlGraphicsAnchors::QmlGraphicsAnchors(QObject *parent)
  : QObject(*new QmlGraphicsAnchorsPrivate(0), parent)
{
    qFatal("QmlGraphicsAnchors::QmlGraphicsAnchors(QObject*) called");
}

QmlGraphicsAnchors::QmlGraphicsAnchors(QmlGraphicsItem *item, QObject *parent)
  : QObject(*new QmlGraphicsAnchorsPrivate(item), parent)
{
}

QmlGraphicsAnchors::~QmlGraphicsAnchors()
{
    Q_D(QmlGraphicsAnchors);
    d->remDepend(d->fill);
    d->remDepend(d->centerIn);
    d->remDepend(d->left.item);
    d->remDepend(d->right.item);
    d->remDepend(d->top.item);
    d->remDepend(d->bottom.item);
    d->remDepend(d->vCenter.item);
    d->remDepend(d->hCenter.item);
    d->remDepend(d->baseline.item);
}

void QmlGraphicsAnchorsPrivate::fillChanged()
{
    if (!fill || !isItemComplete())
        return;

    if (updatingFill < 2) {
        ++updatingFill;

        if (fill == item->parentItem()) {                         //child-parent
            setItemPos(QPointF(leftMargin, topMargin));
        } else if (fill->parentItem() == item->parentItem()) {   //siblings
            setItemPos(QPointF(fill->x()+leftMargin, fill->y()+topMargin));
        }
        setItemWidth(fill->width()-leftMargin-rightMargin);
        setItemHeight(fill->height()-topMargin-bottomMargin);

        --updatingFill;
    } else {
        // ### Make this certain :)
        qmlInfo(item) << QmlGraphicsAnchors::tr("Possible anchor loop detected on fill.");
    }

}

void QmlGraphicsAnchorsPrivate::centerInChanged()
{
    if (!centerIn || fill || !isItemComplete())
        return;

    if (updatingCenterIn < 2) {
        ++updatingCenterIn;

        if (centerIn == item->parentItem()) {
            QPointF p((item->parentItem()->width() - item->width()) / 2. + hCenterOffset,
                      (item->parentItem()->height() - item->height()) / 2. + vCenterOffset);
            setItemPos(p);

        } else if (centerIn->parentItem() == item->parentItem()) {

            QPointF p(centerIn->x() + (centerIn->width() - item->width()) / 2. + hCenterOffset,
                      centerIn->y() + (centerIn->height() - item->height()) / 2. + vCenterOffset);
            setItemPos(p);
        }

        --updatingCenterIn;
    } else {
        // ### Make this certain :)
        qmlInfo(item) << QmlGraphicsAnchors::tr("Possible anchor loop detected on centerIn.");
    }
}

void QmlGraphicsAnchorsPrivate::clearItem(QmlGraphicsItem *item)
{
    if (!item)
        return;
    if (fill == item)
        fill = 0;
    if (centerIn == item)
        centerIn = 0;
    if (left.item == item) {
        left.item = 0;
        usedAnchors &= ~QmlGraphicsAnchors::HasLeftAnchor;
    }
    if (right.item == item) {
        right.item = 0;
        usedAnchors &= ~QmlGraphicsAnchors::HasRightAnchor;
    }
    if (top.item == item) {
        top.item = 0;
        usedAnchors &= ~QmlGraphicsAnchors::HasTopAnchor;
    }
    if (bottom.item == item) {
        bottom.item = 0;
        usedAnchors &= ~QmlGraphicsAnchors::HasBottomAnchor;
    }
    if (vCenter.item == item) {
        vCenter.item = 0;
        usedAnchors &= ~QmlGraphicsAnchors::HasVCenterAnchor;
    }
    if (hCenter.item == item) {
        hCenter.item = 0;
        usedAnchors &= ~QmlGraphicsAnchors::HasHCenterAnchor;
    }
    if (baseline.item == item) {
        baseline.item = 0;
        usedAnchors &= ~QmlGraphicsAnchors::HasBaselineAnchor;
    }
}

void QmlGraphicsAnchorsPrivate::addDepend(QmlGraphicsItem *item)
{
    if (!item)
        return;
    QmlGraphicsItemPrivate *p =
        static_cast<QmlGraphicsItemPrivate *>(QGraphicsItemPrivate::get(item));
    p->addItemChangeListener(this, QmlGraphicsItemPrivate::Geometry);
}

void QmlGraphicsAnchorsPrivate::remDepend(QmlGraphicsItem *item)
{
    if (!item)
        return;
    QmlGraphicsItemPrivate *p =
        static_cast<QmlGraphicsItemPrivate *>(QGraphicsItemPrivate::get(item));
    p->removeItemChangeListener(this, QmlGraphicsItemPrivate::Geometry);
}

bool QmlGraphicsAnchorsPrivate::isItemComplete() const
{
    return componentComplete;
}

void QmlGraphicsAnchors::classBegin()
{
    Q_D(QmlGraphicsAnchors);
    d->componentComplete = false;
}

void QmlGraphicsAnchors::componentComplete()
{
    Q_D(QmlGraphicsAnchors);
    d->componentComplete = true;
}

void QmlGraphicsAnchorsPrivate::setItemHeight(qreal v)
{
    updatingMe = true;
    item->setHeight(v);
    updatingMe = false;
}

void QmlGraphicsAnchorsPrivate::setItemWidth(qreal v)
{
    updatingMe = true;
    item->setWidth(v);
    updatingMe = false;
}

void QmlGraphicsAnchorsPrivate::setItemX(qreal v)
{
    updatingMe = true;
    item->setX(v);
    updatingMe = false;
}

void QmlGraphicsAnchorsPrivate::setItemY(qreal v)
{
    updatingMe = true;
    item->setY(v);
    updatingMe = false;
}

void QmlGraphicsAnchorsPrivate::setItemPos(const QPointF &v)
{
    updatingMe = true;
    item->setPos(v);
    updatingMe = false;
}

void QmlGraphicsAnchorsPrivate::updateMe()
{
    if (updatingMe) {
        updatingMe = false;
        return;
    }

    fillChanged();
    centerInChanged();
    updateHorizontalAnchors();
    updateVerticalAnchors();
}

void QmlGraphicsAnchorsPrivate::updateOnComplete()
{
    fillChanged();
    centerInChanged();
    updateHorizontalAnchors();
    updateVerticalAnchors();
}

void QmlGraphicsAnchorsPrivate::itemGeometryChanged(QmlGraphicsItem *, const QRectF &newG, const QRectF &oldG)
{
    fillChanged();
    centerInChanged();

    if (newG.x() != oldG.x() || newG.width() != oldG.width())
        updateHorizontalAnchors();
    if (newG.y() != oldG.y() || newG.height() != oldG.height())
        updateVerticalAnchors();
}

QmlGraphicsItem *QmlGraphicsAnchors::fill() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->fill;
}

void QmlGraphicsAnchors::setFill(QmlGraphicsItem *f)
{
    Q_D(QmlGraphicsAnchors);
    if (d->fill == f)
        return;

    if (!f) {
        d->remDepend(d->fill);
        d->fill = f;
        emit fillChanged();
        return;
    }
    if (f != d->item->parentItem() && f->parentItem() != d->item->parentItem()){
        qmlInfo(d->item) << tr("Cannot anchor to an item that isn't a parent or sibling.");
        return;
    }
    d->remDepend(d->fill);
    d->fill = f;
    d->addDepend(d->fill);
    emit fillChanged();
    d->fillChanged();
}

void QmlGraphicsAnchors::resetFill()
{
    setFill(0);
}

QmlGraphicsItem *QmlGraphicsAnchors::centerIn() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->centerIn;
}

void QmlGraphicsAnchors::setCenterIn(QmlGraphicsItem* c)
{
    Q_D(QmlGraphicsAnchors);
    if (d->centerIn == c)
        return;

    if (!c) {
        d->remDepend(d->centerIn);
        d->centerIn = c;
        emit centerInChanged();
        return;
    }
    if (c != d->item->parentItem() && c->parentItem() != d->item->parentItem()){
        qmlInfo(d->item) << tr("Cannot anchor to an item that isn't a parent or sibling.");
        return;
    }

    d->remDepend(d->centerIn);
    d->centerIn = c;
    d->addDepend(d->centerIn);
    emit centerInChanged();
    d->centerInChanged();
}

void QmlGraphicsAnchors::resetCenterIn()
{
    setCenterIn(0);
}

bool QmlGraphicsAnchorsPrivate::calcStretch(const QmlGraphicsAnchorLine &edge1,
                                    const QmlGraphicsAnchorLine &edge2,
                                    int offset1,
                                    int offset2,
                                    QmlGraphicsAnchorLine::AnchorLine line,
                                    int &stretch)
{
    bool edge1IsParent = (edge1.item == item->parentItem());
    bool edge2IsParent = (edge2.item == item->parentItem());
    bool edge1IsSibling = (edge1.item->parentItem() == item->parentItem());
    bool edge2IsSibling = (edge2.item->parentItem() == item->parentItem());

    bool invalid = false;
    if ((edge2IsParent && edge1IsParent) || (edge2IsSibling && edge1IsSibling)) {
        stretch = ((int)position(edge2.item, edge2.anchorLine) + offset2)
                    - ((int)position(edge1.item, edge1.anchorLine) + offset1);
    } else if (edge2IsParent && edge1IsSibling) {
        stretch = ((int)position(edge2.item, edge2.anchorLine) + offset2)
                    - ((int)position(item->parentItem(), line)
                    + (int)position(edge1.item, edge1.anchorLine) + offset1);
    } else if (edge2IsSibling && edge1IsParent) {
        stretch = ((int)position(item->parentItem(), line) + (int)position(edge2.item, edge2.anchorLine) + offset2)
                    - ((int)position(edge1.item, edge1.anchorLine) + offset1);
    } else
        invalid = true;

    return invalid;
}

void QmlGraphicsAnchorsPrivate::updateVerticalAnchors()
{
    if (fill || centerIn || !isItemComplete())
        return;

    if (updatingVerticalAnchor < 2) {
        ++updatingVerticalAnchor;
        if (usedAnchors & QmlGraphicsAnchors::HasTopAnchor) {
            //Handle stretching
            bool invalid = true;
            int height = 0;
            if (usedAnchors & QmlGraphicsAnchors::HasBottomAnchor) {
                invalid = calcStretch(top, bottom, topMargin, -bottomMargin, QmlGraphicsAnchorLine::Top, height);
            } else if (usedAnchors & QmlGraphicsAnchors::HasVCenterAnchor) {
                invalid = calcStretch(top, vCenter, topMargin, vCenterOffset, QmlGraphicsAnchorLine::Top, height);
                height *= 2;
            }
            if (!invalid)
                setItemHeight(height);

            //Handle top
            if (top.item == item->parentItem()) {
                setItemY(adjustedPosition(top.item, top.anchorLine) + topMargin);
            } else if (top.item->parentItem() == item->parentItem()) {
                setItemY(position(top.item, top.anchorLine) + topMargin);
            }
        } else if (usedAnchors & QmlGraphicsAnchors::HasBottomAnchor) {
            //Handle stretching (top + bottom case is handled above)
            if (usedAnchors & QmlGraphicsAnchors::HasVCenterAnchor) {
                int height = 0;
                bool invalid = calcStretch(vCenter, bottom, vCenterOffset, -bottomMargin,
                                              QmlGraphicsAnchorLine::Top, height);
                if (!invalid)
                    setItemHeight(height*2);
            }

            //Handle bottom
            if (bottom.item == item->parentItem()) {
                setItemY(adjustedPosition(bottom.item, bottom.anchorLine) - item->height() - bottomMargin);
            } else if (bottom.item->parentItem() == item->parentItem()) {
                setItemY(position(bottom.item, bottom.anchorLine) - item->height() - bottomMargin);
            }
        } else if (usedAnchors & QmlGraphicsAnchors::HasVCenterAnchor) {
            //(stetching handled above)

            //Handle vCenter
            if (vCenter.item == item->parentItem()) {
                setItemY(adjustedPosition(vCenter.item, vCenter.anchorLine)
                              - item->height()/2 + vCenterOffset);
            } else if (vCenter.item->parentItem() == item->parentItem()) {
                setItemY(position(vCenter.item, vCenter.anchorLine) - item->height()/2 + vCenterOffset);
            }
        } else if (usedAnchors & QmlGraphicsAnchors::HasBaselineAnchor) {
            //Handle baseline
            if (baseline.item == item->parentItem()) {
                setItemY(adjustedPosition(baseline.item, baseline.anchorLine)
                        - item->baselineOffset() + baselineOffset);
            } else if (baseline.item->parentItem() == item->parentItem()) {
                setItemY(position(baseline.item, baseline.anchorLine)
                        - item->baselineOffset() + baselineOffset);
            }
        }
        --updatingVerticalAnchor;
    } else {
        // ### Make this certain :)
        qmlInfo(item) << QmlGraphicsAnchors::tr("Possible anchor loop detected on vertical anchor.");
    }
}

void QmlGraphicsAnchorsPrivate::updateHorizontalAnchors()
{
    if (fill || centerIn || !isItemComplete())
        return;

    if (updatingHorizontalAnchor < 2) {
        ++updatingHorizontalAnchor;

        if (usedAnchors & QmlGraphicsAnchors::HasLeftAnchor) {
            //Handle stretching
            bool invalid = true;
            int width = 0;
            if (usedAnchors & QmlGraphicsAnchors::HasRightAnchor) {
                invalid = calcStretch(left, right, leftMargin, -rightMargin, QmlGraphicsAnchorLine::Left, width);
            } else if (usedAnchors & QmlGraphicsAnchors::HasHCenterAnchor) {
                invalid = calcStretch(left, hCenter, leftMargin, hCenterOffset, QmlGraphicsAnchorLine::Left, width);
                width *= 2;
            }
            if (!invalid)
                setItemWidth(width);

            //Handle left
            if (left.item == item->parentItem()) {
                setItemX(adjustedPosition(left.item, left.anchorLine) + leftMargin);
            } else if (left.item->parentItem() == item->parentItem()) {
                setItemX(position(left.item, left.anchorLine) + leftMargin);
            }
        } else if (usedAnchors & QmlGraphicsAnchors::HasRightAnchor) {
            //Handle stretching (left + right case is handled in updateLeftAnchor)
            if (usedAnchors & QmlGraphicsAnchors::HasHCenterAnchor) {
                int width = 0;
                bool invalid = calcStretch(hCenter, right, hCenterOffset, -rightMargin,
                                              QmlGraphicsAnchorLine::Left, width);
                if (!invalid)
                    setItemWidth(width*2);
            }

            //Handle right
            if (right.item == item->parentItem()) {
                setItemX(adjustedPosition(right.item, right.anchorLine) - item->width() - rightMargin);
            } else if (right.item->parentItem() == item->parentItem()) {
                setItemX(position(right.item, right.anchorLine) - item->width() - rightMargin);
            }
        } else if (usedAnchors & QmlGraphicsAnchors::HasHCenterAnchor) {
            //Handle hCenter
            if (hCenter.item == item->parentItem()) {
                setItemX(adjustedPosition(hCenter.item, hCenter.anchorLine) - item->width()/2 + hCenterOffset);
            } else if (hCenter.item->parentItem() == item->parentItem()) {
                setItemX(position(hCenter.item, hCenter.anchorLine) - item->width()/2 + hCenterOffset);
            }
        }

        --updatingHorizontalAnchor;
    } else {
        // ### Make this certain :)
        qmlInfo(item) << QmlGraphicsAnchors::tr("Possible anchor loop detected on horizontal anchor.");
    }
}

QmlGraphicsAnchorLine QmlGraphicsAnchors::top() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->top;
}

void QmlGraphicsAnchors::setTop(const QmlGraphicsAnchorLine &edge)
{
    Q_D(QmlGraphicsAnchors);
    if (!d->checkVAnchorValid(edge) || d->top == edge)
        return;

    d->usedAnchors |= HasTopAnchor;

    if (!d->checkVValid()) {
        d->usedAnchors &= ~HasTopAnchor;
        return;
    }

    d->remDepend(d->top.item);
    d->top = edge;
    d->addDepend(d->top.item);
    emit topChanged();
    d->updateVerticalAnchors();
}

void QmlGraphicsAnchors::resetTop()
{
    Q_D(QmlGraphicsAnchors);
    d->usedAnchors &= ~HasTopAnchor;
    d->remDepend(d->top.item);
    d->top = QmlGraphicsAnchorLine();
    emit topChanged();
    d->updateVerticalAnchors();
}

QmlGraphicsAnchorLine QmlGraphicsAnchors::bottom() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->bottom;
}

void QmlGraphicsAnchors::setBottom(const QmlGraphicsAnchorLine &edge)
{
    Q_D(QmlGraphicsAnchors);
    if (!d->checkVAnchorValid(edge) || d->bottom == edge)
        return;

    d->usedAnchors |= HasBottomAnchor;

    if (!d->checkVValid()) {
        d->usedAnchors &= ~HasBottomAnchor;
        return;
    }

    d->remDepend(d->bottom.item);
    d->bottom = edge;
    d->addDepend(d->bottom.item);
    emit bottomChanged();
    d->updateVerticalAnchors();
}

void QmlGraphicsAnchors::resetBottom()
{
    Q_D(QmlGraphicsAnchors);
    d->usedAnchors &= ~HasBottomAnchor;
    d->remDepend(d->bottom.item);
    d->bottom = QmlGraphicsAnchorLine();
    emit bottomChanged();
    d->updateVerticalAnchors();
}

QmlGraphicsAnchorLine QmlGraphicsAnchors::verticalCenter() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->vCenter;
}

void QmlGraphicsAnchors::setVerticalCenter(const QmlGraphicsAnchorLine &edge)
{
    Q_D(QmlGraphicsAnchors);
    if (!d->checkVAnchorValid(edge) || d->vCenter == edge)
        return;

    d->usedAnchors |= HasVCenterAnchor;

    if (!d->checkVValid()) {
        d->usedAnchors &= ~HasVCenterAnchor;
        return;
    }

    d->remDepend(d->vCenter.item);
    d->vCenter = edge;
    d->addDepend(d->vCenter.item);
    emit verticalCenterChanged();
    d->updateVerticalAnchors();
}

void QmlGraphicsAnchors::resetVerticalCenter()
{
    Q_D(QmlGraphicsAnchors);
    d->usedAnchors &= ~HasVCenterAnchor;
    d->remDepend(d->vCenter.item);
    d->vCenter = QmlGraphicsAnchorLine();
    emit verticalCenterChanged();
    d->updateVerticalAnchors();
}

QmlGraphicsAnchorLine QmlGraphicsAnchors::baseline() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->baseline;
}

void QmlGraphicsAnchors::setBaseline(const QmlGraphicsAnchorLine &edge)
{
    Q_D(QmlGraphicsAnchors);
    if (!d->checkVAnchorValid(edge) || d->baseline == edge)
        return;

    d->usedAnchors |= HasBaselineAnchor;

    if (!d->checkVValid()) {
        d->usedAnchors &= ~HasBaselineAnchor;
        return;
    }

    d->remDepend(d->baseline.item);
    d->baseline = edge;
    d->addDepend(d->baseline.item);
    emit baselineChanged();
    d->updateVerticalAnchors();
}

void QmlGraphicsAnchors::resetBaseline()
{
    Q_D(QmlGraphicsAnchors);
    d->usedAnchors &= ~HasBaselineAnchor;
    d->remDepend(d->baseline.item);
    d->baseline = QmlGraphicsAnchorLine();
    emit baselineChanged();
    d->updateVerticalAnchors();
}

QmlGraphicsAnchorLine QmlGraphicsAnchors::left() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->left;
}

void QmlGraphicsAnchors::setLeft(const QmlGraphicsAnchorLine &edge)
{
    Q_D(QmlGraphicsAnchors);
    if (!d->checkHAnchorValid(edge) || d->left == edge)
        return;

    d->usedAnchors |= HasLeftAnchor;

    if (!d->checkHValid()) {
        d->usedAnchors &= ~HasLeftAnchor;
        return;
    }

    d->remDepend(d->left.item);
    d->left = edge;
    d->addDepend(d->left.item);
    emit leftChanged();
    d->updateHorizontalAnchors();
}

void QmlGraphicsAnchors::resetLeft()
{
    Q_D(QmlGraphicsAnchors);
    d->usedAnchors &= ~HasLeftAnchor;
    d->remDepend(d->left.item);
    d->left = QmlGraphicsAnchorLine();
    emit leftChanged();
    d->updateHorizontalAnchors();
}

QmlGraphicsAnchorLine QmlGraphicsAnchors::right() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->right;
}

void QmlGraphicsAnchors::setRight(const QmlGraphicsAnchorLine &edge)
{
    Q_D(QmlGraphicsAnchors);
    if (!d->checkHAnchorValid(edge) || d->right == edge)
        return;

    d->usedAnchors |= HasRightAnchor;

    if (!d->checkHValid()) {
        d->usedAnchors &= ~HasRightAnchor;
        return;
    }

    d->remDepend(d->right.item);
    d->right = edge;
    d->addDepend(d->right.item);
    emit rightChanged();
    d->updateHorizontalAnchors();
}

void QmlGraphicsAnchors::resetRight()
{
    Q_D(QmlGraphicsAnchors);
    d->usedAnchors &= ~HasRightAnchor;
    d->remDepend(d->right.item);
    d->right = QmlGraphicsAnchorLine();
    emit rightChanged();
    d->updateHorizontalAnchors();
}

QmlGraphicsAnchorLine QmlGraphicsAnchors::horizontalCenter() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->hCenter;
}

void QmlGraphicsAnchors::setHorizontalCenter(const QmlGraphicsAnchorLine &edge)
{
    Q_D(QmlGraphicsAnchors);
    if (!d->checkHAnchorValid(edge) || d->hCenter == edge)
        return;

    d->usedAnchors |= HasHCenterAnchor;

    if (!d->checkHValid()) {
        d->usedAnchors &= ~HasHCenterAnchor;
        return;
    }

    d->remDepend(d->hCenter.item);
    d->hCenter = edge;
    d->addDepend(d->hCenter.item);
    emit horizontalCenterChanged();
    d->updateHorizontalAnchors();
}

void QmlGraphicsAnchors::resetHorizontalCenter()
{
    Q_D(QmlGraphicsAnchors);
    d->usedAnchors &= ~HasHCenterAnchor;
    d->remDepend(d->hCenter.item);
    d->hCenter = QmlGraphicsAnchorLine();
    emit horizontalCenterChanged();
    d->updateHorizontalAnchors();
}

qreal QmlGraphicsAnchors::leftMargin() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->leftMargin;
}

void QmlGraphicsAnchors::setLeftMargin(qreal offset)
{
    Q_D(QmlGraphicsAnchors);
    if (d->leftMargin == offset)
        return;
    d->leftMargin = offset;
    if(d->fill)
        d->fillChanged();
    else
        d->updateHorizontalAnchors();
    emit leftMarginChanged();
}

qreal QmlGraphicsAnchors::rightMargin() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->rightMargin;
}

void QmlGraphicsAnchors::setRightMargin(qreal offset)
{
    Q_D(QmlGraphicsAnchors);
    if (d->rightMargin == offset)
        return;
    d->rightMargin = offset;
    if(d->fill)
        d->fillChanged();
    else
        d->updateHorizontalAnchors();
    emit rightMarginChanged();
}

qreal QmlGraphicsAnchors::margins() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->margins;
}

void QmlGraphicsAnchors::setMargins(qreal offset)
{
    Q_D(QmlGraphicsAnchors);
    if (d->margins == offset)
        return;
    //###Is it significantly faster to set them directly so we can call fillChanged only once?
    if(!d->rightMargin || d->rightMargin == d->margins)
        setRightMargin(offset);
    if(!d->leftMargin || d->leftMargin == d->margins)
        setLeftMargin(offset);
    if(!d->topMargin || d->topMargin == d->margins)
        setTopMargin(offset);
    if(!d->bottomMargin || d->bottomMargin == d->margins)
        setBottomMargin(offset);
    d->margins = offset;
    emit marginsChanged();

}

qreal QmlGraphicsAnchors::horizontalCenterOffset() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->hCenterOffset;
}

void QmlGraphicsAnchors::setHorizontalCenterOffset(qreal offset)
{
    Q_D(QmlGraphicsAnchors);
    if (d->hCenterOffset == offset)
        return;
    d->hCenterOffset = offset;
    if(d->centerIn)
        d->centerInChanged();
    else
        d->updateHorizontalAnchors();
    emit horizontalCenterOffsetChanged();
}

qreal QmlGraphicsAnchors::topMargin() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->topMargin;
}

void QmlGraphicsAnchors::setTopMargin(qreal offset)
{
    Q_D(QmlGraphicsAnchors);
    if (d->topMargin == offset)
        return;
    d->topMargin = offset;
    if(d->fill)
        d->fillChanged();
    else
        d->updateVerticalAnchors();
    emit topMarginChanged();
}

qreal QmlGraphicsAnchors::bottomMargin() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->bottomMargin;
}

void QmlGraphicsAnchors::setBottomMargin(qreal offset)
{
    Q_D(QmlGraphicsAnchors);
    if (d->bottomMargin == offset)
        return;
    d->bottomMargin = offset;
    if(d->fill)
        d->fillChanged();
    else
        d->updateVerticalAnchors();
    emit bottomMarginChanged();
}

qreal QmlGraphicsAnchors::verticalCenterOffset() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->vCenterOffset;
}

void QmlGraphicsAnchors::setVerticalCenterOffset(qreal offset)
{
    Q_D(QmlGraphicsAnchors);
    if (d->vCenterOffset == offset)
        return;
    d->vCenterOffset = offset;
    if(d->centerIn)
        d->centerInChanged();
    else
        d->updateVerticalAnchors();
    emit verticalCenterOffsetChanged();
}

qreal QmlGraphicsAnchors::baselineOffset() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->baselineOffset;
}

void QmlGraphicsAnchors::setBaselineOffset(qreal offset)
{
    Q_D(QmlGraphicsAnchors);
    if (d->baselineOffset == offset)
        return;
    d->baselineOffset = offset;
    d->updateVerticalAnchors();
    emit baselineOffsetChanged();
}

QmlGraphicsAnchors::UsedAnchors QmlGraphicsAnchors::usedAnchors() const
{
    Q_D(const QmlGraphicsAnchors);
    return d->usedAnchors;
}

bool QmlGraphicsAnchorsPrivate::checkHValid() const
{
    if (usedAnchors & QmlGraphicsAnchors::HasLeftAnchor &&
        usedAnchors & QmlGraphicsAnchors::HasRightAnchor &&
        usedAnchors & QmlGraphicsAnchors::HasHCenterAnchor) {
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot specify left, right, and hcenter anchors.");
        return false;
    }

    return true;
}

bool QmlGraphicsAnchorsPrivate::checkHAnchorValid(QmlGraphicsAnchorLine anchor) const
{
    if (!anchor.item) {
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot anchor to a null item.");
        return false;
    } else if (anchor.anchorLine & QmlGraphicsAnchorLine::Vertical_Mask) {
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot anchor a horizontal edge to a vertical edge.");
        return false;
    } else if (anchor.item != item->parentItem() && anchor.item->parentItem() != item->parentItem()){
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot anchor to an item that isn't a parent or sibling.");
        return false;
    } else if (anchor.item == item) {
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot anchor item to self.");
        return false;
    }

    return true;
}

bool QmlGraphicsAnchorsPrivate::checkVValid() const
{
    if (usedAnchors & QmlGraphicsAnchors::HasTopAnchor &&
        usedAnchors & QmlGraphicsAnchors::HasBottomAnchor &&
        usedAnchors & QmlGraphicsAnchors::HasVCenterAnchor) {
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot specify top, bottom, and vcenter anchors.");
        return false;
    } else if (usedAnchors & QmlGraphicsAnchors::HasBaselineAnchor &&
               (usedAnchors & QmlGraphicsAnchors::HasTopAnchor ||
                usedAnchors & QmlGraphicsAnchors::HasBottomAnchor ||
                usedAnchors & QmlGraphicsAnchors::HasVCenterAnchor)) {
        qmlInfo(item) << QmlGraphicsAnchors::tr("Baseline anchor cannot be used in conjunction with top, bottom, or vcenter anchors.");
        return false;
    }

    return true;
}

bool QmlGraphicsAnchorsPrivate::checkVAnchorValid(QmlGraphicsAnchorLine anchor) const
{
    if (!anchor.item) {
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot anchor to a null item.");
        return false;
    } else if (anchor.anchorLine & QmlGraphicsAnchorLine::Horizontal_Mask) {
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot anchor a vertical edge to a horizontal edge.");
        return false;
    } else if (anchor.item != item->parentItem() && anchor.item->parentItem() != item->parentItem()){
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot anchor to an item that isn't a parent or sibling.");
        return false;
    } else if (anchor.item == item){
        qmlInfo(item) << QmlGraphicsAnchors::tr("Cannot anchor item to self.");
        return false;
    }

    return true;
}

#include <moc_qmlgraphicsanchors_p.cpp>

QT_END_NAMESPACE
