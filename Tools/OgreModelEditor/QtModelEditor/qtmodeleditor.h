#ifndef QTMODELEDITOR_H
#define QTMODELEDITOR_H

#include <QtGui/QMainWindow>
#include "ui_qtmodeleditor.h"

#include "ogreqteditor.h"

class CRenderWidget;
class CModelEditor;

class QtModelEditor : public CQtEditor
{
	Q_OBJECT

public:
	QtModelEditor(QWidget *parent = 0, Qt::WFlags flags = 0);
	~QtModelEditor();

private:
	CModelEditor *m_pModelEditor;
	CRenderWidget *m_pRenderWidget;
};

#endif // QTMODELEDITOR_H
