

#include "qtmodeleditor.h"
#include "RenderWidget.h"
#include "ModelEditor.h"

QtModelEditor::QtModelEditor(QWidget *parent, Qt::WFlags flags)
	: CQtEditor(parent, flags)
{
	setWindowIcon(QIcon(":/Resources/qtOgitor.png"));
	m_pModelEditor = new CModelEditor;
	
	m_pRenderWidget = new CRenderWidget(this);
	setCentralWidget(m_pRenderWidget);

	m_pModelEditor->Create(m_pRenderWidget->winId());

	QMainWindow::setWindowState(Qt::WindowMaximized);
}

QtModelEditor::~QtModelEditor()
{

}
