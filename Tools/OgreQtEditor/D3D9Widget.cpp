

#include "D3D9Widget.h"

CD3D9Widget::CD3D9Widget(QWidget *parent)
{
	QWidget::setAttribute(Qt::WA_PaintOnScreen); // 允许D3D绘制
	setFocusPolicy(Qt::WheelFocus); // 触发按键事件

	m_Timer.setInterval( 0 );
	QObject::connect( &m_Timer, SIGNAL( timeout() ), this, SLOT( Idle() ) );
	m_Timer.start();
}

CD3D9Widget::~CD3D9Widget()
{

}

void CD3D9Widget::Idle()
{
	Render();
}

void CD3D9Widget::resizeEvent( QResizeEvent *event )
{
	Resize(event->size());
}

void CD3D9Widget::mousePressEvent(QMouseEvent *event)
{
	m_ptPreMouse = event->pos();
	if (event->button() == Qt::LeftButton)
		m_bLeftBtnPressed = true;
	else if (event->button() == Qt::RightButton)
		m_bRightBtnPressed = true;
}

void CD3D9Widget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		m_bLeftBtnPressed = false;
	else if (event->button() == Qt::RightButton)
		m_bRightBtnPressed = false;
}

void CD3D9Widget::mouseMoveEvent(QMouseEvent *event)
{
	MouseMove(event->pos());
}