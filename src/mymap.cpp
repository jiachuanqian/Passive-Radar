#include "mymap.h"
mymap::mymap(QWidget* parent) : QGraphicsView(parent)
{

	setMouseTracking(true);
	scene = new QGraphicsScene();

	QFile mapFile("map/maps.txt");
	if (mapFile.open(QIODevice::ReadOnly))
	{
		QTextStream ts(&mapFile);
		if (!ts.atEnd())
		{
			ts >> mapName >> x1 >> y1 >> x2 >> y2;
		}
	}
	else
	{
		qDebug("unable to read map txt\n");
	}
	pixmap = QPixmap(mapName);
	if (!pixmap.isNull()) {
		scene->addPixmap(pixmap);
		//qDebug() << "Map loaded successfully.";
	}
	else {
		qDebug() << "unable to load map\n";
	}

	scale(0.2, 0.2);
	setScene(scene);
	w = sceneRect().width();
	h = sceneRect().height();
	m_isTranslate = false;

	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void mymap::enterEvent(QEvent*)
{
	setCursor(Qt::CrossCursor);
}

void mymap::leaveEvent(QEvent*)
{
	setCursor(Qt::ArrowCursor);
}


void mymap::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		m_isTranslate = true;
		m_lastMousePos = ev->pos();
	}
	QGraphicsView::mousePressEvent(ev);

}

void mymap::mouseReleaseEvent(QMouseEvent* ev)
{

	if (ev->button() == Qt::LeftButton)
		m_isTranslate = false;
	QGraphicsView::mouseReleaseEvent(ev);
}

void mymap::mouseMoveEvent(QMouseEvent* ev)
{
	if (m_isTranslate)
	{
		QPointF mouseDelta = ev->pos() - m_lastMousePos;
		Translate(mouseDelta);
		setDragMode(QGraphicsView::ScrollHandDrag);
	}
	else
		setDragMode(QGraphicsView::NoDrag);
	m_lastMousePos = ev->pos();
	QPoint viewPoint = ev->pos();
	QPointF sp = mapToScene(viewPoint);
	qreal lon = y1 - ((sp.y()) * abs(y1 - y2) / h);
	qreal lat = x1 + ((sp.x()) * abs(x1 - x2) / w);
	QString Position = QString("position: (%1, %2)").arg(lon, 0, 'Q', 6).arg(lat, 0, 'Q', 6);
	emit SendMessageSignal(Position);
}
void mymap::wheelEvent(QWheelEvent* ev)
{
	QPoint scrollAmount = ev->angleDelta();
	scrollAmount.y() > 0 ? ZoomIn() : ZoomOut();
}

void mymap::ZoomIn()
{
	Zoom(1.1);
}

void mymap::ZoomOut()
{
	Zoom(0.9);
}

void mymap::Zoom(double scaleFactor)
{
	qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
	if (factor < 0.05 || factor > 20)
		return;

	scale(scaleFactor, scaleFactor);
}

void mymap::mouseDoubleClickEvent(QMouseEvent* ev)
{
	centerOn(w / 2, h / 2);
}

void mymap::Translate(QPointF delta)
{
	int ww = viewport()->rect().width();
	int hh = viewport()->rect().height();
	QPoint newCenter(ww / 2. - delta.x() + 0.5, hh / 2. - delta.y() + 0.5);
	centerOn(mapToScene(newCenter));
}

int mymap::GetItems()
{
	QList<QGraphicsItem*> allItems = scene->items();
	return allItems.size();
}

void mymap::ClearItems(int size)
{
	QList<QGraphicsItem*> allItems = scene->items();
	for (int i = 0; i < allItems.size() - size; ++i) {
		QGraphicsItem* item = allItems[i];
		scene->removeItem(item);
		delete item;
		//item->deleteLater();
	}
}
void mymap::PlotItem(double x, double y)
{
	QPixmap pixmap("Target.png");
	double width = pixmap.width();
	double height = pixmap.height();
	QGraphicsPixmapItem* itemR = new QGraphicsPixmapItem(pixmap);
	itemR->setPos((x - x1) * w / (abs(x1 - x2)), (y1 - y) * h / (abs(y1 - y2)));
	itemR->setOffset(-width / 2, -height / 2);
	scene->addItem(itemR);
	itemR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	QWidget::repaint();
}
void mymap::PlotTR2(double x, double y, double xx, double yy, double xxx, double yyy)
{
	QPixmap pixmap1("R.png");
	double width1 = pixmap1.width();
	double height1 = pixmap1.height();
	QGraphicsPixmapItem* itemR = new QGraphicsPixmapItem(pixmap1);
	itemR->setPos((x - x1) * w / (abs(x1 - x2)), (y1 - y) * h / (abs(y1 - y2)));
	scene->addItem(itemR);
	itemR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	itemR->setOffset(-width1 / 2, -height1 / 2);

	QPixmap pixmap2("T.png");
	double width2 = pixmap2.width();
	double height2 = pixmap2.height();
	QGraphicsPixmapItem* itemT = new QGraphicsPixmapItem(pixmap2);
	itemT->setPos((xx - x1) * w / (abs(x1 - x2)), (y1 - yy) * h / (abs(y1 - y2)));
	scene->addItem(itemT);
	itemT->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	itemT->setOffset(-width2 / 2, -height2 / 2);

	QPixmap pixmap3("T.png");
	double width3 = pixmap3.width();
	double height3 = pixmap3.height();
	QGraphicsPixmapItem* itemTT = new QGraphicsPixmapItem(pixmap3);
	itemTT->setPos((xxx - x1) * w / (abs(x1 - x2)), (y1 - yyy) * h / (abs(y1 - y2)));
	scene->addItem(itemTT);
	itemTT->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	itemTT->setOffset(-width3 / 2, -height3 / 2);

	QWidget::repaint();
}
void mymap::PlotTR(double x, double y, double xx, double yy)
{
	QPixmap pixmap1("R.png");
	double width1 = pixmap1.width();
	double height1 = pixmap1.height();
	QGraphicsPixmapItem* itemR = new QGraphicsPixmapItem(pixmap1);
	itemR->setPos((x - x1) * w / (abs(x1 - x2)), (y1 - y) * h / (abs(y1 - y2)));
	scene->addItem(itemR);
	itemR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	itemR->setOffset(-width1 / 2, -height1 / 2);

	QPixmap pixmap2("T.png");
	double width2 = pixmap1.width();
	double height2 = pixmap1.height();
	QGraphicsPixmapItem* itemT = new QGraphicsPixmapItem(pixmap2);
	itemT->setPos((xx - x1) * w / (abs(x1 - x2)), (y1 - yy) * h / (abs(y1 - y2)));
	scene->addItem(itemT);
	itemT->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	itemT->setOffset(-width2 / 2, -height2 / 2);

	QWidget::repaint();
}
