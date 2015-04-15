
#include "wowmapviewer.h"
#include "mpq_libmpq.h"
#include "areadb.h"
#include <QSettings>
#include <windows.h>
#include "OgreCamera.h"

//---------------------------------------
//---------------Global------------------
//---------------------------------------
AreaDB gAreaDB;
LPDIRECT3DDEVICE9 g_pDevice = NULL;
float g_fAspect;
extern Ogre::CCamera gCamera;
extern CShaderMgr g_ShaderMgr;

bool IsKeyPressed(int key)
{
	return GetAsyncKeyState(key) & 0x8000;
}

//---------------------------------------
//-------------WowMapViewer--------------
//---------------------------------------
WowMapViewer::WowMapViewer(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	//ui.setupUi(this);

	m_pWorld = NULL;

	m_pD3D9Widget = new CD3D9Widget(this);
	m_pD3D9Widget->setObjectName("D3D9Widget");

	//setFocusPolicy(Qt::NoFocus);
	QMainWindow::setCentralWidget(m_pD3D9Widget);

	// 读设置...
	ReadSetting();

	m_pD3D9Widget->InitD3D9Device();
	//setMouseTracking(false);
}

void WowMapViewer::ReadSetting(QString filename)
{
	// ini文件格式...
	QSettings settings("WowMapViewerCfg", QSettings::IniFormat);

	settings.beginGroup("session");
	QMainWindow::restoreGeometry(settings.value("Size").toByteArray());
	bool bRestore = QMainWindow::restoreState(settings.value("Layout").toByteArray());
	settings.endGroup();

	QMainWindow::setWindowState(Qt::WindowMaximized);
}

void WowMapViewer::WriteSetting(QString filename)
{
	QSettings settings("WowMapViewerCfg", QSettings::IniFormat);

	settings.beginGroup("session");
	settings.setValue("Size", QMainWindow::saveGeometry());
	settings.setValue("Layout", QMainWindow::saveState());
	settings.endGroup();
}

void WowMapViewer::closeEvent(QCloseEvent *event)
{
	WriteSetting();
}

WowMapViewer::~WowMapViewer()
{
	m_vecMap.clear();
	SAFE_DELETE(m_pWorld);
}

void WowMapViewer::LoadWowResources()
{
	char* gamepath = "F:\\Resource\\WOWResource\\WOW_Data\\Data\\";

	std::vector<MPQArchive*> archives;
	const char* archiveNames[] =
	{
		"common.MPQ", 
		"expansion.MPQ", 
		"enUS\\locale-enUS.MPQ", 
		"enUS\\expansion-locale-enUS.MPQ", 
		"enGB\\locale-enGB.MPQ", 
		"enGB\\expansion-locale-enGB.MPQ", 
		"deDE\\locale-deDE.MPQ", 
		"deDE\\expansion-locale-deDE.MPQ", 
		"frFR\\locale-frFR.MPQ", 
		"frFR\\expansion-locale-frFR.MPQ"
	};

	char path[512];

	if (true)
	{
		// patch goes first -> fake priority handling
		sprintf(path, "%s%s", gamepath, "patch.MPQ");
		archives.push_back(new MPQArchive(path));

		sprintf(path, "%s%s", gamepath, "enUS\\Patch-enUS.MPQ");
		archives.push_back(new MPQArchive(path));

		sprintf(path, "%s%s", gamepath, "enGB\\Patch-enGB.MPQ");
		archives.push_back(new MPQArchive(path));

		sprintf(path, "%s%s", gamepath, "deDE\\Patch-deDE.MPQ");
		archives.push_back(new MPQArchive(path));

		sprintf(path, "%s%s", gamepath, "frFR\\Patch-frFR.MPQ");
		archives.push_back(new MPQArchive(path));
	}

	for (size_t i = 0; i < 10; i++) 
	{
		sprintf(path, "%s%s", gamepath, archiveNames[i]);
		archives.push_back(new MPQArchive(path));
	}

	gAreaDB.open();

	// load map...
	DBCFile f("DBFilesClient\\Map.dbc");
	f.open();
	for(DBCFile::Iterator i = f.begin(); i != f.end(); ++i)
	{
		Ogre::CString name = i->getString(1);
		m_vecMap.push_back(name);
	}

	g_ShaderMgr.Init();
	m_pWorld = new CWorld((char*)m_vecMap[0].c_str());
	g_pWorld = m_pWorld;

	m_pWorld->EnterTile(35, 48);
}

