#ifndef MYMAP_H
#define MYMAP_H

#include <QtWidgets>
#include <QMouseEvent>
#include <QGraphicsView>
//#include <QGraphicsItem>


class mymap : public QGraphicsView
{
	Q_OBJECT
public:
	explicit mymap(QWidget* parent = 0);
	QGraphicsScene* scene;
	QString mapName;
	QPixmap pixmap;

	double x1{}, y1{}, x2{}, y2{};
	qreal w{}, h{};

	int GetItems();
	void ClearItems(int size = 1);
	void ZoomIn();
	void ZoomOut();
	void Zoom(double scaleFactor);
	void Translate(QPointF delta);
	
	void mouseDoubleClickEvent(QMouseEvent *ev);
	void enterEvent(QEvent*);
	void leaveEvent(QEvent*);
	void mousePressEvent(QMouseEvent* ev);
	void mouseReleaseEvent(QMouseEvent* ev);
	void mouseMoveEvent(QMouseEvent* ev);
	void wheelEvent(QWheelEvent* ev);
private:
	bool m_isTranslate;
	QPoint m_lastMousePos;
signals:
	void SendMessageSignal(QString Position);
	public slots :
	void PlotTR(double x, double y, double xx, double yy);
	void PlotTR2(double x, double y, double xx, double yy, double xxx, double yyy);
	void PlotItem(double x, double y);
};

#endif // MYMAP_H

