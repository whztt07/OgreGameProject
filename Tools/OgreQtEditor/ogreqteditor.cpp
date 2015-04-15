#include "ogreqteditor.h"

CQtEditor::CQtEditor(QWidget *parent, Qt::WFlags flags)
:QMainWindow(parent, flags)
{
	m_pMenuBar = new QMenuBar;
	m_pToolBar = new QToolBar;
	m_pStatusBar = new QStatusBar;
	CreateAction();
	CreateMenu();
	CreateToolBar();
	CreateStatusBar();
}

CQtEditor::~CQtEditor()
{

}

void CQtEditor::CreateAction()
{
	m_pNewAct = new QAction("New", this);
	m_pNewAct->setShortcut(QKeySequence::New);
	m_pNewAct->setIcon(QIcon(":/Resources/filenew.png"));

	m_pOpenAct = new QAction("Open", this);
	m_pOpenAct->setShortcut(QKeySequence::Open);
	m_pOpenAct->setIcon(QIcon(":/Resources/fileopen.png"));

	m_pSaveAct = new QAction("Save", this);
	m_pSaveAct->setShortcut(QKeySequence::Save);
	m_pSaveAct->setIcon(QIcon(":/Resources/filesave.png"));

	m_pUndoAct = new QAction("Undo", this);
	m_pUndoAct->setShortcut(QKeySequence::Undo);
	m_pUndoAct->setIcon(QIcon(":/Resources/undo.png"));

	m_pRedoAct = new QAction("Redo", this);
	m_pRedoAct->setShortcut(QKeySequence::Redo);
	m_pRedoAct->setIcon(QIcon(":/Resources/redo.png"));

	m_pCopyAct = new QAction("Copy", this);
	m_pCopyAct->setShortcut(QKeySequence::Copy);
	m_pCopyAct->setIcon(QIcon(":/Resources/copy.png"));

	m_pCutAct = new QAction("Cut", this);
	m_pCutAct->setShortcut(QKeySequence::Cut);
	m_pCutAct->setIcon(QIcon(":/Resources/cut.png"));

	m_pPasteAct = new QAction("Paste", this);
	m_pPasteAct->setShortcut(QKeySequence::Paste);
	m_pPasteAct->setIcon(QIcon(":/Resources/paste.png"));
}

void CQtEditor::CreateMenu()
{
	QMenu *file = new QMenu("File");
	file->addAction(m_pNewAct);
	file->addAction(m_pOpenAct);
	file->addAction(m_pSaveAct);

	QMenu *edit = new QMenu("Edit");
	edit->addAction(m_pUndoAct);
	edit->addAction(m_pRedoAct);
	edit->addSeparator();
	edit->addAction(m_pCopyAct);
	edit->addAction(m_pCutAct);
	edit->addAction(m_pPasteAct);

	m_pMenuBar->addMenu(file);
	m_pMenuBar->addMenu(edit);

	this->setMenuBar(m_pMenuBar);
}

void CQtEditor::CreateToolBar()
{
	m_pToolBar->addAction(m_pNewAct);
	m_pToolBar->addAction(m_pOpenAct);
	m_pToolBar->addAction(m_pSaveAct);
	m_pToolBar->addSeparator();
	m_pToolBar->addAction(m_pUndoAct);
	m_pToolBar->addAction(m_pRedoAct);
	m_pToolBar->addAction(m_pCopyAct);
	m_pToolBar->addAction(m_pCutAct);
	m_pToolBar->addAction(m_pPasteAct);
	m_pToolBar->setMovable(false);

	this->addToolBar(m_pToolBar);
};

void CQtEditor::CreateStatusBar()
{
	this->setStatusBar(m_pStatusBar);
}
