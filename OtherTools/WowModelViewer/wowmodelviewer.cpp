
#include "wowmodelviewer.h"
#include "modelexport.h"
#include "ModelCanvas.h"
#include <QSettings>
#include <QFile>
#include <QFileDialog>
#include "database.h"

CModel *g_pSelModel = NULL;
extern CMpqTextureManager g_TextureManager;
CCategoryChoiceWidget *g_pCategoryChoiceWidget;
CAttachment *m_pCharAtt = NULL;

//--------------------------------------
//---------------Common-----------------
//--------------------------------------
int slotOrderWithRobe[] = 
{
	CS_SHIRT,
	CS_HEAD,
	CS_NECK,
	CS_SHOULDER,
	CS_BOOTS,
	CS_PANTS,
	CS_BRACERS,
	CS_CHEST,
	CS_GLOVES,
	CS_TABARD,
	CS_BELT,
	CS_HAND_RIGHT,
	CS_HAND_LEFT,
	CS_CAPE,
	CS_QUIVER
};

int slotOrder[] =
{	
	CS_SHIRT,
	CS_HEAD,
	CS_NECK,
	CS_SHOULDER,
	CS_PANTS,
	CS_BOOTS,
	CS_CHEST,
	CS_TABARD,
	CS_BELT,
	CS_BRACERS,
	CS_GLOVES,
	CS_HAND_RIGHT,
	CS_HAND_LEFT,
	CS_CAPE,
	CS_QUIVER
};

const char* regionPaths[NUM_REGIONS] =
{
	"",
	"Item\\TextureComponents\\ArmUpperTexture\\",
	"Item\\TextureComponents\\ArmLowerTexture\\",
	"Item\\TextureComponents\\HandTexture\\",
	"",
	"",
	"Item\\TextureComponents\\TorsoUpperTexture\\",
	"Item\\TextureComponents\\TorsoLowerTexture\\",
	"Item\\TextureComponents\\LegUpperTexture\\",
	"Item\\TextureComponents\\LegLowerTexture\\",
	"Item\\TextureComponents\\FootTexture\\"
};

bool SlotHasModel(int i)
{
	return (i==CS_HEAD || i==CS_SHOULDER || i==CS_HAND_LEFT || i==CS_HAND_RIGHT || i==CS_QUIVER);
}

//---------------------------------------
//------------CWowModelViewer------------
//---------------------------------------
CWowModelViewer::CWowModelViewer(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	// icon...
	QIcon icon;
	icon.addPixmap(QPixmap(":/Resources/qtOgitor.png"), QIcon::Normal, QIcon::Off);
	QMainWindow::setWindowIcon(icon);

	// 默认尺寸...
	QMainWindow::resize(800, 600);
	
	//...
	AddDockWindow(this);
	TouchWOWModelEvent();

	g_pCategoryChoiceWidget = new CCategoryChoiceWidget;
	
	m_pD3D9Widget = new CD3D9Widget(this);
	m_pD3D9Widget->setObjectName("D3D9Widget");

	QMainWindow::setCentralWidget(m_pD3D9Widget);

	// 读设置...
	ReadSetting();

	m_pD3D9Widget->InitD3D9Device();
}

CWowModelViewer::~CWowModelViewer()
{
}

void CWowModelViewer::AddDockWindow(QMainWindow *parent)
{
	m_pWOWResDirTree = new QTreeWidget(parent);
	m_pWOWResDirTree->setHeaderLabel(tr("WOW Res Dir"));

	m_pResDirDock = new QDockWidget(parent);
	m_pResDirDock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
	m_pResDirDock->setObjectName(QString::fromUtf8("ResDirDock"));
	m_pResDirDock->setWidget(m_pWOWResDirTree);
	parent->addDockWidget(static_cast<Qt::DockWidgetArea>(1), m_pResDirDock);

	m_pAnimCtrlWidget = new CAnimControlWidget(parent);
	m_pAnimCtrlDock = new QDockWidget(parent);
	m_pAnimCtrlDock->setAllowedAreas(Qt::BottomDockWidgetArea);
	m_pAnimCtrlDock->setObjectName(QString::fromUtf8("AnimControlDock"));
	m_pAnimCtrlDock->setWidget(m_pAnimCtrlWidget);
	parent->addDockWidget(static_cast<Qt::DockWidgetArea>(1), m_pAnimCtrlDock);
	
	m_pCharControlWidget = new CCharControlWidget(parent);
	m_pCharCtrlDock = new QDockWidget(parent);
	m_pCharCtrlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	m_pCharCtrlDock->setObjectName(QString::fromUtf8("CharControlDock"));
	m_pCharCtrlDock->setWidget(m_pCharControlWidget);
	parent->addDockWidget(static_cast<Qt::DockWidgetArea>(1), m_pCharCtrlDock);

	m_pResDirDock->setWindowTitle(QApplication::translate("MainWindow", "ResDir", 0, QApplication::UnicodeUTF8));
	m_pCharCtrlDock->setWindowTitle(QApplication::translate("MainWindow", "CharCtrl", 0, QApplication::UnicodeUTF8));

	parent->tabifyDockWidget(m_pResDirDock, m_pCharCtrlDock);
	m_pResDirDock->raise();
}

void CWowModelViewer::ReadSetting(QString filename)
{
	// ini文件格式...
	QSettings settings("WowModelViewerCfg", QSettings::IniFormat);

	settings.beginGroup("session");
	QMainWindow::restoreGeometry(settings.value("Size").toByteArray());
	bool bRestore = QMainWindow::restoreState(settings.value("Layout").toByteArray());
	settings.endGroup();

	QMainWindow::setWindowState(Qt::WindowMaximized);
}

void CWowModelViewer::WriteSetting(QString filename)
{
	QSettings settings("WowModelViewerCfg", QSettings::IniFormat);

	settings.beginGroup("session");
	settings.setValue("Size", QMainWindow::saveGeometry());
	settings.setValue("Layout", QMainWindow::saveState());
	settings.endGroup();
}

void CWowModelViewer::closeEvent(QCloseEvent *event)
{
	WriteSetting();
}

void CWowModelViewer::LoadWOWModelResource()
{
	m_pModelCanvas = new CModelCanvas();
	if (m_pModelCanvas == NULL)
	{
		qCritical("CWowModelViewer LoadWOWModelResource new CModelCanvas");
	}

	m_pD3D9Widget->SetModelCanvas(m_pModelCanvas);

	// 加载WOW的设置...
	LoadWOWSettings();

	InitMPQArchives();

	InitMPQDatBase();
}

