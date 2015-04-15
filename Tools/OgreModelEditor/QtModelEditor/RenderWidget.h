

#ifndef __RenderWidget_h__
#define __RenderWidget_h__

#include "D3D9Widget.h"

class CRenderWidget : public CD3D9Widget
{
	Q_OBJECT
public:
	CRenderWidget(QWidget *parent);
	~CRenderWidget();

	virtual void Render();
	virtual void Resize(QSize size);
	virtual void MouseMove(QPoint pt);
};

#endif