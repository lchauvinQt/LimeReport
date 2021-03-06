/***************************************************************************
 *   This file is part of the Lime Report project                          *
 *   Copyright (C) 2015 by Alexander Arin                                  *
 *   arin_a@bk.ru                                                          *
 *                                                                         *
 **                   GNU General Public License Usage                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 **                  GNU Lesser General Public License                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation, either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library.                                      *
 *   If not, see <http://www.gnu.org/licenses/>.                           *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 ****************************************************************************/
#include "lrbanddesignintf.h"
#include "lritemdesignintf.h"
#include "lrglobal.h"
#include <algorithm>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

namespace LimeReport {

BandMarker::BandMarker(BandDesignIntf *band, QGraphicsItem* parent)
    :QGraphicsItem(parent),m_rect(0,0,30,30),m_band(band)
{}

QRectF BandMarker::boundingRect() const
{
    return m_rect;
}

void BandMarker::paint(QPainter *painter, const QStyleOptionGraphicsItem* /**option*/, QWidget* /*widget*/)
{
    painter->save();
    painter->setOpacity(Const::BAND_MARKER_OPACITY);
    painter->fillRect(boundingRect(),m_color);
    painter->setOpacity(1);
    painter->setRenderHint(QPainter::Antialiasing);
    qreal size = (boundingRect().width()<boundingRect().height()) ? boundingRect().width() : boundingRect().height();
    QRectF r = QRectF(0,0,size,size);
    painter->setBrush(Qt::white);
    painter->setPen(Qt::white);
    painter->drawEllipse(r.adjusted(5,5,-5,-5));
    if (m_band->isSelected()){
        painter->setBrush(LimeReport::Const::SELECTION_COLOR);
        painter->drawEllipse(r.adjusted(7,7,-7,-7));
    }
    painter->restore();
}

void BandMarker::setHeight(qreal height)
{
    if (m_rect.height()!=height){
        prepareGeometryChange();
        m_rect.setHeight(height);
    }
}

void BandMarker::setWidth(qreal width)
{
    if (m_rect.width()!=width){
        prepareGeometryChange();
        m_rect.setWidth(width);
    }
}

void BandMarker::setColor(QColor color)
{
    if (m_color!=color){
        m_color = color;
        update(boundingRect());
    }
}

void BandMarker::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button()==Qt::LeftButton) {
        if (!(event->modifiers() & Qt::ControlModifier))
            m_band->scene()->clearSelection();
        m_band->setSelected(true);
        update(0,0,boundingRect().width(),boundingRect().width());
    }
}

bool Segment::intersect(Segment value)
{
    return ((value.m_end>=m_begin)&&(value.m_end<=m_end))   ||
           ((value.m_begin>=m_begin)&&(value.m_end>=m_end))   ||
           ((value.m_begin>=m_begin)&&(value.m_end<=m_end)) ||
           ((value.m_begin<m_begin)&&(value.m_end>m_end)) ;
}

qreal Segment::intersectValue(Segment value)
{
    if ((value.m_end>=m_begin)&&(value.m_end<=m_end)){
        return value.m_end-m_begin;
    }
    if ((value.m_begin>=m_begin)&&(value.m_end>=m_end)){
        return m_end-value.m_begin;
    }
    if ((value.m_begin>=m_begin)&&(value.m_end<=m_end)){
        return value.m_end-value.m_begin;
    }
    if ((value.m_begin<m_begin)&&(value.m_end>m_end)){
        return m_end-m_begin;
    }
    return 0;
}