void CWowModelViewer::InitMPQDatBase()
{
	qDebug("Initiating Databases...");
	m_bInitDB = true;

	if (!animdb.open()) 
	{
		m_bInitDB = false;
		qCritical("Error: Could not open the Animation DB.");
	}

	if (!modeldb.open()) 
	{
		m_bInitDB = false;
		qCritical("Error: Could not open the Creatures DB.");
	}

	if (!skindb.open()) 
	{
		m_bInitDB = false;
		qCritical("Error: Could not open the CreatureDisplayInfo DB.");
	}

	if(!hairdb.open()) 
	{
		m_bInitDB = false;
		qCritical("Error: Could not open the Hair Geoset DB.");
	}

	if(!chardb.open())
	{
		m_bInitDB = false;
		qCritical("Error: Could not open the Character DB.");
	}

	if(!racedb.open())
	{
		m_bInitDB = false;
		qCritical("Error: Could not open the Char Races DB.");
	}

	if(!classdb.open())
	{
		m_bInitDB = false;
		qCritical("Error: Could not open the Char Classes DB.");
	}

	if(!facialhairdb.open()) 
	{
		m_bInitDB = false;
		qCritical("Error: Could not open the Char Facial Hair DB.");
	}

	if(!visualdb.open())
		qCritical("Error: Could not open the ItemVisuals DB.");

	if(!effectdb.open())
		qCritical("Error: Could not open the ItemVisualEffects DB.");

	if(!subclassdb.open())
		qCritical("Error: Could not open the Item Subclasses DB.");
	
	if(!startdb.open())
		qCritical("Error: Could not open the Start Outfit Sets DB.");

	//if(!helmetdb.open()) return false;
	if(!npcdb.open()) 
		qCritical("Error: Could not open the Start Outfit NPC DB.");

	if(!npctypedb.open())
		qCritical("Error: Could not open the Creature Type DB.");

	if (QFile::exists("items.csv"))
		items.open("items.csv");
	else
		qCritical("Error: Could not find items.csv to load an item list from.");

	if(!itemdb.open())
		qCritical("Error: Could not open the ItemDisplayInfo DB.");
	else
		items.cleanup(itemdb);

	if(!setsdb.open())
		qCritical("Error: Could not open the Item Sets DB.");
	else
		setsdb.cleanup(items);

	if(QFile::exists("npcs.csv"))
		npcs.open("npcs.csv");
	else
		qCritical("Error: Could not find npcs.csv, unable to create NPC list.");

	//if(spelleffectsdb.open())
	//	GetSpellEffects();
	//else
	//	qCritical("Error: Could not open the SpellVisualEffects DB.");

	qDebug("Finished initiating database files.\n");
}

bool FilterModels(std::string s)
{
	//s.LowerCase();
	const size_t len = s.length();
	if (len < 4) 
		return false;

	return ( 
		((s[len-2]|0x20)=='m' && s[len-1]=='2') ||
		((s[len-3]|0x20)=='w' && (s[len-2]|0x20)=='m' && (s[len-1]|0x20)=='o' ) 
		);
}

void CWowModelViewer::InitMPQArchives()
{
	for (size_t i = 0; i < m_mpqArchives.size(); i++)
	{
		if (QFile::exists(m_mpqArchives[i]))
			m_vecMPQArchive.push_back(new CMPQArchive1(m_mpqArchives[i].toAscii()));
	}

	GetFileLists(m_setFilelist, FilterModels);

	// ------------------------------
	QFont font(QString(tr("方正舒体")), 25);
	m_pWOWResDirTree->setFont(font);

	struct stCompTreeItem
	{
		std::string name;
		QTreeWidgetItem *item;
	};
	std::vector<stCompTreeItem> compTreeItem;

	QStringList rootName;
	rootName.push_back( QString(tr("WOW模型列表")) );
	QTreeWidgetItem *pRoot = new QTreeWidgetItem(m_pWOWResDirTree, rootName);
	stCompTreeItem compItem;
	compItem.name = "";
	compItem.item = pRoot;
	compTreeItem.push_back(compItem);

	FileListVector::iterator iterFile = m_setFilelist.begin();
	for (; iterFile != m_setFilelist.end(); iterFile++)
	{
		const std::string &str = iterFile->fn;
		size_t p = 0;

		// find the matching place in the stack
		int nCompTreeItem = 1;
		for (; nCompTreeItem < compTreeItem.size(); nCompTreeItem++) 
		{
			std::string &comp = compTreeItem[nCompTreeItem].name;
			bool match = true;
			for (unsigned int j = 0; j < comp.length(); j++) 
			{
				if (comp[j] != str[p + j])
				{
					match = false;
					break;
				}
			}

			if (match)
			{
				match &= (str[p + comp.length()] == '\\' ||
					str[p + comp.length()] == '/' );
			}

			if (!match)
				break;

			p += (comp.length() + 1);
		}

		// delete the extra parts off the end of the stack
		size_t numtopop = compTreeItem.size() - nCompTreeItem;
		for (size_t k = 0; k < numtopop; k++) 
		{
			compTreeItem.pop_back();
		}

		// -------------------------------------
		size_t start = p;
		for (; p < str.length(); p++) 
		{
			if (str[p]=='\\' || str[p] == '/')
			{
				std::string strItem = str.substr(start, p - start);
				start = p + 1;

				QStringList list;
				list.push_back( QString(strItem.c_str()) );
				QTreeWidgetItem *pItem = new QTreeWidgetItem(compTreeItem[compTreeItem.size() - 1].item, list);
				
				if (iterFile->col == 0)
					pItem->setTextColor(0, QColor(0, 0, 0, 255));
				else if (iterFile->col == 1)
					pItem->setTextColor(0, QColor(0, 0, 255, 255));
				else if (iterFile->col == 2)
					pItem->setTextColor(0, QColor(255, 0, 0, 255));
				else if (iterFile->col == 3)
					pItem->setTextColor(0, QColor(160, 0, 160, 255));

				compItem.name = list[0].toStdString();
				compItem.item = pItem;
				compTreeItem.push_back(compItem);
			}
		}

		std::string strEnd = str.substr(start);
		QStringList list;
		list.push_back( QString(strEnd.c_str()) );
		QTreeWidgetItem *pItem = new QTreeWidgetItem(compTreeItem[compTreeItem.size() - 1].item, list);
		pItem->setData(0, 1, QVariant(str.c_str()));

		if (iterFile->col == 0)
			pItem->setTextColor(0, QColor(0, 0, 0, 255));
		else if (iterFile->col == 1)
			pItem->setTextColor(0, QColor(0, 0, 255, 255));
		else if (iterFile->col == 2)
			pItem->setTextColor(0, QColor(255, 0, 0, 255));
		else if (iterFile->col == 3)
			pItem->setTextColor(0, QColor(160, 0, 160, 255));
	}

	m_pWOWResDirTree->addTopLevelItem(pRoot);
	pRoot->setExpanded(true); // 根节点展开
}

void CWowModelViewer::LoadWOWSettings()
{
	QSettings configFileRead("WOWModelConfig.ini", QSettings::IniFormat);

	QString path = configFileRead.value("Settings//Path").toString();
	
	QString mpqFile1 = configFileRead.value("Settings//MPQFiles1").toString();
	QString mpqFile2 = configFileRead.value("Settings//MPQFiles2").toString();
	QString mpqFile3 = configFileRead.value("Settings//MPQFiles3").toString();
	QString mpqFile4 = configFileRead.value("Settings//MPQFiles4").toString();
	QString mpqFile5 = configFileRead.value("Settings//MPQFiles5").toString();
	QString mpqFile6 = configFileRead.value("Settings//MPQFiles6").toString();
	QString mpqFile7 = configFileRead.value("Settings//MPQFiles7").toString();
	QString mpqFile8 = configFileRead.value("Settings//MPQFiles8").toString();
	QString mpqFile9 = configFileRead.value("Settings//MPQFiles9").toString();
	m_mpqArchives.push_back(mpqFile1);
	m_mpqArchives.push_back(mpqFile2);
	m_mpqArchives.push_back(mpqFile3);
	m_mpqArchives.push_back(mpqFile4);
	m_mpqArchives.push_back(mpqFile5);
	m_mpqArchives.push_back(mpqFile6);
	m_mpqArchives.push_back(mpqFile7);
	m_mpqArchives.push_back(mpqFile8);
	m_mpqArchives.push_back(mpqFile9);
}

