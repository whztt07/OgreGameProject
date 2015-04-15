

#ifndef __D3D9Widget_h__
#define __D3D9Widget_h__

#include "ogreqteditor_global.h"
#include <QtGui>

class OGREQTEDITOR_EXPORT CD3D9Widget : public QWidget
{
	Q_OBJECT
public:
	CD3D9Widget(QWidget *parent);
	virtual ~CD3D9Widget();
public slots:
	void Idle();
protected:
	QPaintEngine *paintEngine() const { return 0; } // ‘ –ÌD3DªÊ÷∆

	virtual void resizeEvent( QResizeEvent *event );
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	
	virtual void Render() = 0;
	virtual void Resize(QSize size) = 0;
	virtual void MouseMove(QPoint pt) = 0;

	QTimer m_Timer;
	
	bool m_bLeftBtnPressed;
	bool m_bRightBtnPressed;
	QPoint m_ptPreMouse;
};

#endif