BandDesignIntf::BandDesignIntf(BandsType bandType, const QString &xmlTypeName, QObject* owner, QGraphicsItem *parent) :
    BaseDesignIntf(xmlTypeName, owner,parent),
    m_bandType(bandType),
    m_bandIndex(static_cast<int>(bandType)),
    m_dataSourceName(""),
    m_autoHeight(true),
    m_keepBottomSpace(false),
    m_parentBand(0),
    m_parentBandName(""),
    m_bandMarker(0),
    m_tryToKeepTogether(false),
    m_splitable(false),
    m_keepFooterTogether(false),
    m_maxScalePercent(0),
    m_sliceLastRow(false),
    m_printIfEmpty(false),
    m_columnsCount(1),
    m_columnIndex(0),
    m_columnsFillDirection(Horizontal),
    m_reprintOnEachPage(false),
    m_startNewPage(false),
    m_startFromNewPage(false),
    m_printAlways(false),
    m_repeatOnEachRow(false)
{
    setPossibleResizeDirectionFlags(ResizeBottom);
    setPossibleMoveFlags(TopBotom);

    if (parent) {
        BaseDesignIntf* parentItem = dynamic_cast<BaseDesignIntf*>(parent);
        if (parentItem) setWidth(parentItem->width());
    }

    setHeight(100);
    setFixedPos(true);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    m_bandMarker = new BandMarker(this);
    m_bandMarker->setColor(Qt::magenta);
    m_bandMarker->setHeight(height());
    m_bandMarker->setPos(pos().x()-m_bandMarker->width(),pos().y());
    if (scene()) scene()->addItem(m_bandMarker);

    m_bandNameLabel = new BandNameLabel(this);
    m_bandNameLabel->setVisible(false);
    if (scene()) scene()->addItem(m_bandNameLabel);
    m_alternateBackgroundColor = backgroundColor();
}

BandDesignIntf::~BandDesignIntf()
{
//    if (itemMode()&DesignMode){
//        foreach(BandDesignIntf* band,childBands()) {
//            removeChildBand(band);
//            delete band;
//        }
//    }
    delete m_bandMarker;
    delete m_bandNameLabel;
}

void BandDesignIntf::paint(QPainter *ppainter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (backgroundColor()!=Qt::white) {
        ppainter->fillRect(rect(),backgroundColor());
    }
    if (itemMode()&DesignMode){
        ppainter->save();
        QString bandText = objectName();
        if (parentBand()) bandText+=QLatin1String(" connected to ")+parentBand()->objectName();
        QFont font("Arial",7*Const::fontFACTOR,-1,true);
        QFontMetrics fontMetrics(font);

        QVector<QRectF> bandNameRects;
        bandNameRects.push_back(QRectF(8,8,fontMetrics.width(" "+bandText+" "),fontMetrics.height()));
        //bandNameRects.push_back(QRectF(width()-fontMetrics.width(" "+bandText+" "),2,fontMetrics.width(" "+bandText+" "),fontMetrics.height()));
        //bandNameRects.push_back(QRectF(2,height()-fontMetrics.height(),fontMetrics.width(" "+bandText+" "),fontMetrics.height()));
        //bandNameRects.push_back(QRectF(width()-fontMetrics.width(" "+bandText+" "),height()-fontMetrics.height(),fontMetrics.width(" "+bandText+" "),fontMetrics.height()));
        //if (bandNameRects[0].intersects(bandNameRects[2])) bandNameRects.remove(2,2);
        //if (isSelected()) ppainter->setPen(QColor(167,244,167));
       // else ppainter->setPen(QColor(220,220,220));

        ppainter->setFont(font);
        for (int i=0;i<bandNameRects.count();i++){
            QRectF labelRect = bandNameRects[i].adjusted(-2,-2,2,2);
            if ((labelRect.height())<height() && (childBaseItems().isEmpty()) && !isSelected()){
                ppainter->setRenderHint(QPainter::Antialiasing);
                ppainter->setBrush(bandColor());
                ppainter->setOpacity(Const::BAND_NAME_AREA_OPACITY);
                ppainter->drawRoundedRect(labelRect,8,8);
                ppainter->setOpacity(Const::BAND_NAME_TEXT_OPACITY);
                ppainter->setPen(Qt::black);
                ppainter->drawText(bandNameRects[i],Qt::AlignHCenter,bandText);
            }
        }
        ppainter->restore();
    }
    BaseDesignIntf::paint(ppainter,option,widget);
}

BandDesignIntf::BandsType  BandDesignIntf::bandType() const
{
    return m_bandType;
}

QString  BandDesignIntf::bandTitle() const
{
    QString result = objectName();
    if (parentBand()) result +=tr(" connected to ")+parentBand()->objectName();
    return result;
}

QIcon  BandDesignIntf::bandIcon() const
{
    return QIcon();
}