void CWowModelViewer::LoadModel(QString path)
{
	QString left = path.left(4);
	bool bChar = left.contains(QString("Char"), Qt::CaseInsensitive);
	m_pCharAtt = NULL;

	if (bChar)
	{
		m_pCharAtt = m_pModelCanvas->LoadCharModel(path);
	}
	else
	{
		m_pModelCanvas->LoadModel(path);
	}

	g_pSelModel = m_pModelCanvas->m_pModel;
	g_pSelModel->m_bChar = bChar;
	g_pSelModel->m_nModelType = MT_NORMAL;
	
	if (bChar)
	{
		g_pSelModel->m_nModelType = MT_CHAR;
		m_pCharControlWidget->UpdateModel(g_pSelModel);
	}
	
	m_pAnimCtrlWidget->Update(g_pSelModel);
}

//---------------------------------------
//---------CCharControlWidget------------
//---------------------------------------
CCharControlWidget::CCharControlWidget(QWidget *parent, Qt::WFlags flags)
{
	myGUI.setupUi(this);

	connect(myGUI.m_pSkinColorSlider, SIGNAL(valueChanged(int)), this, SLOT(SkinColorSliderMove(int)) );
	connect(myGUI.m_pFaceTypeSlider, SIGNAL(valueChanged(int)), this, SLOT(FaceTypeSliderMove(int)) );
	connect(myGUI.m_pHairColorSlider, SIGNAL(valueChanged(int)), this, SLOT(HairColorSliderMove(int)) );
	connect(myGUI.m_pHairStyleSlider, SIGNAL(valueChanged(int)), this, SLOT(HairStyleSliderMove(int)) );
	connect(myGUI.m_pFacialFeatureSlider, SIGNAL(valueChanged(int)), this, SLOT(FacialFeatureSliderMove(int)) );
	connect(myGUI.m_pFacialColorSlider, SIGNAL(valueChanged(int)), this, SLOT(FacialColorSliderMove(int)) );

	connect(myGUI.m_pHeadBtn, SIGNAL(clicked()), this, SLOT(HeadButtonClick()));
	connect(myGUI.m_pShoulderBtn, SIGNAL(clicked()), this, SLOT(ShoulderButtonClick()));
	connect(myGUI.m_pRightHandBtn, SIGNAL(clicked()), this, SLOT(RightHandButtonClick()));
	connect(myGUI.m_pLeftHandBtn, SIGNAL(clicked()), this, SLOT(LeftHandButtonClick()));
	connect(myGUI.m_pShirtBtn, SIGNAL(clicked()), this, SLOT(ShirtButtonClick()));
	connect(myGUI.m_pChestBtn, SIGNAL(clicked()), this, SLOT(ChestButtonClick()));
	connect(myGUI.m_pCapeBtn, SIGNAL(clicked()), this, SLOT(CapeButtonClick()));

	m_nChoosingSlot = -1;
	charTex = -1;
	hairTex = -1;
	furTex = -1;
	capeTex = -1;
	gobTex = -1;
	m_pModel = NULL;
	bSheathe = false;
}

CCharControlWidget::~CCharControlWidget()
{

}

void CCharControlWidget::UpdateModel(CModel *pModel)
{
	m_CharDetails.Reset();
	m_pModel = pModel;

	// hide most geosets
	for (size_t i = 0; i < pModel->m_vecGeoset.size(); i++)
	{
		int id = pModel->m_vecGeoset[i].id;
		pModel->m_pbShowGeosets[i] = (id==0);
	}

	size_t p1 = pModel->m_strName.find_first_of('\\', 0);
	size_t p2 = pModel->m_strName.find_first_of('\\', p1+1);
	size_t p3 = pModel->m_strName.find_first_of('\\', p2+1);

	std::string raceName = pModel->m_strName.substr(p1+1,p2-p1-1);
	std::string genderName = pModel->m_strName.substr(p2+1,p3-p2-1);

	unsigned int race, gender;

	try 
	{
		// Okay for some reason Blizzard have removed the full racial names
		// out of the ChrRaces.dbc.  Going to have to hardcode the values.
		CharRacesDB::Record raceRec = racedb.getByName(raceName.c_str());
		race = raceRec.getUInt(CharRacesDB::RaceID);

		gender = (genderName == "female" || genderName == "Female" || genderName == "FEMALE") ? 1 : 0;
	} 
	catch (CharRacesDB::NotFound) 
	{
		race = 0;
		gender = 0;
	}

	// Enable the use of NPC skins if its  a goblin.
	if (race==9)
		m_CharDetails.useNPC=1;
	else
		m_CharDetails.useNPC=0;

	if (race==6 || race==8 || race==11 || race==13 || race==14) // If its a troll/tauren/dranei/naga/broken, show the feet (dont wear boots)
		m_CharDetails.showFeet = true;
	else
		m_CharDetails.showFeet = false;

	// get max values
	m_CharDetails.maxSkinColor = chardb.getColorsFor(race, gender, CharSectionsDB::SkinType, 0, m_CharDetails.useNPC);
	if (m_CharDetails.maxSkinColor==0 && m_CharDetails.useNPC==1) 
	{
		qCritical("The selected character does not have any NPC skins!\nSwitching back to normal character skins.");
		m_CharDetails.useNPC = 0;
		m_CharDetails.maxSkinColor = chardb.getColorsFor(race, gender, CharSectionsDB::SkinType, 0, m_CharDetails.useNPC);
	}

	m_CharDetails.maxFaceType  = chardb.getSectionsFor(race, gender, CharSectionsDB::FaceType, 0, m_CharDetails.useNPC);
	m_CharDetails.maxHairColor = chardb.getColorsFor(race, gender, CharSectionsDB::HairType, 0, 0);
	m_CharDetails.maxFacialHair = facialhairdb.getStylesFor(race, gender);
	m_CharDetails.maxFacialColor = m_CharDetails.maxHairColor;

	m_CharDetails.race = race;
	m_CharDetails.gender = gender;
	
	std::set<int> styles;
	for (CharHairGeosetsDB::Iterator it = hairdb.begin(); it != hairdb.end(); ++it)
	{
		if (it->getUInt(CharHairGeosetsDB::Race)==race && it->getUInt(CharHairGeosetsDB::Gender)==gender)
		{
			styles.insert(it->getUInt(CharHairGeosetsDB::Section));
		}
	}
	m_CharDetails.maxHairStyle = (int)styles.size();

	if (m_CharDetails.maxFaceType==0) m_CharDetails.maxFaceType = 1;
	if (m_CharDetails.maxSkinColor==0) m_CharDetails.maxSkinColor = 1;
	if (m_CharDetails.maxHairColor==0) m_CharDetails.maxHairColor = 1;
	if (m_CharDetails.maxHairStyle==0) m_CharDetails.maxHairStyle = 1;
	if (m_CharDetails.maxFacialHair==0) m_CharDetails.maxFacialHair = 1;
	myGUI.m_pSkinColorSlider->setRange(0, m_CharDetails.maxSkinColor - 1);
	myGUI.m_pFaceTypeSlider->setRange(0, m_CharDetails.maxFaceType - 1);
	myGUI.m_pHairColorSlider->setRange(0, m_CharDetails.maxHairColor - 1);
	myGUI.m_pHairStyleSlider->setRange(0, m_CharDetails.maxHairStyle - 1);
	myGUI.m_pFacialFeatureSlider->setRange(0, m_CharDetails.maxFacialHair - 1);
	myGUI.m_pFacialColorSlider->setRange(0, m_CharDetails.maxHairColor - 1);

	td.Icon = 0;
	td.IconColor = 0;
	td.Border = 0;
	td.BorderColor = 0;
	td.Background = 0;

	myGUI.m_pSkinColorSlider->setValue(m_CharDetails.skinColor);
	myGUI.m_pFaceTypeSlider->setValue(m_CharDetails.faceType);
	myGUI.m_pHairColorSlider->setValue(m_CharDetails.hairColor);
	myGUI.m_pHairStyleSlider->setValue(m_CharDetails.hairStyle);
	myGUI.m_pFacialFeatureSlider->setValue(m_CharDetails.facialHair);
	myGUI.m_pFacialColorSlider->setValue(m_CharDetails.facialColor);

	myGUI.m_pSkinColorLabel->setText( QString("%1").arg(myGUI.m_pSkinColorSlider->value()) );
	myGUI.m_pFaceTypeLabel->setText( QString("%1").arg(myGUI.m_pFaceTypeSlider->value()) );
	myGUI.m_pHairColorLabel->setText( QString("%1").arg(myGUI.m_pHairColorSlider->value()) );
	myGUI.m_pHairStyleLabel->setText( QString("%1").arg(myGUI.m_pHairStyleSlider->value()) );
	myGUI.m_pFacialFeatureLabel->setText( QString("%1").arg(myGUI.m_pFacialFeatureSlider->value()) );
	myGUI.m_pFacialColorLabel->setText( QString("%1").arg(myGUI.m_pFacialColorSlider->value()) );

	RefreshModel(pModel);
}