//--------------------------------
//--------CD3D9Widget-------------
//--------------------------------
CD3D9Widget::CD3D9Widget( QWidget *parent, Qt::WFlags flags )
:QWidget(parent, flags)
{	
	m_pD3D = NULL;
	m_pDevice = NULL;
	m_bMidBtnPressed = false;
	m_bLeftBtnPressed = false;
	m_bRightBtnPressed = false;

	QWidget::setAttribute(Qt::WA_PaintOnScreen); // 允许D3D绘制
	setFocusPolicy(/*Qt::StrongFocus*/Qt::ClickFocus); // 触发按键事件
	//setMouseTracking(false); // 设置鼠标移动

	m_Timer.setInterval( 0 );
	QObject::connect( &m_Timer, SIGNAL( timeout() ), this, SLOT( Idle() ) );
	m_Timer.start();
}

CD3D9Widget::~CD3D9Widget()
{

}

void CD3D9Widget::InitD3D9Device()
{
	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION); //Standard

	D3DCAPS9 Caps;
	m_pD3D->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &Caps );

	DWORD BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
		Caps.VertexShaderVersion < D3DVS_VERSION( 2, 0 ) )
	{
		BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	int nScnWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScnHeight = GetSystemMetrics(SM_CYSCREEN);
	QSize size = QWidget::size();

	ZeroMemory( &m_D3Dpp, sizeof(m_D3Dpp) );
	m_D3Dpp.Windowed = TRUE;
	m_D3Dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_D3Dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	m_D3Dpp.BackBufferWidth = size.width();
	m_D3Dpp.BackBufferHeight = size.height();
	m_D3Dpp.BackBufferCount = 1;
	m_D3Dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_D3Dpp.MultiSampleQuality = 0;
	m_D3Dpp.EnableAutoDepthStencil = TRUE;
	m_D3Dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	m_D3Dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	//m_D3Dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; 用这个移动的时候屏幕上会有条线
	// 而且帧率足够的情况下也只有60帧
	m_D3Dpp.FullScreen_RefreshRateInHz = 0;

	// Hardware Device
	HRESULT hr = m_pD3D->CreateDevice(
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winId(),
		BehaviorFlags, &m_D3Dpp, &m_pDevice );

	g_pDevice = m_pDevice;
	g_fAspect = (float)size.width() / (float)size.height();

	if( FAILED(hr) )
	{
		qFatal("Create D3D error...");
	}
}

void CD3D9Widget::BeginScene()
{
	if (m_pDevice)
	{
		m_pDevice->BeginScene();
		m_pDevice->Clear( 0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
			D3DCOLOR_COLORVALUE(0.1f, 0.2f, 0.4f, 0.0f), 1.0f, 0);
	}
}

void CD3D9Widget::EndScene()
{
	if (m_pDevice)
	{
		m_pDevice->EndScene();
		m_pDevice->Present(0, 0, 0, 0);
	}
}

void CD3D9Widget::SetViewport()
{
	if (m_pDevice)
	{
		QSize size = CD3D9Widget::size();
		D3DVIEWPORT9 viewport;
		viewport.X = 0;
		viewport.Y = 0;
		viewport.Width = size.width();
		viewport.Height = size.height();
		viewport.MinZ = 0.0f;
		viewport.MaxZ = 1.0f;
		m_pDevice->SetViewport(&viewport);
	}
}