int BandDesignIntf::bandIndex() const
{
    return m_bandIndex;
}

void BandDesignIntf::setBandIndex(int value)
{
    m_bandIndex=value;
}

void BandDesignIntf::changeBandIndex(int value)
{
    int indexOffset = value - m_bandIndex;
    foreach(BandDesignIntf* band, childBands()){
        int newIndex = band->bandIndex()+indexOffset;
        band->changeBandIndex(newIndex);
    }
    setBandIndex(value);
}

bool BandDesignIntf::isUnique() const
{
    return true;
}

QString BandDesignIntf::datasourceName(){
    return m_dataSourceName;
}

void BandDesignIntf::setDataSourceName(const QString &datasource){
    m_dataSourceName=datasource;
}

void BandDesignIntf::addChildBand(BandDesignIntf *band)
{
    m_childBands.append(band);
    connect(band,SIGNAL(destroyed(QObject*)),this,SLOT(childBandDeleted(QObject*)));
}

void BandDesignIntf::removeChildBand(BandDesignIntf *band)
{
    m_childBands.removeAt(m_childBands.indexOf(band));
}

void BandDesignIntf::setParentBand(BandDesignIntf *band)
{
    m_parentBand=band;
    if (band){
        if (parentBandName().compare(band->objectName(),Qt::CaseInsensitive)!=0)
          setParentBandName(band->objectName());
        band->addChildBand(this);
    }
}

void BandDesignIntf::setParentBandName(const QString &parentBandName)
{
    m_parentBandName=parentBandName;
    if (itemMode()&DesignMode && !m_parentBandName.isEmpty()){
        if ((parentBand() == 0 )||(parentBand()->objectName()!= parentBandName))
            setParentBand(findParentBand());
    }
}

QString BandDesignIntf::parentBandName(){
    if (!m_parentBand) return m_parentBandName;
    else return m_parentBand->objectName();
}

bool BandDesignIntf::isConnectedToBand(BandDesignIntf::BandsType bandType) const
{
    foreach(BandDesignIntf* band,childBands()) if (band->bandType()==bandType) return true;
    return false;
}

int BandDesignIntf::maxChildIndex(QSet<BandDesignIntf::BandsType> ignoredBands) const{
    int curIndex = bandIndex();
    foreach(BandDesignIntf* childBand, childBands()){
        if (!ignoredBands.contains(childBand->bandType()) && childBand->bandIndex()>bandIndex()){
            curIndex = std::max(curIndex,childBand->maxChildIndex(ignoredBands));
        }
    }
    return curIndex;
}

int BandDesignIntf::minChildIndex(BandDesignIntf::BandsType bandType){
    int curIndex = bandIndex();
    foreach(BandDesignIntf* childBand, childBands()){
        if (curIndex>childBand->bandIndex() && (childBand->bandType()>bandType)){
            curIndex = childBand->bandIndex();
        }
    }
    return curIndex;
}

QList<BandDesignIntf *> BandDesignIntf::childrenByType(BandDesignIntf::BandsType type)
{
    QList<BandDesignIntf*> resList;
    foreach(BandDesignIntf* item,childBands()){
        if (item->bandType()==type) resList<<item;
    }
    qSort(resList.begin(),resList.end(),bandIndexLessThen);
    return resList;
}

bool BandDesignIntf::canBeSplitted(int height) const
{
    if (isSplittable()){
        foreach(QGraphicsItem* qgItem,childItems()){
            BaseDesignIntf* item=dynamic_cast<BaseDesignIntf*>(qgItem);
            if (item)
                if ((item->minHeight()>height) && (item->minHeight()>(this->height()-height))) return false;
        }
    }
    return isSplittable();
}

bool BandDesignIntf::isEmpty() const
{
    foreach(QGraphicsItem* qgItem,childItems()){
        BaseDesignIntf* item = dynamic_cast<BaseDesignIntf*>(qgItem);
        if ((item)&&(!item->isEmpty())) return false;
    }
    return true;
}

bool BandDesignIntf::isNeedRender() const
{
    return true;
}

void BandDesignIntf::setTryToKeepTogether(bool value)
{
    m_tryToKeepTogether=value;
}