std::string CCharControlWidget::MakeItemTexture(int region, const std::string name)
{
	// just return an empty filename
	if (name.length() < 3) 
		return "";

	char leggings = name[name.length() - 2];

	// try prefered version first
	std::string fn = regionPaths[region];
	fn += name;
	fn += "_";

	if (leggings == 'l' || leggings == 'L')
		fn += "U";
	else
		fn += m_CharDetails.gender ? "F" : "M";

	fn += ".blp";
	if (CMPQFile::getSize(fn.c_str()) > 0)  //MPQFile::exists(fn.c_str()) && 
		return fn;

	if (fn.length() < 5)
		return "";

	// if that failed try alternate texture version
	if (leggings == 'l' || leggings == 'L')
		fn[fn.length()-5] = m_CharDetails.gender ? 'F' : 'M';
	else
		fn[fn.length()-5] = 'U';

	if (CMPQFile::getSize(fn.c_str()) > 0) //MPQFile::exists(fn.c_str()) && 
		return fn;

	fn = regionPaths[region];
	fn += name;	
	fn += ".blp";

	// return the default name, nothing else could be found.
	return fn;
}

void CCharControlWidget::AddEquipment(int slot, int itemnum, int layer, CCharTexture &tex, bool lookup)
{
	if (slot==CS_PANTS && m_CharDetails.geosets[13]==2) 
		return; // if we are wearing a robe, no pants for us! ^_^

	try 
	{
		const ItemRecord &item = items.get(itemnum);
		int type = item.type;
		int itemID = 0;

		if (lookup)
			itemID = item.model;
		else
			itemID = itemnum;

		ItemDisplayDB::Record r = itemdb.getById(itemID);
		
		// Just a rough check to make sure textures are only being added to where they're suppose to.
		if (slot == CS_CHEST || slot == CS_SHIRT) 
		{
			m_CharDetails.geosets[8] = 1 + r.getUInt(ItemDisplayDB::GeosetA);

			tex.AddLayer(MakeItemTexture(CR_ARM_UPPER, r.getString(ItemDisplayDB::TexArmUpper)), CR_ARM_UPPER, layer);
			tex.AddLayer(MakeItemTexture(CR_ARM_LOWER, r.getString(ItemDisplayDB::TexArmLower)), CR_ARM_LOWER, layer);

			tex.AddLayer(MakeItemTexture(CR_TORSO_UPPER, r.getString(ItemDisplayDB::TexChestUpper)), CR_TORSO_UPPER, layer);
			tex.AddLayer(MakeItemTexture(CR_TORSO_LOWER, r.getString(ItemDisplayDB::TexChestLower)), CR_TORSO_LOWER, layer);

			if (type == IT_ROBE) 
			{
				tex.AddLayer(MakeItemTexture(CR_LEG_UPPER, r.getString(ItemDisplayDB::TexLegUpper)), CR_LEG_UPPER, layer);
				tex.AddLayer(MakeItemTexture(CR_LEG_LOWER, r.getString(ItemDisplayDB::TexLegLower)), CR_LEG_LOWER, layer);
			}
		}
		else if (slot == CS_BELT)
			tex.AddLayer(MakeItemTexture(CR_LEG_UPPER, r.getString(ItemDisplayDB::TexLegUpper)), CR_LEG_UPPER, layer);
		else if (slot == CS_BRACERS)
			tex.AddLayer(MakeItemTexture(CR_ARM_LOWER, r.getString(ItemDisplayDB::TexArmLower)), CR_ARM_LOWER, layer);
		else if (slot == CS_PANTS) 
		{
			m_CharDetails.geosets[9] = 1 + r.getUInt(ItemDisplayDB::GeosetB);

			tex.AddLayer(MakeItemTexture(CR_LEG_UPPER, r.getString(ItemDisplayDB::TexLegUpper)), CR_LEG_UPPER, layer);
			tex.AddLayer(MakeItemTexture(CR_LEG_LOWER, r.getString(ItemDisplayDB::TexLegLower)), CR_LEG_LOWER, layer);
		} 
		else if (slot == CS_GLOVES)
		{
			m_CharDetails.geosets[4] = 1 + r.getUInt(ItemDisplayDB::GeosetA);

			tex.AddLayer(MakeItemTexture(CR_HAND, r.getString(ItemDisplayDB::TexHands)), CR_HAND, layer);
			tex.AddLayer(MakeItemTexture(CR_ARM_LOWER, r.getString(ItemDisplayDB::TexArmLower)), CR_ARM_LOWER, layer);
		}
		else if (slot == CS_BOOTS) 
		{ 
			m_CharDetails.geosets[5] = 1 + r.getUInt(ItemDisplayDB::GeosetA);

			tex.AddLayer(MakeItemTexture(CR_LEG_LOWER, r.getString(ItemDisplayDB::TexLegLower)), CR_LEG_LOWER, layer);
			if (!m_CharDetails.showFeet)
				tex.AddLayer(MakeItemTexture(CR_FOOT, r.getString(ItemDisplayDB::TexFeet)), CR_FOOT, layer);
		}
		else if (slot==CS_TABARD && td.showCustom) 
		{ 
			// Display our customised tabard
			m_CharDetails.geosets[12] = 2;
			tex.AddLayer(td.GetBackgroundTex(CR_TORSO_UPPER).c_str(), CR_TORSO_UPPER, layer);
			tex.AddLayer(td.GetBackgroundTex(CR_TORSO_LOWER).c_str(), CR_TORSO_LOWER, layer);
			tex.AddLayer(td.GetIconTex(CR_TORSO_UPPER).c_str(), CR_TORSO_UPPER, layer);
			tex.AddLayer(td.GetIconTex(CR_TORSO_LOWER).c_str(), CR_TORSO_LOWER, layer);
			tex.AddLayer(td.GetBorderTex(CR_TORSO_UPPER).c_str(), CR_TORSO_UPPER, layer);
			tex.AddLayer(td.GetBorderTex(CR_TORSO_LOWER).c_str(), CR_TORSO_LOWER, layer);
		} 
		else if (slot==CS_TABARD) 
		{ 
			// if its just a normal tabard then do the usual
			m_CharDetails.geosets[12] = 2;
			tex.AddLayer(MakeItemTexture(CR_TORSO_UPPER, r.getString(ItemDisplayDB::TexChestUpper)), CR_TORSO_UPPER, layer);
			tex.AddLayer(MakeItemTexture(CR_TORSO_LOWER, r.getString(ItemDisplayDB::TexChestLower)), CR_TORSO_LOWER, layer);
		} 
		else if (slot==CS_CAPE) 
		{ // capes
			m_CharDetails.geosets[15] = 1 + r.getUInt(ItemDisplayDB::GeosetA);

			// load the cape texture
			const char *tex = r.getString(ItemDisplayDB::Skin);
			if (tex && strlen(tex)) 
				capeTex = g_TextureManager.Add( std::string(
				CAnimControlWidget::MakeSkinTexture("Item\\ObjectComponents\\Cape\\", tex)) );
		}

		// robe
		if (m_CharDetails.geosets[13]==1) 
			m_CharDetails.geosets[13] = 1 + r.getUInt(ItemDisplayDB::GeosetC);
		
		if (m_CharDetails.geosets[13]==2) 
		{
			m_CharDetails.geosets[5] = 0;	
			m_CharDetails.geosets[12] = 0;		// also hide the tabard.
		}

		// gloves - this is so gloves have preference over shirt sleeves.
		if (m_CharDetails.geosets[4] > 1) 
			m_CharDetails.geosets[8] = 0;
	} 
	catch (ItemDisplayDB::NotFound) 
	{
	}
}