void CD3D9Widget::Render()
{
	BeginScene();

	g_pWorld->RenderWorld();

	EndScene();
}

void CD3D9Widget::paintEvent( QPaintEvent *paintE )
{
	
}

void CD3D9Widget::Idle()
{
	UpdateKey();
	UpdateMouse();

	Render();
}

void CD3D9Widget::resizeEvent( QResizeEvent *event )
{
	//SetViewport();

	QSize size = QWidget::size();
	m_D3Dpp.BackBufferWidth = size.width();
	m_D3Dpp.BackBufferHeight = size.height();
	g_fAspect = (float)size.width() / (float)size.height();
	m_pDevice->Reset(&m_D3Dpp);
}

void CD3D9Widget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_bRightBtnPressed)
	{
		Qt::MouseButtons btn = event->buttons();

		//::GetCursorPos(&m_ptCurMouse);
		m_ptCurMouse = event->pos();
		int x = m_ptCurMouse.x() - m_ptPreMouse.x();
		int y = m_ptCurMouse.y() - m_ptPreMouse.y();

		//if (abs(x) <= 1 && abs(y) <= 1)
		{
			gCamera.Yaw(x * 0.005f);
			gCamera.Pitch(y * 0.005f);
		}

		m_ptPreMouse = m_ptCurMouse;
	}
}

void CD3D9Widget::wheelEvent(QWheelEvent *event)
{
	float fDelta = event->delta() * 0.1f;
	//gCamera.MoveForward(fDelta * 0.1f);
}

void CD3D9Widget::UpdateMouse()
{
}

void CD3D9Widget::UpdateKey()
{
	if(IsKeyPressed('w') || IsKeyPressed('W'))
	{
		gCamera.MoveForward(3.0f);
	}

	if(IsKeyPressed('s') || IsKeyPressed('S'))
	{
		gCamera.MoveForward(-3.0f);
	}

	if(IsKeyPressed('a') || IsKeyPressed('A'))
	{
		gCamera.Strafe(-3.0f);
	}

	if(IsKeyPressed('d') || IsKeyPressed('D'))
	{
		gCamera.Strafe(3.0f);
	}

	if(IsKeyPressed('e') || IsKeyPressed('E'))
	{
		gCamera.MoveUpward(3.0f);
	}

	if(IsKeyPressed('q') || IsKeyPressed('Q'))
	{
		gCamera.MoveUpward(-3.0f);
	}
}

void CD3D9Widget::keyPressEvent(QKeyEvent *k)
{
	//if(k->key() == Qt::Key_W) 
	//{ 
	//	gCamera.MoveForward(5.0f);
	//} 
	//
	//if (k->key() == Qt::Key_S)
	//{
	//	gCamera.MoveForward(-5.0f);
	//}
	//
	//if (k->key() == Qt::Key_A)
	//{
	//	gCamera.Strafe(-5.0f);
	//}
	//
	//if (k->key() == Qt::Key_D)
	//{
	//	gCamera.Strafe(5.0f);
	//}
	//
	//if (k->key() == Qt::Key_Q)
	//{
	//	gCamera.MoveUpward(-5.0f);
	//}
	//
	//if (k->key() == Qt::Key_E)
	//{
	//	gCamera.MoveUpward(5.0f);
	//}
}

void CD3D9Widget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_bLeftBtnPressed = true;
	}
	else if (event->button() == Qt::MiddleButton)
	{
		m_bMidBtnPressed = true;
	}
	else if (event->button() == Qt::RightButton)
	{
		m_bRightBtnPressed = true;
	}

	m_ptPreMouse = event->pos();
	//::GetCursorPos(&m_ptPreMouse);
}

void CD3D9Widget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_bLeftBtnPressed = false;
	}
	else if (event->button() == Qt::MiddleButton)
	{
		m_bMidBtnPressed = false;
	}
	else if (event->button() == Qt::RightButton)
	{
		m_bRightBtnPressed = false;
	}
}