bool BandDesignIntf::tryToKeepTogether()
{
    return m_tryToKeepTogether;
}

void BandDesignIntf::checkEmptyTable(){
    bool isEmpty = true;
    bool allItemsIsText = true;
    foreach (QGraphicsItem* qgItem, childItems()) {
        BaseDesignIntf* item = dynamic_cast<BaseDesignIntf*>(qgItem);
        if (item && !item->isEmpty()) isEmpty = false;
        if (!item) allItemsIsText = false;
    }
    if (isEmpty && allItemsIsText){
        foreach (QGraphicsItem* qgItem, childItems()) {
            ContentItemDesignIntf* item = dynamic_cast<ContentItemDesignIntf*>(qgItem);
            item->setHeight(0);
        }
    }
}

void BandDesignIntf::setColumnsCount(int value)
{
    if (m_columnsCount!=value && value>0){
        qreal oldValue = m_columnsCount;
        qreal fullWidth = m_columnsCount * width();
        m_columnsCount = value;
        if (!isLoading()){
            setWidth(fullWidth/m_columnsCount);
            notify("columnsCount",oldValue,value);
        }
    }
}

void BandDesignIntf::setColumnsFillDirection(BandDesignIntf::BandColumnsLayoutType value)
{
    if (m_columnsFillDirection!=value){
        qreal oldValue = m_columnsFillDirection;
        m_columnsFillDirection = value;
        if (!isLoading())
            notify("columnsFillDirection",oldValue,value);
    }

}

void BandDesignIntf::moveItemsDown(qreal startPos, qreal offset){
   foreach (QGraphicsItem* item, childItems()){
       if (item->pos().y()>=startPos)
           item->setPos(item->x(),item->y()+offset);
   }
}

BaseDesignIntf* BandDesignIntf::cloneUpperPart(int height, QObject *owner, QGraphicsItem *parent)
{
    int maxBottom = 0;
    BandDesignIntf* upperPart = dynamic_cast<BandDesignIntf*>(createSameTypeItem(owner,parent));
    BaseDesignIntf* upperItem;

    upperPart->initFromItem(this);

    foreach(QGraphicsItem* qgItem,childItems()){
        BaseDesignIntf* item = dynamic_cast<BaseDesignIntf*>(qgItem);
        if (item){
            if (item->geometry().bottom()<=height){
               upperItem = item->cloneItem(item->itemMode(),upperPart,upperPart);
               if (maxBottom<upperItem->geometry().bottom()) maxBottom = upperItem->geometry().bottom();
            }
            else if ((item->geometry().top()<height) && (item->geometry().bottom()>height)){
                int sliceHeight = height-item->geometry().top();
                if (!item->isSplittable()){
                    if (sliceHeight>(this->height()-sliceHeight)){
                        upperItem = item->cloneItem(item->itemMode(),upperPart,upperPart);
                        upperItem->setHeight(height);
                    } else {
                        item->cloneEmpty(sliceHeight,upperPart,upperPart); //for table
                        //qgItem->setPos(item->pos().x(),item->pos().y()+((height+1)-item->geometry().top()));
                        moveItemsDown(item->pos().y(),(height+1)-item->geometry().top());
                    }
                } else if (item->canBeSplitted(sliceHeight)){
                    upperItem = item->cloneUpperPart(sliceHeight,upperPart,upperPart);
                    if (maxBottom<upperItem->geometry().bottom()) maxBottom = upperItem->geometry().bottom();
                    m_slicedItems.insert(upperItem->objectName(),upperItem);
                } else {
                    item->cloneEmpty(sliceHeight,upperPart,upperPart); //for table
                    moveItemsDown(item->pos().y(),(height+1)-item->geometry().top());
                    //qgItem->setPos(item->pos().x(),item->pos().y()+((height+1)-item->geometry().top()));
                }
            }
        }
    }
    upperPart->setHeight(height);
    height = maxBottom;
    return upperPart;
}

bool itemLessThen(QGraphicsItem* i1, QGraphicsItem* i2){
    return i1->pos().y()<i2->pos().y();
}

