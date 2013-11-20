/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2006-2013 Calle Laakkonen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include <QDebug>

#include "canvasscene.h"
#include "canvasitem.h"
#include "statetracker.h"

#include "annotationitem.h"

#include "widgets/layerlistwidget.h"

#include "core/layerstack.h"
#include "core/layer.h"

#include "ora/orawriter.h"
#include "ora/orareader.h"

namespace drawingboard {

CanvasScene::CanvasScene(QObject *parent, widgets::LayerListWidget *layerlistwidget)
	: QGraphicsScene(parent), _image(0), _statetracker(0), _toolpreview(0), _showAnnotationBorders(false),
	_layerlistwidget(layerlistwidget)
{
	setItemIndexMethod(NoIndex);
}

CanvasScene::~CanvasScene()
{
	delete _image;
	delete _statetracker;
}

/**
 * This prepares the canvas for new drawing commands.
 */
void CanvasScene::initCanvas()
{
	delete _image;
	delete _statetracker;
	_image = new CanvasItem();
	_statetracker = new StateTracker(this, _image->image(), _layerlistwidget);
	
	addItem(_image);
	clearAnnotations();
	
	QList<QRectF> regions;
	regions.append(sceneRect());
	emit changed(regions);
#if 0
	previewstarted_ = false;
#endif
}

#if 0
bool CanvasScene::initBoard(const QString& file)
{
	using openraster::Reader;
	if(file.endsWith(".ora", Qt::CaseInsensitive)) {
		Reader reader = openraster::Reader(file);
		if(reader.load()==false) {
			qWarning() << "Unable to open" << file << "reason:" << reader.error();
			return false;
		}

		if(reader.warnings() != Reader::NO_WARNINGS) {
			QString text = tr("DrawPile does not support all the features used in this OpenRaster file. Saving this file may result in data loss.\n");
			if((reader.warnings() & Reader::ORA_EXTENDED))
				text += "\n- " + tr("Application specific extensions are used");
			if((reader.warnings() & Reader::ORA_NESTED))
				text += "\n- " + tr("Nested layers are not fully supported.");
			QMessageBox::warning(0, tr("Partially supported OpenRaster"), text);
		}

		// Image loaded, clear out the board
		dpcore::LayerStack *layers = reader.layers();
		setSceneRect(0,0,layers->width(), layers->height());
		delete _image;
		delete _statetracker;
		_image = new CanvasItem(layers);
		_statetracker = new StateTracker(_image);
		addItem(_image);

		// Add annotations (if present)
#if 0
		foreach(QString a, reader.annotations()) {
			AnnotationItem *item = new AnnotationItem(AnnotationItem::nextId(), _image);
			item->showBorder(hla_);
			// Parse the annotation message
			protocol::Message msg(a);
			item->setOptions(protocol::Annotation(msg.tokens()));
		}
#endif

		// Refresh UI
		QList<QRectF> regions;
		regions.append(sceneRect());
		emit changed(regions);
		previewstarted_ = false;
	} else {
		// Not an OpenRaster file, no need for complex loading
		QImage image;
        if(image.load(file)==false)
			return false;
		return initBoard(image);
	}
	return true;
}
#endif

#if 0
/**
 * @param zeroid if true, set the ID of each annotation to zero
 * @return list of ANNOTATE messages
 */
QStringList CanvasScene::getAnnotations(bool zeroid) const
{
	QStringList messages;

	if(_image)
		foreach(QGraphicsItem *item, _image->childItems())
			if(item->type() == AnnotationItem::Type) {
				protocol::Annotation a;
				static_cast<AnnotationItem*>(item)->getOptions(a);
				if(zeroid)
					a.id = 0;
				messages << protocol::Message::quote(a.tokens());
			}
	return messages;
}
#endif

void CanvasScene::clearAnnotations()
{
	foreach(QGraphicsItem *item, items()) {
		if(item->type() == AnnotationItem::Type) {
			emit annotationDeleted(static_cast<AnnotationItem*>(item)->id());
			delete item;
		}
	}
}

void CanvasScene::showAnnotations(bool show)
{
	foreach(QGraphicsItem *item, items()) {
		if(item->type() == AnnotationItem::Type)
			item->setVisible(show);
	}
}

void CanvasScene::showAnnotationBorders(bool hl)
{
	_showAnnotationBorders = hl;
	foreach(QGraphicsItem *item, items()) {
		if(item->type() == AnnotationItem::Type)
			static_cast<AnnotationItem*>(item)->setShowBorder(hl);
	}
}

void CanvasScene::unHilightAnnotation(int id)
{
	AnnotationItem *i = getAnnotationById(id);
	if(i)
		i->setHighlight(false);
}

bool CanvasScene::hasAnnotations() const
{
	foreach(const QGraphicsItem *i, items()) {
		if(i->type() == AnnotationItem::Type)
			return true;
	}
	return false;
}

AnnotationItem *CanvasScene::annotationAt(const QPoint &point)
{
	foreach(QGraphicsItem *i, items(point)) {
		if(i->type() == AnnotationItem::Type)
			return static_cast<AnnotationItem*>(i);
	}
	return 0;
}

AnnotationItem *CanvasScene::getAnnotationById(int id)
{
	foreach(QGraphicsItem *i, items()) {
		if(i->type() == AnnotationItem::Type) {
			AnnotationItem *item = static_cast<AnnotationItem*>(i);
			if(item->id() == id)
				return item;
		}
	}
	return 0;
}

void CanvasScene::deleteAnnotation(int id)
{
	AnnotationItem *a = getAnnotationById(id);
	if(a) {
		emit annotationDeleted(id);
		delete a;
	}
}

/**
 * @return board contents
 */
QImage CanvasScene::image() const
{
	if(_image)
		return _image->image()->toFlatImage();
	else
		return QImage();
}

void CanvasScene::pickColor(int x, int y)
{
	if(_image) {
		QColor color = _image->image()->colorAt(x, y);
		if(color.isValid())
			emit colorPicked(color);
	}
}


/**
 * The file format is determined from the name of the file
 * @param file file path
 * @return true on succcess
 */
bool CanvasScene::save(const QString& file) const
{
	if(file.endsWith(".ora", Qt::CaseInsensitive)) {
		// Special case: Save as OpenRaster with all the layers intact.
		openraster::Writer writer(_image->image());
#if 0
		writer.setAnnotations(getAnnotations(true));
#endif
		return writer.save(file);
	} else {
		// Regular image formats: flatten the image first.
		return image().save(file);
	}
}

/**
 * An image cannot be saved as a regular PNG without loss of information
 * if
 * <ul>
 * <li>It has more than one layer</li>
 * <li>It has annotations</li>
 * </ul>
 */
bool CanvasScene::needSaveOra() const
{
	return _image->image()->layers() > 1 ||
		hasAnnotations();
}

/**
 * @retval true if the board contains an image
 */
bool CanvasScene::hasImage() const {
	return _image!=0;
}

/**
 * @return board width
 */
int CanvasScene::width() const {
	if(_image)
		return _image->image()->width();
	else
		return -1;
}

/**
 * @return board height
 */
int CanvasScene::height() const {
	if(_image)
		return _image->image()->height();
	else
		return -1;
}

/**
 * @return layer stack
 */
dpcore::LayerStack *CanvasScene::layers()
{
	if(_image)
		return _image->image();
	return 0;
}

void CanvasScene::setToolPreview(QGraphicsItem *preview)
{
    delete _toolpreview;
    _toolpreview = preview;
	if(preview)
		addItem(preview);
}


#if 0
/**
 * Preview strokes are used to give immediate feedback to the user,
 * before the stroke info messages have completed their roundtrip
 * through the server.
 * @param point stroke point
 */
void CanvasScene::addPreview(const dpcore::Point& point)
{
	Q_ASSERT(localuser_ != -1);
	User *user = users_.value(localuser_);

	Preview *pre;
	if(previewcache_.isEmpty()) {
		pre = new StrokePreview(user->board());
	} else
		pre = previewcache_.dequeue();
	if(previewstarted_) {
		pre->preview(lastpreview_, point, brushsrc_->getBrush());
	} else {
		pre->preview(point, point, brushsrc_->getBrush());
		previewstarted_ = true;
	}
	lastpreview_ = point;
	previews_.enqueue(pre);
}


/**
 */
void CanvasScene::endPreview()
{
	previewstarted_ = false;
}

/**
 * This is called when leaving a session. All pending preview strokes
 * are immediately drawn on the board.
 */
void CanvasScene::commitPreviews()
{
#if 0
	dpcore::Point lastpoint(-1,-1,0);
	while(previews_.isEmpty()==false) {
		qreal distance;
		Preview *p = previews_.dequeue();
		if(p->from() != lastpoint) // TODO
			_image->drawPoint(0, p->from(), p->brush());
		else // TODO
			_image->drawLine(0, p->from(), p->to(), p->brush(), distance);
		lastpoint = p->to();
		delete p;
	}
#endif
	while(previewcache_.isEmpty()==false)
		delete previewcache_.dequeue();
}

/**
 * Remove all preview strokes from the board
 */
void CanvasScene::flushPreviews()
{
	while(previews_.isEmpty()==false) {
		Preview *p = previews_.dequeue();
		p->hidePreview();
		previewcache_.enqueue(p);
	}
}
#endif

void CanvasScene::handleDrawingCommand(protocol::MessagePtr cmd)
{
	if(_statetracker) {
		_statetracker->receiveCommand(cmd);
		emit canvasModified();
	} else {
		qWarning() << "Received a drawing command but canvas does not exist!";
	}
}

}

