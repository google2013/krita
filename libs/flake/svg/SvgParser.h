/* This file is part of the KDE project
 * Copyright (C) 2002-2003,2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005-2006 Tim Beaulen <tbscope@gmail.com>
 * Copyright (C) 2005,2007-2009 Jan Hambrecht <jaham@gmx.net>
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

#ifndef SVGPARSER_H
#define SVGPARSER_H

#include <KoXmlReader.h>

#include <QMap>
#include <QSizeF>
#include <QRectF>
#include <QSharedPointer>

#include "kritaflake_export.h"
#include "SvgGradientHelper.h"
#include "SvgFilterHelper.h"
#include "SvgClipPathHelper.h"
#include "SvgLoadingContext.h"
#include "SvgStyleParser.h"
#include "KoClipMask.h"


class KoShape;
class KoShapeGroup;
class KoDocumentResourceManager;
class KoVectorPatternBackground;


class KRITAFLAKE_EXPORT SvgParser
{
public:
    explicit SvgParser(KoDocumentResourceManager *documentResourceManager);
    virtual ~SvgParser();

    /// Parses a svg fragment, returning the list of top level child shapes
    QList<KoShape*> parseSvg(const KoXmlElement &e, QSizeF * fragmentSize = 0);

    /// Sets the initial xml base directory (the directory form where the file is read)
    void setXmlBaseDir(const QString &baseDir);

    void setResolution(const QRectF boundsInPixels, qreal pixelsPerInch);

    /// Returns the list of all shapes of the svg document
    QList<KoShape*> shapes() const;

    typedef std::function<QByteArray(const QString&)> FileFetcherFunc;
    void setFileFetcher(FileFetcherFunc func);

protected:

    /// Parses a group-like element element, saving all its topmost properties
    KoShape* parseGroup(const KoXmlElement &e, const KoXmlElement &overrideChildrenFrom = KoXmlElement());
    /// Parses a container element, returning a list of child shapes
    QList<KoShape*> parseContainer(const KoXmlElement &);
    QList<KoShape*> parseSingleElement(const KoXmlElement &b);
    /// Parses a use element, returning a list of child shapes
    KoShape* parseUse(const KoXmlElement &);
    /// Parses a gradient element
    SvgGradientHelper *parseGradient(const KoXmlElement &);
    /// Parses a pattern element
    QSharedPointer<KoVectorPatternBackground> parsePattern(const KoXmlElement &e, const KoShape *__shape);
    /// Parses a filter element
    bool parseFilter(const KoXmlElement &, const KoXmlElement &referencedBy = KoXmlElement());
    /// Parses a clip path element
    bool parseClipPath(const KoXmlElement &);
    bool parseClipMask(const KoXmlElement &e);
    /// parses a length attribute
    qreal parseUnit(const QString &, bool horiz = false, bool vert = false, const QRectF &bbox = QRectF());
    /// parses a length attribute in x-direction
    qreal parseUnitX(const QString &unit);
    /// parses a length attribute in y-direction
    qreal parseUnitY(const QString &unit);
    /// parses a length attribute in xy-direction
    qreal parseUnitXY(const QString &unit);

    KoShape *createObjectDirect(const KoXmlElement &b);

    /// Creates an object from the given xml element
    KoShape * createObject(const KoXmlElement &, const SvgStyles &style = SvgStyles());
    /// Create path object from the given xml element
    KoShape * createPath(const KoXmlElement &);
    /// find gradient with given id in gradient map
    SvgGradientHelper* findGradient(const QString &id);
    /// find pattern with given id in pattern map
    QSharedPointer<KoVectorPatternBackground> findPattern(const QString &id, const KoShape *shape);
    /// find filter with given id in filter map
    SvgFilterHelper* findFilter(const QString &id, const QString &href = QString());
    /// find clip path with given id in clip path map
    SvgClipPathHelper* findClipPath(const QString &id);

    /// Adds list of shapes to the given group shape
    void addToGroup(QList<KoShape*> shapes, KoShapeGroup * group);

    /// creates a shape from the given shape id
    KoShape * createShape(const QString &shapeID);
    /// Creates shape from specified svg element
    KoShape * createShapeFromElement(const KoXmlElement &element, SvgLoadingContext &context);

    /// Builds the document from the given shapes list
    void buildDocument(QList<KoShape*> shapes);

    void uploadStyleToContext(const KoXmlElement &e);
    void applyCurrentStyle(KoShape *shape, const QPointF &shapeToOriginalUserCoordinates);

    /// Applies styles to the given shape
    void applyStyle(KoShape *, const KoXmlElement &, const QPointF &shapeToOriginalUserCoordinates);

    /// Applies styles to the given shape
    void applyStyle(KoShape *, const SvgStyles &, const QPointF &shapeToOriginalUserCoordinates);

    /// Applies the current fill style to the object
    void applyFillStyle(KoShape * shape);

    /// Applies the current stroke style to the object
    void applyStrokeStyle(KoShape * shape);

    /// Applies the current filter to the object
    void applyFilter(KoShape * shape);

    /// Applies the current clip path to the object
    void applyClipping(KoShape *shape, const QPointF &shapeToOriginalUserCoordinates);
    void applyMaskClipping(KoShape *shape, const QPointF &shapeToOriginalUserCoordinates);

    /// Applies id to specified shape
    void applyId(const QString &id, KoShape *shape);

    /// Applies viewBox transformation to the current graphical context
    /// NOTE: after applying the function currectBoundingBox can become null!
    void applyViewBoxTransform(const KoXmlElement &element);

private:
    QSizeF m_documentSize;
    SvgLoadingContext m_context;
    QMap<QString, SvgGradientHelper> m_gradients;
    QMap<QString, SvgFilterHelper> m_filters;
    QMap<QString, SvgClipPathHelper> m_clipPaths;
    QMap<QString, QSharedPointer<KoClipMask>> m_clipMasks;
    KoDocumentResourceManager *m_documentResourceManager;
    QList<KoShape*> m_shapes;
    QList<KoShape*> m_toplevelShapes;
};

#endif