BaseDesignIntf *BandDesignIntf::cloneBottomPart(int height, QObject *owner, QGraphicsItem *parent)
{
    BandDesignIntf* bottomPart = dynamic_cast<BandDesignIntf*>(createSameTypeItem(owner,parent));
    bottomPart->initFromItem(this);

    QList<QGraphicsItem*> bandItems;
    bandItems = childItems();
    std::sort(bandItems.begin(),bandItems.end(), itemLessThen);

    foreach(QGraphicsItem* qgItem, bandItems){
        BaseDesignIntf* item = dynamic_cast<BaseDesignIntf*>(qgItem);

        if (item){
            if (item->geometry().top()>height){
                BaseDesignIntf* tmpItem = item->cloneItem(item->itemMode(),bottomPart,bottomPart);
                tmpItem->setPos(tmpItem->pos().x(),tmpItem->pos().y()-height);
            }
            else if ((item->geometry().top()<height) && (item->geometry().bottom()>height)){
                int sliceHeight = height-item->geometry().top();
                if (item->canBeSplitted(sliceHeight)) {
                    BaseDesignIntf* tmpItem=item->cloneBottomPart(sliceHeight,bottomPart,bottomPart);
                    tmpItem->setPos(tmpItem->pos().x(),0);
                    if (tmpItem->pos().y()<0) tmpItem->setPos(tmpItem->pos().x(),0);
                    qreal sizeOffset = (m_slicedItems.value(tmpItem->objectName())->height()+tmpItem->height()) - item->height();
                    qreal bottomOffset = (height - m_slicedItems.value(tmpItem->objectName())->pos().y())-m_slicedItems.value(tmpItem->objectName())->height();
                    moveItemsDown(item->pos().y()+item->height(), sizeOffset + bottomOffset);
                }
                else if (item->isSplittable()){
                    BaseDesignIntf* tmpItem = item->cloneItem(item->itemMode(),bottomPart,bottomPart);
                    tmpItem->setPos(tmpItem->pos().x(),0);
                } else {
                    if ((item->geometry().bottom()-height)>height){
                        BaseDesignIntf* tmpItem = item->cloneItem(item->itemMode(),bottomPart,bottomPart);
                        tmpItem->setPos(tmpItem->pos().x(),0);
                        tmpItem->setHeight((this->height()-height));
                    } else {
                        BaseDesignIntf* tmpItem = item->cloneEmpty((this->height()-height),bottomPart,bottomPart);
                        if (tmpItem)
                            tmpItem->setPos(tmpItem->pos().x(),0);
                    }
                }
            }
        }
    }

    return bottomPart;
}

void BandDesignIntf::parentObjectLoadFinished()
{
    if (!parentBandName().isEmpty())
        setParentBand(findParentBand());
}

void BandDesignIntf::objectLoadFinished()
{
    m_bandMarker->setHeight(height());
    BaseDesignIntf::objectLoadFinished();
}

void BandDesignIntf::emitBandRendered(BandDesignIntf* band)
{
    emit bandRendered(band);
}

void BandDesignIntf::setSplittable(bool value){
    if (m_splitable!=value){
        bool oldValue = m_splitable;
        m_splitable = value;
        if (!isLoading())
            notify("splittable",oldValue,value);
    }
}

bool itemSortContainerLessThen(const PItemSortContainer c1, const PItemSortContainer c2)
{
    VSegment vS1(c1->m_rect),vS2(c2->m_rect);
    HSegment hS1(c1->m_rect),hS2(c2->m_rect);
    if (vS1.intersectValue(vS2)>hS1.intersectValue(hS2))
        return c1->m_rect.x()<c2->m_rect.x();
    else return c1->m_rect.y()<c2->m_rect.y();
}

bool bandIndexLessThen(const BandDesignIntf* b1, const BandDesignIntf* b2)
{
    return b1->bandIndex()<b2->bandIndex();
}

void BandDesignIntf::snapshotItemsLayout()
{
    m_bandItems.clear();
    foreach(BaseDesignIntf *childItem,childBaseItems()){
          m_bandItems.append(PItemSortContainer(new ItemSortContainer(childItem)));
    }
    qSort(m_bandItems.begin(),m_bandItems.end(),itemSortContainerLessThen);
}

