

#include "RenderWidget.h"
#include "ModelEditor.h"
//#include "OgreEdCamera.h"

CRenderWidget::CRenderWidget(QWidget *parent)
	:CD3D9Widget(parent)
{

}

CRenderWidget::~CRenderWidget()
{

}

void CRenderWidget::Render()
{
	CModelEditor::GetInstancePtr()->RenderEditor();
}

void CRenderWidget::Resize(QSize size)
{
	CModelEditor::GetInstancePtr()->ResetEditor(size.width(), size.height());
}

void CRenderWidget::MouseMove(QPoint pt)
{
	int x = pt.x() - m_ptPreMouse.x();
	int y = pt.y() - m_ptPreMouse.y();

	//Ogre::CCamera *pCamera = OgreEd::CEdCamera::GetInstancePtr()->GetCamera();
	//if (m_bRightBtnPressed)
	//{
	//	pCamera->Strafe(-x * 0.005f);
	//	pCamera->MoveUpward(y * 0.005f);
	//}
	//else if (m_bLeftBtnPressed)
	//{
	//}

	m_ptPreMouse = pt;
}