void CCharControlWidget::RefreshModel(CModel *pModel)
{
	// Reset geosets
	for (int i=0; i<16; i++) 
		m_CharDetails.geosets[i] = 1;
	m_CharDetails.geosets[1] = m_CharDetails.geosets[2] = m_CharDetails.geosets[3] = 0;

	// show ears, if toggled
	if (m_CharDetails.showEars) 
		m_CharDetails.geosets[7] = 2;

	CCharTexture tex;
	CharSectionsDB::Record rec = chardb.getRecord(0);

	// base character layer/texture
	try 
	{
		rec = chardb.getByParams(m_CharDetails.race, m_CharDetails.gender, 
			CharSectionsDB::SkinType, 0, m_CharDetails.skinColor, m_CharDetails.useNPC);
		tex.AddLayer(rec.getString(CharSectionsDB::Tex1), CR_BASE, 0);

		// Tauren fur
		const char *furTexName = rec.getString(CharSectionsDB::Tex2);
		if (strlen(furTexName))
			furTex = g_TextureManager.Add(furTexName);

	}
	catch (CharSectionsDB::NotFound) 
	{
		qCritical("Assertion Error: %s : line #%i : %s");
	}

	// Hair related boolean flags
	bool bald = false;
	bool showHair = m_CharDetails.showHair;
	bool showFacialHair = m_CharDetails.showFacialHair;

	if (m_CharDetails.race != 9)
	{ // Goblin chars base texture already contains all this stuff.

		// Display underwear on the model?
		if (m_CharDetails.showUnderwear)
		{
			try 
			{
				rec = chardb.getByParams(m_CharDetails.race, m_CharDetails.gender,
					CharSectionsDB::UnderwearType, 0, m_CharDetails.skinColor, m_CharDetails.useNPC);
				tex.AddLayer(rec.getString(CharSectionsDB::Tex1), CR_PELVIS_UPPER, 1); // pants
				tex.AddLayer(rec.getString(CharSectionsDB::Tex2), CR_TORSO_UPPER, 1); // top
			} 
			catch (CharSectionsDB::NotFound)
			{
				qCritical("DBC Error: %s : line #%i : %s");
			}
		}

		// face
		try 
		{
			rec = chardb.getByParams(m_CharDetails.race, m_CharDetails.gender, CharSectionsDB::FaceType,
				m_CharDetails.faceType, m_CharDetails.skinColor, m_CharDetails.useNPC);
			tex.AddLayer(rec.getString(CharSectionsDB::Tex1), CR_FACE_LOWER, 1);
			tex.AddLayer(rec.getString(CharSectionsDB::Tex2), CR_FACE_UPPER, 1);
		} 
		catch (CharSectionsDB::NotFound)
		{
			qCritical("DBC Error: %s : line #%i : %s");
		}

		// facial feature
		try 
		{
			rec = chardb.getByParams(m_CharDetails.race, m_CharDetails.gender, CharSectionsDB::FacialHairType,
				m_CharDetails.facialHair, m_CharDetails.facialColor, 0);
			tex.AddLayer(rec.getString(CharSectionsDB::Tex1), CR_FACE_LOWER, 2);
			tex.AddLayer(rec.getString(CharSectionsDB::Tex2), CR_FACE_UPPER, 2);
		} 
		catch (CharSectionsDB::NotFound)
		{
			qCritical("DBC Error: %s : line #%i : %s");
		}

		// facial feature geosets
		try 
		{
			CharFacialHairDB::Record frec = facialhairdb.getByParams(
				m_CharDetails.race, m_CharDetails.gender, m_CharDetails.facialHair);
			m_CharDetails.geosets[1] = frec.getUInt(CharFacialHairDB::Geoset100);
			m_CharDetails.geosets[2] = frec.getUInt(CharFacialHairDB::Geoset200);
			m_CharDetails.geosets[3] = frec.getUInt(CharFacialHairDB::Geoset300);

		} 
		catch (CharFacialHairDB::NotFound)
		{
			qCritical("DBC Error: %s : line #%i : %s");
		}
	}

	// select hairstyle geoset(s)
	for (CharHairGeosetsDB::Iterator it = hairdb.begin(); it != hairdb.end(); ++it) 
	{
		if (it->getUInt(CharHairGeosetsDB::Race) == m_CharDetails.race &&
			it->getUInt(CharHairGeosetsDB::Gender) == m_CharDetails.gender) 
		{
			unsigned int id = it->getUInt(CharHairGeosetsDB::Geoset);
			unsigned int section = it->getUInt(CharHairGeosetsDB::Section);

			if (id!=0) 
			{
				for (size_t j = 0; j < pModel->m_vecGeoset.size(); j++) 
				{
					if (pModel->m_vecGeoset[j].id == id) 
					{
						pModel->m_pbShowGeosets[j] = (m_CharDetails.hairStyle == section) && showHair;
					}
				}

			} 
			else if (m_CharDetails.hairStyle == section) 
			{
				bald = true;
			}
		}
	}

	// hair
	try 
	{
		rec = chardb.getByParams(m_CharDetails.race, m_CharDetails.gender, CharSectionsDB::HairType, 
			m_CharDetails.hairStyle, m_CharDetails.hairColor, 0);
		const char* hairTexfn = rec.getString(CharSectionsDB::Tex1);
		if (strlen(hairTexfn)) 
			hairTex = g_TextureManager.Add(hairTexfn);
		else 
		{
			// oops, looks like we're missing a hair texture. Let's try with hair style #0.
			// (only a problem for orcs with no hair but some beard
			try 
			{
				rec = chardb.getByParams(m_CharDetails.race, m_CharDetails.gender, 
					CharSectionsDB::HairType, 0, m_CharDetails.hairColor, 0);
				hairTexfn = rec.getString(CharSectionsDB::Tex1);
				if (strlen(hairTexfn)) 
					hairTex = g_TextureManager.Add(hairTexfn);
				else 
					hairTex = 0;
			} 
			catch (CharSectionsDB::NotFound)
			{
				// oh well, give up.
				hairTex = 0; // or chartex?
			}
		}

		if (!bald) 
		{
			tex.AddLayer(rec.getString(CharSectionsDB::Tex2), CR_FACE_LOWER, 3);
			tex.AddLayer(rec.getString(CharSectionsDB::Tex3), CR_FACE_UPPER, 3);
		}

	} 
	catch (CharSectionsDB::NotFound) 
	{
		qCritical("DBC Error: %s : line #%i : %s");
		hairTex = 0;
	}

	// If they have no hair, toggle the 'bald' flag.
	if (!showHair)
		bald = true;

	// Hide facial hair if it isn't toggled and they don't have tusks, horns, etc.
	if (!showFacialHair) 
	{		
		try 
		{
			CharRacesDB::Record race = racedb.getById(m_CharDetails.race);
			std::string tmp = race.getString(CharRacesDB::GeoType1);
			if (tmp == "NORMAL") 
			{
				m_CharDetails.geosets[1] = 1;
				m_CharDetails.geosets[2] = 1;
				m_CharDetails.geosets[3] = 1;
			}
		} 
		catch (CharRacesDB::NotFound)
		{
			qCritical("Assertion Error: %s : line #%i : %s");
		}
	}

	// check if we have a robe on
	bool hadRobe = false;
	if (m_CharDetails.equipment[CS_CHEST] != 0) 
	{
		try
		{
			const ItemRecord &item = items.get(m_CharDetails.equipment[CS_CHEST]);
			if (item.type==IT_ROBE) 
			{
				ItemDisplayDB::Record r = itemdb.getById(item.model);
				if (r.getUInt(ItemDisplayDB::GeosetC)==1) 
					hadRobe = true;
			}
		} 
		catch (ItemDisplayDB::NotFound)
		{
			qCritical("Assertion Error: %s : line #%i : %s");
		}
	}

	// check if we have a kilt on, just like our robes
	if (m_CharDetails.equipment[CS_PANTS] != 0)
	{
		try 
		{
			const ItemRecord &item = items.get(m_CharDetails.equipment[CS_PANTS]);
			int type = item.type;
			if (type==IT_PANTS) 
			{
				ItemDisplayDB::Record r = itemdb.getById(item.model);
				if (r.getUInt(ItemDisplayDB::GeosetC)==1) 
					hadRobe = true;
			}
		} 
		catch (ItemDisplayDB::NotFound)
		{
			qCritical("Assertion Error: %s : line #%i : %s");
		}
	}

	// Default
	slotOrderWithRobe[7] = CS_CHEST;
	slotOrderWithRobe[8] = CS_GLOVES;

	// check the order of robe/gloves
	if (m_CharDetails.equipment[CS_CHEST] && m_CharDetails.equipment[CS_GLOVES]) 
	{
		try
		{
			const ItemRecord &item2 = items.get(m_CharDetails.equipment[CS_GLOVES]);
			ItemDisplayDB::Record r2 = itemdb.getById(item2.model);
			if (r2.getUInt(ItemDisplayDB::GeosetA)==0)
			{
				slotOrderWithRobe[7] = CS_GLOVES;
				slotOrderWithRobe[8] = CS_CHEST;
			}
		}
		catch (ItemDisplayDB::NotFound) 
		{
			qCritical("Assertion Error: %s : line #%i : %s");
		}
	}

	// dressup
	for (int i = 0; i < NUM_CHAR_SLOTS; i++) 
	{
		int sn = hadRobe ? slotOrderWithRobe[i] : slotOrder[i];
		if (m_CharDetails.equipment[sn] != 0) 
			AddEquipment(sn, m_CharDetails.equipment[sn], 10+i, tex);
	}

	// reset geosets
	for (size_t j=0; j < pModel->m_vecGeoset.size(); j++)
	{
		int id = pModel->m_vecGeoset[j].id;

		// hide top-of-head if we have hair.
		if (id == 1)
			pModel->m_pbShowGeosets[j] = bald;

		for (int i=1; i<16; i++) 
		{
			int a = i * 100;
			int b = (i + 1) * 100;
			if (id > a && id < b) 
			{
				pModel->m_pbShowGeosets[j] = (id == (a + m_CharDetails.geosets[i]));
			}
		}
	}

	// finalize character texture
	tex.Compose(charTex);

	// set replacable textures
	pModel->m_nReplaceTextures[1] = charTex;
	pModel->m_nReplaceTextures[2] = capeTex;
	pModel->m_nReplaceTextures[6] = hairTex;
	pModel->m_nReplaceTextures[8] = furTex;
	pModel->m_nReplaceTextures[11] = gobTex;
}