void BandDesignIntf::arrangeSubItems(RenderPass pass, DataSourceManager *dataManager, ArrangeType type)
{
    bool needArrage=(type==Force);

    foreach (PItemSortContainer item, m_bandItems) {
        if (item->m_item->isNeedUpdateSize(pass)){
            item->m_item->updateItemSize(dataManager, pass);
            needArrage=true;
        }
    }

    if (needArrage){
        //qSort(m_bandItems.begin(),m_bandItems.end(),itemSortContainerLessThen);
        for (int i=0;i<m_bandItems.count();i++)
            for (int j=i;j<m_bandItems.count();j++){
                if ((i!=j) && (m_bandItems[i]->m_item->collidesWithItem(m_bandItems[j]->m_item))){
                    HSegment hS1(m_bandItems[j]->m_rect),hS2(m_bandItems[i]->m_rect);
                    VSegment vS1(m_bandItems[j]->m_rect),vS2(m_bandItems[i]->m_rect);
                    if (m_bandItems[i]->m_rect.bottom()<m_bandItems[i]->m_item->geometry().bottom()){
                       if (hS1.intersectValue(hS2)>vS1.intersectValue(vS2))
                           m_bandItems[j]->m_item->setY(m_bandItems[i]->m_item->y()+m_bandItems[i]->m_item->height()
                                                      +m_bandItems[j]->m_rect.top()-m_bandItems[i]->m_rect.bottom());

                    }
                    if (m_bandItems[i]->m_rect.right()<m_bandItems[i]->m_item->geometry().right()){
                       if (vS1.intersectValue(vS2)>hS1.intersectValue(hS2))
                       m_bandItems[j]->m_item->setX(m_bandItems[i]->m_item->geometry().right()+
                                                  (m_bandItems[j]->m_rect.x()-m_bandItems[i]->m_rect.right()));
                    }
                }
            }
    }

    if(needArrage||pass==FirstPass){
        int maxBottom = findMaxBottom();
        foreach(BaseDesignIntf* item,childBaseItems()){
            ItemDesignIntf* childItem=dynamic_cast<ItemDesignIntf*>(item);
            if (childItem){
                if (childItem->stretchToMaxHeight())
                    childItem->setHeight(maxBottom-childItem->geometry().top());
            }
        }
    }

}

qreal BandDesignIntf::findMaxBottom()
{
    qreal maxBottom=0;
    foreach(QGraphicsItem* item,childItems()){
        BaseDesignIntf* subItem = dynamic_cast<BaseDesignIntf *>(item);
        if(subItem)
           if ( subItem->isVisible() && (subItem->geometry().bottom()>maxBottom) )
               maxBottom=subItem->geometry().bottom();
    }
    return maxBottom;
}

qreal BandDesignIntf::findMaxHeight(){
    qreal maxHeight=0;
    foreach(QGraphicsItem* item,childItems()){
        BaseDesignIntf* subItem = dynamic_cast<BaseDesignIntf *>(item);
        if(subItem)
           if (subItem->geometry().height()>maxHeight) maxHeight=subItem->geometry().height();
    }
    return maxHeight;
}

void BandDesignIntf::trimToMaxHeight(int maxHeight)
{
    foreach(BaseDesignIntf* item,childBaseItems()){
        if (item->height()>maxHeight) item->setHeight(maxHeight);
    }
}

void BandDesignIntf::setBandTypeText(const QString &value){
    m_bandTypeText=value;
    m_bandNameLabel->updateLabel();
}

QSet<BandDesignIntf::BandsType> BandDesignIntf::groupBands()
{
    QSet<BandDesignIntf::BandsType> result;
    result<<GroupHeader<<GroupFooter;
    return result;
}

QSet<BandDesignIntf::BandsType> BandDesignIntf::subdetailBands()
{
    QSet<BandDesignIntf::BandsType> result;
    result<<SubDetailBand<<SubDetailHeader<<SubDetailFooter;
    return result;
}

