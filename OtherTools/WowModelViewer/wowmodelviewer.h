#ifndef WOWMODELVIEWER_H
#define WOWMODELVIEWER_H

#include <QtGui/QMainWindow>
//#include "ui_wowmodelviewer.h"
#include "ui_AnimControl.h"
#include "ui_CharControl.h"
#include "ui_CategoryChoice.h"
#include <QDockWidget>
#include <QTreeWidget>
#include <QToolBox>
#include <vector>
#include "OgrePrerequisites.h"
#include "mpq_libmpq.h"
#include "ModelCanvas.h"
#include "CharControl.h"

class QTreeWidget;

class CAnimControlWidget : public QWidget
{
	Q_OBJECT
public:
	CAnimControlWidget(QWidget *parent = 0, Qt::WFlags flags = 0);
	~CAnimControlWidget();

	void Update(CModel *pModel);
	bool FillSkinSelector(TextureSet &skins);
	static std::string MakeSkinTexture(const char *texfn, const char *skin);
	void SetSkin(int num);
	
	Ui_WOWAnimControl myGUI;
	TextureSet m_setSkin;
private slots:
	void SelectAnim(int index);
	void SelectSkin(int index);
	void PlayClick(bool);
};

class CCharControlWidget : public QWidget
{
	Q_OBJECT
public:
	CCharControlWidget(QWidget *parent = 0, Qt::WFlags flags = 0);
	~CCharControlWidget();
	
	void UpdateModel(CModel *pModel);
	void RefreshModel(CModel *pModel);
	std::string MakeItemTexture(int region, const std::string name);
	void AddEquipment(int slot, int itemnum, int layer, CCharTexture &tex, bool lookup = true);
	void SelectItem(int type, int current, QString caption);
	bool CorrectType(int type, int slot);
	void UpdateItem(int type, int sel);
	void RefreshItem(int slot);
	
	Ui_CharControl myGUI;
	CCharDetails m_CharDetails;
	TabardDetails td;
	CModel *m_pModel;
	std::vector<int> m_vecNumber;
	int m_nChoosingSlot;
	int charTex;
	int hairTex;
	int furTex;
	int capeTex;
	int gobTex;

	bool bSheathe;
private slots:
	void SkinColorSliderMove(int);
	void FaceTypeSliderMove(int);
	void HairColorSliderMove(int);
	void HairStyleSliderMove(int);
	void FacialFeatureSliderMove(int);
	void FacialColorSliderMove(int);
	void HeadButtonClick();
	void ShoulderButtonClick();
	void RightHandButtonClick();
	void LeftHandButtonClick();
	void ShirtButtonClick();
	void ChestButtonClick();
	void CapeButtonClick();
};

class CCategoryChoiceWidget : public QWidget
{
	Q_OBJECT
public:
	CCategoryChoiceWidget(QWidget *parent = 0, Qt::WFlags flags = 0);
	~CCategoryChoiceWidget();

	Ui_CategoryChoice myGUI;
	CCharControlWidget *m_pCharCtrlWidget;
	int m_nType;
private slots:
	void ListChange(int index);
};

class CWowModelViewer : public QMainWindow
{
	Q_OBJECT
public:
	CWowModelViewer(QWidget *parent = 0, Qt::WFlags flags = 0);
	~CWowModelViewer();

	void LoadWOWModelResource();
private slots:
	void ResDirClickedEvent(QTreeWidgetItem*, int);
private:
	// message...
	void closeEvent(QCloseEvent *event);
	
	void TouchWOWModelEvent();
	void AddDockWindow(QMainWindow *parent);
	void ReadSetting(QString filename = "");
	void WriteSetting(QString filename = "");
	void LoadWOWSettings();
	void InitMPQArchives();
	void InitMPQDatBase();
	void LoadModel(QString path);

	QDockWidget *m_pResDirDock;
	QDockWidget *m_pAnimCtrlDock;
	QDockWidget *m_pCharCtrlDock;
	QTreeWidget *m_pWOWResDirTree;
	CD3D9Widget *m_pD3D9Widget;
	CAnimControlWidget *m_pAnimCtrlWidget;
	CCharControlWidget *m_pCharControlWidget;
	QStringList m_mpqArchives;
	
	typedef std::vector<CMPQArchive1*> MPQArchiveList;
	typedef std::set<CFileTreeItem> FileListVector;
	FileListVector m_setFilelist;
	MPQArchiveList m_vecMPQArchive;
	CModelCanvas *m_pModelCanvas;
	bool m_bInitDB;
};

#endif // WOWMODELVIEWER_H