void CCharControlWidget::SkinColorSliderMove(int value)
{
	if (m_pModel == NULL)
	{
		return;
	}

	if (!m_pModel->m_bChar)
	{
		return;
	}

	m_CharDetails.skinColor = value;
	myGUI.m_pSkinColorSlider->setValue(m_CharDetails.skinColor);
	myGUI.m_pSkinColorLabel->setText( QString("%1").arg(myGUI.m_pSkinColorSlider->value()) );
	RefreshModel(m_pModel);
}

void CCharControlWidget::FaceTypeSliderMove(int value)
{
	if (m_pModel == NULL)
	{
		return;
	}

	if (!m_pModel->m_bChar)
	{
		return;
	}

	m_CharDetails.faceType = value;
	myGUI.m_pFaceTypeSlider->setValue(m_CharDetails.faceType);
	myGUI.m_pFaceTypeLabel->setText( QString("%1").arg(myGUI.m_pFaceTypeSlider->value()) );
	RefreshModel(m_pModel);
}

void CCharControlWidget::HairColorSliderMove(int value)
{
	if (m_pModel == NULL)
	{
		return;
	}

	if (!m_pModel->m_bChar)
	{
		return;
	}

	m_CharDetails.hairColor = value;
	myGUI.m_pHairColorSlider->setValue(m_CharDetails.hairColor);
	myGUI.m_pHairColorLabel->setText( QString("%1").arg(myGUI.m_pHairColorSlider->value()) );
	RefreshModel(m_pModel);
}


void CCharControlWidget::HairStyleSliderMove(int value)
{
	if (m_pModel == NULL)
	{
		return;
	}

	if (!m_pModel->m_bChar)
	{
		return;
	}

	m_CharDetails.hairStyle = value;
	myGUI.m_pHairStyleSlider->setValue(m_CharDetails.hairStyle);
	myGUI.m_pHairStyleLabel->setText( QString("%1").arg(myGUI.m_pHairStyleSlider->value()) );
	RefreshModel(m_pModel);
}

void CCharControlWidget::FacialFeatureSliderMove(int value)
{
	if (m_pModel == NULL)
	{
		return;
	}

	if (!m_pModel->m_bChar)
	{
		return;
	}

	m_CharDetails.facialHair = value;
	myGUI.m_pFacialFeatureSlider->setValue(m_CharDetails.facialHair);
	myGUI.m_pFacialFeatureLabel->setText( QString("%1").arg(myGUI.m_pFacialFeatureSlider->value()) );
	RefreshModel(m_pModel);
}

void CCharControlWidget::FacialColorSliderMove(int value)
{
	if (m_pModel == NULL)
	{
		return;
	}

	if (!m_pModel->m_bChar)
	{
		return;
	}

	m_CharDetails.facialColor = value;
	myGUI.m_pFacialColorSlider->setValue(m_CharDetails.facialColor);	
	myGUI.m_pFacialColorLabel->setText( QString("%1").arg(myGUI.m_pFacialColorSlider->value()) );
	RefreshModel(m_pModel);
}