BandDesignIntf* BandDesignIntf::findParentBand()
{
    if (parent()&&(!dynamic_cast<BaseDesignIntf*>(parent())->isLoading())){
        foreach(QObject* item, parent()->children()){
            BandDesignIntf* band = dynamic_cast<BandDesignIntf*>(item);
            if(band&&(band->objectName().compare(parentBandName(),Qt::CaseInsensitive)==0))
                return band;
        }
    }
    return 0;
}

void BandDesignIntf::geometryChangedEvent(QRectF, QRectF )
{
    if (((itemMode()&DesignMode) || (itemMode()&EditMode))&&parentItem()){
        QPointF sp = parentItem()->mapToScene(pos());
        if (m_bandMarker){
            m_bandMarker->setPos((sp.x()-m_bandMarker->boundingRect().width()),sp.y());
            m_bandMarker->setHeight(rect().height());
        }
    }
    foreach (BaseDesignIntf* item, childBaseItems()) {
        if (item->itemAlign()!=DesignedItemAlign){
            item->updateItemAlign();
        }
    }
}

QVariant BandDesignIntf::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if ((change==ItemPositionChange)&&((itemMode()&DesignMode)||(itemMode()&EditMode))){
        if (m_bandMarker){
            m_bandMarker->setPos((value.toPointF().x()-m_bandMarker->boundingRect().width()),
                                 value.toPointF().y());
        }
    }
    if (change==ItemSelectedChange){
        if (m_bandMarker){
            m_bandMarker->update(0,0,
                                 m_bandMarker->boundingRect().width(),
                                 m_bandMarker->boundingRect().width());
            m_bandNameLabel->updateLabel();
            m_bandNameLabel->setVisible(value.toBool());

        }
    }
    if (change==ItemChildAddedChange || change==ItemChildRemovedChange){
        update(rect());
    }
    return BaseDesignIntf::itemChange(change,value);
}

void BandDesignIntf::initMode(ItemMode mode)
{
    BaseDesignIntf::initMode(mode);
    if ((mode==PreviewMode)||(mode==PrintMode)){
        m_bandMarker->setVisible(false);
    }
}

QColor BandDesignIntf::bandColor() const
{
    return Qt::darkBlue;
}

void BandDesignIntf::setMarkerColor(QColor color)
{
    if (m_bandMarker) m_bandMarker->setColor(color);
}

void BandDesignIntf::childBandDeleted(QObject *band)
{
    m_childBands.removeAt(m_childBands.indexOf(reinterpret_cast<BandDesignIntf*>(band)));
}

QColor BandDesignIntf::alternateBackgroundColor() const
{
    if (metaObject()->indexOfProperty("alternateBackgroundColor")!=-1)
        return m_alternateBackgroundColor;
    else
        return backgroundColor();
}

void BandDesignIntf::setAlternateBackgroundColor(const QColor &alternateBackgroundColor)
{
    m_alternateBackgroundColor = alternateBackgroundColor;
}

bool BandDesignIntf::repeatOnEachRow() const
{
    return m_repeatOnEachRow;
}

void BandDesignIntf::setRepeatOnEachRow(bool repeatOnEachRow)
{
    m_repeatOnEachRow = repeatOnEachRow;
}

bool BandDesignIntf::printAlways() const
{
    return m_printAlways;
}

void BandDesignIntf::setPrintAlways(bool printAlways)
{
    m_printAlways = printAlways;
}

bool BandDesignIntf::startFromNewPage() const
{
    return m_startFromNewPage;
}

void BandDesignIntf::setStartFromNewPage(bool startWithNewPage)
{
    m_startFromNewPage = startWithNewPage;
}

bool BandDesignIntf::startNewPage() const
{
    return m_startNewPage;
}

void BandDesignIntf::setStartNewPage(bool startNewPage)
{
    m_startNewPage = startNewPage;
}

bool BandDesignIntf::reprintOnEachPage() const
{
    return m_reprintOnEachPage;
}

void BandDesignIntf::setReprintOnEachPage(bool reprintOnEachPage)
{
    m_reprintOnEachPage = reprintOnEachPage;
}

int BandDesignIntf::columnIndex() const
{
    return m_columnIndex;
}

void BandDesignIntf::setColumnIndex(int columnIndex)
{
    m_columnIndex = columnIndex;
}

