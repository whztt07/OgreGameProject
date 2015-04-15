#ifndef OGREQTEDITOR_H
#define OGREQTEDITOR_H

#include "ogreqteditor_global.h"
#include <QtGui>

class OGREQTEDITOR_EXPORT CQtEditor : public QMainWindow
{
	Q_OBJECT
public:
	CQtEditor(QWidget *parent, Qt::WFlags flags);
	~CQtEditor();
protected:
	virtual void CreateAction();
	virtual void CreateMenu();
	virtual void CreateToolBar();
	virtual void CreateStatusBar();

	QAction *m_pNewAct;
	QAction *m_pOpenAct;
	QAction *m_pSaveAct;

	QAction *m_pUndoAct;
	QAction *m_pRedoAct;
	QAction *m_pCopyAct;
	QAction *m_pCutAct;
	QAction *m_pPasteAct;

	QMenuBar *m_pMenuBar;
	QToolBar *m_pToolBar;
	QStatusBar *m_pStatusBar;
};

#endif // OGREQTEDITOR_H
