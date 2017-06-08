/* This file is part of the KDE project
 * Copyright (C) 2008 Peter Simonsson <peter.simonsson@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "ShapeCollectionDocker.h"

#include "CollectionItemModel.h"

#include <KoShapeFactoryBase.h>
#include <KoShapeRegistry.h>
#include <KoCanvasController.h>
#include <KoToolManager.h>
#include <KoCreateShapesTool.h>
#include <KoShape.h>
#include <KoZoomHandler.h>
#include <KoShapePaintingContext.h>

#include <KoIcon.h>

#include <klocalizedstring.h>
#include <KoResourcePaths.h>
#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <kmessagebox.h>
#include <ksharedconfig.h>

#include <QGridLayout>
#include <QListView>
#include <QListWidget>
#include <QStandardItemModel>
#include <QList>
#include <QSize>
#include <QToolButton>
#include <QDir>
#include <QMenu>
#include <QPainter>
#include <QDebug>

//This class is needed so that the menu returns a sizehint based on the layout and not on the number (0) of menu items
class CollectionMenu : public QMenu
{
public:
    CollectionMenu(QWidget *parent = 0);
    QSize sizeHint() const override;
};

CollectionMenu::CollectionMenu(QWidget *parent)
    : QMenu(parent)
{
}
QSize CollectionMenu::sizeHint() const
{
    return layout()->sizeHint();
}

//
// ShapeCollectionDockerFactory
//

ShapeCollectionDockerFactory::ShapeCollectionDockerFactory()
    : KoDockFactoryBase()
{
}

QString ShapeCollectionDockerFactory::id() const
{
    return QString("ShapeCollectionDocker");
}

QDockWidget *ShapeCollectionDockerFactory::createDockWidget()
{
    ShapeCollectionDocker *docker = new ShapeCollectionDocker();

    return docker;
}

void ShapeCollectionDocker::locationChanged(Qt::DockWidgetArea area)
{
    resize(0, 0);

    switch (area) {
    case Qt::TopDockWidgetArea:
    case Qt::BottomDockWidgetArea:
        m_spacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
        break;
    case Qt::LeftDockWidgetArea:
    case Qt::RightDockWidgetArea:
        m_spacer->changeSize(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        break;
    default:
        break;
    }
    m_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_layout->invalidate();
}

//
// ShapeCollectionDocker
//

ShapeCollectionDocker::ShapeCollectionDocker(QWidget *parent)
    : QDockWidget(parent)
{
    setWindowTitle(i18n("Add Shape"));

    QWidget *mainWidget = new QWidget(this);
    m_layout = new QGridLayout(mainWidget);
    m_layout->setMargin(0);
    m_layout->setHorizontalSpacing(0);
    m_layout->setVerticalSpacing(0);
    m_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    setWidget(mainWidget);

    m_quickView = new QListView(mainWidget);
    m_layout->addWidget(m_quickView, 0, 0);
    m_quickView->setViewMode(QListView::IconMode);
    m_quickView->setDragDropMode(QListView::DragOnly);
    m_quickView->setSelectionMode(QListView::SingleSelection);
    m_quickView->setResizeMode(QListView::Adjust);
    m_quickView->setFlow(QListView::LeftToRight);
    m_quickView->setGridSize(QSize(40, 44));
    m_quickView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_quickView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_quickView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_quickView->setTextElideMode(Qt::ElideNone);
    m_quickView->setWordWrap(true);

    m_spacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_layout->addItem(m_spacer, 1, 2);
    m_layout->setRowStretch(1, 1);
    m_layout->setColumnStretch(2, 1);

    connect(this, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), this, SLOT(locationChanged(Qt::DockWidgetArea)));

    m_moreShapes = new QToolButton(mainWidget);
    m_moreShapes->setText(i18n("More"));
    m_moreShapes->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_moreShapes->setIconSize(QSize(32, 32));
    m_moreShapes->setIcon(koIcon("shape-choose"));
    m_moreShapes->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_layout->addWidget(m_moreShapes, 0, 1);

    m_moreShapesContainer = new CollectionMenu(mainWidget);
    m_moreShapes->setMenu(m_moreShapesContainer);
    m_moreShapes->setPopupMode(QToolButton::InstantPopup);
    QGridLayout *containerLayout = new QGridLayout(m_moreShapesContainer);
    containerLayout->setMargin(4);

    m_collectionChooser = new QListWidget(m_moreShapesContainer);
    containerLayout->addWidget(m_collectionChooser, 0, 0, 1, 2);
    m_collectionChooser->setViewMode(QListView::IconMode);
    m_collectionChooser->setSelectionMode(QListView::SingleSelection);
    m_collectionChooser->setResizeMode(QListView::Adjust);
    m_collectionChooser->setGridSize(QSize(75, 64));
    m_collectionChooser->setMovement(QListView::Static);
    m_collectionChooser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_collectionChooser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_collectionChooser, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(activateShapeCollection(QListWidgetItem*)));


    m_collectionView = new QListView(m_moreShapesContainer);
    containerLayout->addWidget(m_collectionView, 0, 2, -1, 1);
    m_collectionView->setViewMode(QListView::IconMode);
    m_collectionView->setDragDropMode(QListView::DragOnly);
    m_collectionView->setSelectionMode(QListView::SingleSelection);
    m_collectionView->setResizeMode(QListView::Adjust);
    m_collectionView->setGridSize(QSize(48 + 20, 48));
    m_collectionView->setFixedSize(QSize(165, 345));
    m_collectionView->setWordWrap(true);


    // Load the default shapes and add them to the combobox
    loadDefaultShapes();
}

void ShapeCollectionDocker::setCanvas(KoCanvasBase *canvas)
{
    setEnabled(canvas != 0);
}

void ShapeCollectionDocker::unsetCanvas()
{
    setEnabled(false);
}

void ShapeCollectionDocker::loadDefaultShapes()
{
    QList<KoCollectionItem> defaultList;
    QList<KoCollectionItem> arrowList;
    QList<KoCollectionItem> funnyList;
    QList<KoCollectionItem> geometricList;
    QList<KoCollectionItem> quicklist;
    int quickCount = 0;

    QStringList quickShapes;
    quickShapes << "TextShapeID" << "PictureShape" << "ChartShape" << "ArtisticText";
    KConfigGroup cfg =  KSharedConfig::openConfig()->group("KoShapeCollection");
    quickShapes = cfg.readEntry("QuickShapes", quickShapes);

    Q_FOREACH (const QString &id, KoShapeRegistry::instance()->keys()) {
        KoShapeFactoryBase *factory = KoShapeRegistry::instance()->value(id);
        // don't show hidden factories
        if (factory->hidden()) {
            continue;
        }
        bool oneAdded = false;

        Q_FOREACH (const KoShapeTemplate &shapeTemplate, factory->templates()) {
            oneAdded = true;
            KoCollectionItem temp;
            temp.id = shapeTemplate.id;
            temp.name = shapeTemplate.name;
            temp.toolTip = shapeTemplate.toolTip;
            temp.icon = KisIconUtils::loadIcon(shapeTemplate.iconName);
            temp.properties = shapeTemplate.properties;
            if (shapeTemplate.family == "funny") {
                funnyList.append(temp);
            } else if (shapeTemplate.family == "arrow") {
                arrowList.append(temp);
            } else if (shapeTemplate.family == "geometric") {
                geometricList.append(temp);
            } else {
                defaultList.append(temp);
            }

            QString id = temp.id;
            if (!shapeTemplate.templateId.isEmpty()) {
                id += '_' + shapeTemplate.templateId;
            }

            if (quickShapes.contains(id)) {
                quicklist.append(temp);
                quickCount++;
            }
        }

        if (!oneAdded) {
            KoCollectionItem temp;
            temp.id = factory->id();
            temp.name = factory->name();
            temp.toolTip = factory->toolTip();
            temp.icon = KisIconUtils::loadIcon(factory->iconName());
            temp.properties = 0;
            if (factory->family() == "funny") {
                funnyList.append(temp);
            } else if (factory->family() == "arrow") {
                arrowList.append(temp);
            } else if (factory->family() == "geometric") {
                geometricList.append(temp);
            } else {
                defaultList.append(temp);
            }

            if (quickShapes.contains(temp.id)) {
                quicklist.append(temp);
                quickCount++;
            }
        }
    }

    CollectionItemModel *model = new CollectionItemModel(this);
    model->setShapeTemplateList(defaultList);
    addCollection("default", i18n("Default"), model);

    model = new CollectionItemModel(this);
    model->setShapeTemplateList(geometricList);
    addCollection("geometric", i18n("Geometrics"), model);

    model = new CollectionItemModel(this);
    model->setShapeTemplateList(arrowList);
    addCollection("arrow", i18n("Arrows"), model);

    model = new CollectionItemModel(this);
    model->setShapeTemplateList(funnyList);
    addCollection("funny", i18n("Funny"), model);

    CollectionItemModel *quickModel = new CollectionItemModel(this);
    quickModel->setShapeTemplateList(quicklist);
    m_quickView->setModel(quickModel);

    int fw = m_quickView->frameWidth();
    m_quickView->setMaximumSize(QSize(quickCount * 40 + 2 * fw + 1, 44 + 2 * fw + 1));
    m_quickView->setMinimumSize(QSize(quickCount * 40 + 2 * fw + 1, 44 + 2 * fw + 1));

    m_collectionChooser->setMinimumSize(QSize(75 + 2 * fw, 0));
    m_collectionChooser->setMaximumSize(QSize(75 + 2 * fw, 1000));

    m_collectionChooser->setCurrentRow(0);
    activateShapeCollection(m_collectionChooser->item(0));
}

void ShapeCollectionDocker::activateShapeCollection(QListWidgetItem *item)
{
    QString id = item->data(Qt::UserRole).toString();

    if (m_modelMap.contains(id)) {
        m_collectionView->setModel(m_modelMap[id]);
    } else {
        qCritical() << "Didn't find a model with id ==" << id;
    }

}

bool ShapeCollectionDocker::addCollection(const QString &id, const QString &title, CollectionItemModel *model)
{
    if (m_modelMap.contains(id)) {
        return false;
    }

    m_modelMap.insert(id, model);
    QListWidgetItem *collectionChooserItem = new QListWidgetItem(koIcon("shape-choose"), title);
    collectionChooserItem->setData(Qt::UserRole, id);
    m_collectionChooser->addItem(collectionChooserItem);
    return true;
}


QIcon ShapeCollectionDocker::generateShapeIcon(KoShape *shape)
{
    KoZoomHandler converter;

    qreal diffx = 30 / converter.documentToViewX(shape->size().width());
    qreal diffy = 30 / converter.documentToViewY(shape->size().height());
    converter.setZoom(qMin(diffx, diffy));

    QPixmap pixmap(qRound(converter.documentToViewX(shape->size().width())) + 2, qRound(converter.documentToViewY(shape->size().height())) + 2);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(1, 1);
    KoShapePaintingContext paintContext; //FIXME
    shape->paint(painter, converter, paintContext);
    painter.end();

    return QIcon(pixmap);
}