bool BandDesignIntf::printIfEmpty() const
{
    return m_printIfEmpty;
}

void BandDesignIntf::setPrintIfEmpty(bool printIfEmpty)
{
    m_printIfEmpty = printIfEmpty;
}

BandDesignIntf *BandDesignIntf::bandHeader()
{
    foreach (BandDesignIntf* band, childBands()) {
        if (band->isHeader() && !band->isGroupHeader())
            return band;
    }
    return 0;
}

BandDesignIntf *BandDesignIntf::bandFooter()
{
    foreach (BandDesignIntf* band, childBands()) {
        if (band->isFooter()) return band;
    }
    return 0;
}

bool BandDesignIntf::sliceLastRow() const
{
    return m_sliceLastRow;
}

void BandDesignIntf::setSliceLastRow(bool sliceLastRow)
{
    m_sliceLastRow = sliceLastRow;
}

int BandDesignIntf::maxScalePercent() const
{
    return m_maxScalePercent;
}

void BandDesignIntf::setMaxScalePercent(int maxScalePercent)
{
    m_maxScalePercent = maxScalePercent;
}

bool BandDesignIntf::keepFooterTogether() const
{
    return m_keepFooterTogether;
}

void BandDesignIntf::setKeepFooterTogether(bool value)
{
    if (m_keepFooterTogether!=value){
        bool oldValue = m_keepFooterTogether;
        m_keepFooterTogether = value;
        if (!isLoading())
            notify("keepFooterTogether",oldValue,value);
    }
}


void BandDesignIntf::updateItemSize(DataSourceManager* dataManager, RenderPass pass, int maxHeight)
{
    qreal spaceBorder=0;
    if (keepBottomSpaceOption()) spaceBorder=height()-findMaxBottom();
    if (borderLines()!=0){
        spaceBorder += borderLineSize();
    }
    snapshotItemsLayout();
    arrangeSubItems(pass, dataManager);
    if (autoHeight()){
        //if keepBottomSpace()&& height()<findMaxBottom()
        setHeight(findMaxBottom()+spaceBorder);
    }
    if ((maxHeight>0)&&(height()>maxHeight)){
        trimToMaxHeight(maxHeight);
        setHeight(maxHeight);
    }
    BaseDesignIntf::updateItemSize(dataManager, pass, maxHeight);
}

void BandDesignIntf::updateBandNameLabel()
{
    if (m_bandNameLabel) m_bandNameLabel->updateLabel();
}

QColor BandDesignIntf::selectionColor() const
{
    return Qt::yellow;
}

DataBandDesignIntf::DataBandDesignIntf(BandDesignIntf::BandsType bandType, QString xmlTypeName, QObject *owner, QGraphicsItem *parent)
    :BandDesignIntf(bandType,xmlTypeName,owner,parent)
{
}

BandNameLabel::BandNameLabel(BandDesignIntf *band, QGraphicsItem *parent)
    :QGraphicsItem(parent),m_rect(5,5,30,30),m_band(band)
{
    setAcceptHoverEvents(true);
}

void BandNameLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setFont(QFont("Arial",7*Const::fontFACTOR,-1,true));
    painter->setOpacity(1);
    QPen pen(Const::BAND_NAME_BORDER_COLOR);
    //pen.setWidth(2);
    painter->setBrush(Qt::yellow);
    painter->setPen(pen);
    painter->drawRoundedRect(m_rect,8,8);
    painter->setOpacity(0.8);
    painter->setPen(Qt::black);
    painter->drawText(m_rect,Qt::AlignCenter,m_band->bandTitle());
    painter->restore();
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

QRectF BandNameLabel::boundingRect() const
{
    return m_rect;
}

void BandNameLabel::updateLabel()
{
    QFont font("Arial",7*Const::fontFACTOR,-1,true);
    QFontMetrics fontMetrics(font);
    prepareGeometryChange();
    m_rect = QRectF(
                m_band->pos().x()+10,
                m_band->pos().y()-(fontMetrics.height()+10),
                fontMetrics.width(m_band->bandTitle())+20,fontMetrics.height()+10
                );
    update();
}

void BandNameLabel::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    setVisible(false);
    Q_UNUSED(event)
}

}