void CCharControlWidget::HeadButtonClick()
{
	QString name = myGUI.m_pHeadBtn->text();
	SelectItem(0, m_CharDetails.equipment[0], name);

	g_pCategoryChoiceWidget->show();
}

void CCharControlWidget::ShoulderButtonClick()
{
	QString name = myGUI.m_pShoulderBtn->text();
	SelectItem(2, m_CharDetails.equipment[2], name);

	g_pCategoryChoiceWidget->show();
}

void CCharControlWidget::RightHandButtonClick()
{
	QString name = myGUI.m_pRightHandBtn->text();
	SelectItem(10, m_CharDetails.equipment[10], name);

	g_pCategoryChoiceWidget->show();
}

void CCharControlWidget::LeftHandButtonClick()
{
	QString name = myGUI.m_pLeftHandBtn->text();
	SelectItem(11, m_CharDetails.equipment[11], name);

	g_pCategoryChoiceWidget->show();
}

void CCharControlWidget::ShirtButtonClick()
{
	QString name = myGUI.m_pShirtBtn->text();
	SelectItem(5, m_CharDetails.equipment[5], name);

	g_pCategoryChoiceWidget->show();
}

void CCharControlWidget::ChestButtonClick()
{
	QString name = myGUI.m_pChestBtn->text();
	SelectItem(7, m_CharDetails.equipment[7], name);

	g_pCategoryChoiceWidget->show();
}

void CCharControlWidget::CapeButtonClick()
{
	QString name = myGUI.m_pCapeBtn->text();
	SelectItem(CS_CAPE, m_CharDetails.equipment[CS_CAPE], name);

	g_pCategoryChoiceWidget->show();
}

void CCharControlWidget::SelectItem(int slot, int current, QString caption)
{
	g_pCategoryChoiceWidget->myGUI.m_pList->clear();
	m_vecNumber.clear();
	
	g_pCategoryChoiceWidget->m_pCharCtrlWidget = this;
	g_pCategoryChoiceWidget->m_nType = UPDATE_ITEM;
	m_nChoosingSlot = slot;

	std::vector<ItemRecord>::iterator iter = items.items.begin();
	for (; iter != items.items.end(); ++iter)
	{
		if (CorrectType(iter->type, slot)) 
		{
			g_pCategoryChoiceWidget->myGUI.m_pList->addItem(QString(iter->name.c_str()));
			m_vecNumber.push_back(iter->id);
		}
	}
}

bool CCharControlWidget::CorrectType(int type, int slot)
{
	if (type == IT_ALL) 
		return true;

	switch (slot)
	{
		case CS_HEAD:		return (type == IT_HEAD);
		case CS_NECK:		return (type == IT_NECK);
		case CS_SHOULDER:	return (type == IT_SHOULDER);
		case CS_SHIRT:		return (type == IT_SHIRT);
		case CS_CHEST:		return (type == IT_CHEST || type == IT_ROBE);
		case CS_BELT:		return (type == IT_BELT);
		case CS_PANTS:		return (type == IT_PANTS);
		case CS_BOOTS:		return (type == IT_BOOTS);
		case CS_BRACERS:	return (type == IT_BRACERS);
		case CS_GLOVES:		return (type == IT_GLOVES);
		case CS_HAND_RIGHT:	return (type == IT_LEFTHANDED || type == IT_GUN || type == IT_THROWN || type == IT_2HANDED || type == IT_DAGGER);
		case CS_HAND_LEFT:	return (type == IT_RIGHTHANDED || type == IT_BOW || type == IT_SHIELD || type == IT_2HANDED || type == IT_DAGGER || type == IT_OFFHAND);
		case CS_CAPE:		return (type == IT_CAPE);
		case CS_TABARD:		return (type == IT_TABARD);
		case CS_QUIVER:		return (type == IT_QUIVER);
	}
	
	return false;
}

void CCharControlWidget::RefreshItem(int slot)
{
	m_pCharAtt->DelSlot(slot);
	
	int itemnum = m_CharDetails.equipment[slot];
	if (itemnum != 0)
	{
		int id1=-1, id2=-1;
		std::string path;

		if (slot==CS_HEAD)
		{
			id1 = 11;
			path = "Item\\ObjectComponents\\Head\\";
		}
		else if (slot==CS_SHOULDER) 
		{
			id1 = 6;
			id2 = 5;
			path = "Item\\ObjectComponents\\Shoulder\\";
		}
		else if (slot == CS_HAND_LEFT) 
		{
			id1 = 2;
			m_pCharAtt->m_pModel->m_CharModelDetails.closeLHand = true;
		} 
		else if (slot == CS_HAND_RIGHT)
		{
			id1 = 1;
			m_pCharAtt->m_pModel->m_CharModelDetails.closeRHand = true;
		} 

		if (slot==CS_HAND_LEFT || slot==CS_HAND_RIGHT) 
		{
			if (items.get(itemnum).type == IT_SHIELD) 
			{
				path = "Item\\ObjectComponents\\Shield\\";
				id1 = 0;
			}
			else 
			{
				path = "Item\\ObjectComponents\\Weapon\\";
			}

			// If we're sheathing our weapons, relocate the items to
			// their correct positions
			if (bSheathe && items.get(itemnum).sheath>0) 
			{	
				id1 = items.get(itemnum).sheath;
				if (id1==32 && slot==CS_HAND_LEFT)
					id1 = 33;
				
				if (id1==26 && items.get(itemnum).subclass==7 && slot==CS_HAND_LEFT)
					id1 = 27;

				if (slot==CS_HAND_LEFT)
					m_pCharAtt->m_pModel->m_CharModelDetails.closeLHand = false;
				if (slot==CS_HAND_RIGHT)
					m_pCharAtt->m_pModel->m_CharModelDetails.closeRHand = false;
			}
		}

		try 
		{
			// This corrects the problem with trying to automatically load equipment on NPC's
			int ItemID = 0;
			if (g_pSelModel->m_nModelType == MT_NPC)
				ItemID = itemnum;
			else 
			{
				const ItemRecord &item = items.get(itemnum);
				ItemID = item.model;
			}

			ItemDisplayDB::Record r = itemdb.getById(ItemID);

			int tex;
			std::string mp;
			bool succ = false;
			CAttachment *att = NULL;
			CModel *m = NULL;

			if (id1>=0) 
			{
				mp = path + r.getString(ItemDisplayDB::Model);

				if (slot==CS_HEAD) 
				{
					// sigh, head items have more crap to sort out
					mp = mp.substr(0, mp.length()-4); // delete .mdx
					mp.append("_");
					try 
					{
						CharRacesDB::Record race = racedb.getById(m_CharDetails.race);
						mp.append(race.getString(CharRacesDB::ShortName));
						mp.append(m_CharDetails.gender?"F":"M");
						mp.append(".m2");
					} 
					catch (CharRacesDB::NotFound) 
					{
						mp = "";
					}
				}

				if (mp.length()) 
				{
					if (m_pCharAtt)
					{
						att = m_pCharAtt->AddChild(mp.c_str(), id1, slot, 0);
						if (att) 
						{
							m = static_cast<CModel*>(att->m_pModel);
							if (m->m_bOK)
							{
								mp = path + r.getString(ItemDisplayDB::Skin);
								mp.append(".blp");
								tex = g_TextureManager.Add(mp);
								m->m_nReplaceTextures[2] = tex;

								succ = true;
							}
						}
					}
				}
			}

			if (id2>=0) 
			{
				mp = path + r.getString(ItemDisplayDB::Model2);
				if (mp.length()) 
				{
					att = m_pCharAtt->AddChild(mp.c_str(), id2, slot, 0);
					if (att) 
					{
						m = static_cast<CModel*>(att->m_pModel);
						if (m->m_bOK) 
						{
							mp = path + r.getString(ItemDisplayDB::Skin2);
							mp.append(".blp");
							tex = g_TextureManager.Add(mp);
							m->m_nReplaceTextures[2] = tex;

							succ = true;
						}
					}
				}
			}

			int i = 0;
		}
		catch (ItemDisplayDB::NotFound)
		{}
	}
}

void CCharControlWidget::UpdateItem(int type, int sel)
{
	switch(type)
	{
	case UPDATE_ITEM:
		{
			int num = m_vecNumber[sel];
			m_CharDetails.equipment[m_nChoosingSlot] = num;
			if (SlotHasModel(m_nChoosingSlot))
			{
				RefreshItem(m_nChoosingSlot);
			}
		}
		break;
	default:
		break;
	}

	RefreshModel(m_pModel);
}

//---------------------------------------
//--------CCategoryChoiceWidget----------
//---------------------------------------
CCategoryChoiceWidget::CCategoryChoiceWidget(QWidget *parent, Qt::WFlags flags)
{
	myGUI.setupUi(this);
	m_pCharCtrlWidget = NULL;

	connect(myGUI.m_pList, SIGNAL(currentRowChanged(int)),
		this, SLOT(ListChange(int)) );
}

CCategoryChoiceWidget::~CCategoryChoiceWidget()
{

}

void CCategoryChoiceWidget::ListChange(int index)
{
	if (index == -1)
	{
		return;
	}

	if (m_pCharCtrlWidget)
	{
		m_pCharCtrlWidget->UpdateItem(m_nType, index);
	}
}

//---------------------------------------
//---------CAnimControlWidget------------
//---------------------------------------
CAnimControlWidget::CAnimControlWidget(QWidget *parent, Qt::WFlags flags)
{
	myGUI.setupUi(this);

	connect(myGUI.m_pAnimNameComboBox, SIGNAL(currentIndexChanged(int)), 
		this, SLOT(SelectAnim(int)) );
	connect(myGUI.m_pSkinList, SIGNAL(currentIndexChanged(int)),
		this, SLOT(SelectSkin(int)) );
	connect(myGUI.m_pPlayButton, SIGNAL(clicked(bool)), this, SLOT(PlayClick(bool)));
}

CAnimControlWidget::~CAnimControlWidget()
{

}

void CAnimControlWidget::Update(CModel *pModel)
{
	bool bUsedAnim = false;
	myGUI.m_pAnimNameComboBox->clear();
	myGUI.m_pSkinList->clear();
	m_setSkin.clear();

	if (pModel->m_pAnim == NULL)
	{
		return;
	}

	// ...
	pModel->m_vecAnimName.clear();
	for (unsigned int i = 0; i < pModel->m_ModelHeader.nAnimations; i++)
	{
		AnimDB::Record rec = animdb.getByAnimID(pModel->m_pAnim[i].animID);
		QString strName = rec.getString(AnimDB::Name);
		pModel->m_vecAnimName.push_back(strName.toStdString());

		if(!bUsedAnim && strName == "Stand")
		{
			bUsedAnim = true;
			g_pSelModel->m_nCurrentAnim = i;
		}

		strName += QString("[%1]").arg(i);

		myGUI.m_pAnimNameComboBox->addItem(QString(strName), QVariant(i));
	}

	// replace M2 with MDX
	QString fn(pModel->m_strName.c_str());
	if (fn[fn.length() - 1] == '2') 
	{
		fn[fn.length() - 1] = 'd';
		fn.append("x");
	}

	bool bFind = true;
	CreatureModelDB::Record rec = modeldb.getByFilename( fn.toStdString(), bFind );
	if (bFind)
	{
		if (rec.getUInt(CreatureModelDB::Type) != 4)
		{
			unsigned int modelid = rec.getUInt(CreatureModelDB::ModelID);

			for (CreatureSkinDB::Iterator it = skindb.begin();  it!=skindb.end();  ++it) 
			{
				if (it->getUInt(CreatureSkinDB::ModelID) == modelid)
				{
					CTextureGroup grp;
					for (int i = 0; i < CTextureGroup::num; i++)
					{
						std::string skin(it->getString(CreatureSkinDB::Skin + i));

						grp.tex[i] = skin;
					}

					grp.base = 11;
					grp.count = 3;
					if (grp.tex[0].length() > 0)
					{
						bool bFind = false;
						TextureSet::iterator iter = m_setSkin.begin();
						for (; iter != m_setSkin.end(); iter++)
						{
							CTextureGroup& grpnew = *iter;
							if (!stricmp(grpnew.tex->c_str(), grp.tex->c_str()))
							{
								bFind = true;
								break;
							}
						}

						if (!bFind)
						{
							m_setSkin.insert(grp);
						}
					}
				}
			}

			FillSkinSelector(m_setSkin);
		}
	}
}

void CAnimControlWidget::SelectAnim(int index)
{
	QString text = myGUI.m_pAnimNameComboBox->currentText();
	int nAnimID = myGUI.m_pAnimNameComboBox->itemData(index).toInt();

	g_pSelModel->m_nCurrentAnim = nAnimID;
}

bool CAnimControlWidget::FillSkinSelector(TextureSet &skins)
{
	if (skins.size() > 0)
	{
		// fill our skin selector
		for (TextureSet::iterator it = skins.begin(); it != skins.end(); ++it) 
		{
			myGUI.m_pSkinList->addItem(it->tex[0].c_str());
		}

		bool existingTexture = false;
		for (int i = 0; i<32; i++) 
		{
			if (g_pSelModel->m_nReplaceTextures[i] > 0)
			{
				existingTexture = true;
				break;
			}
		}

		if (!existingTexture) 
		{
			int mySkin = 0;
			SetSkin(mySkin);
		}
	}

	return true;
}

void CAnimControlWidget::SetSkin(int num)
{
	TextureSet::iterator iter = m_setSkin.begin();
	for (int i = 0; i < num; i++)
	{
		iter++;
	}
	
	CTextureGroup *grp = (CTextureGroup*)&(*iter);
	for (int i = 0; i < grp->count; i++) 
	{
		if (g_pSelModel->m_bUseReplaceTextures[grp->base + i]) 
		{
			g_pSelModel->m_nReplaceTextures[grp->base + i] = g_TextureManager.Add( 
				MakeSkinTexture(g_pSelModel->m_strName.c_str(), grp->tex[i].c_str()) );
		}
	}
}

void CAnimControlWidget::SelectSkin(int index)
{
	SetSkin(index);
}

std::string CAnimControlWidget::MakeSkinTexture(const char *texfn, const char *skin)
{
	std::string res = texfn;
	size_t i = res.find_last_of('\\');
	res = res.substr(0,i + 1);
	res.append(skin);
	res.append(".blp");
	return res;
}

void CAnimControlWidget::PlayClick(bool)
{
	QString dir = QFileDialog::getExistingDirectory(this, QString("Open File"));

	CModelExport *pExport = new CModelExport(g_pSelModel);
	if (pExport)
	{
		pExport->Export((char*)dir.toStdString().c_str());
	}

	SAFE_DELETE(pExport);
}