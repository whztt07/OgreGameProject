
#ifndef WOWMAPVIEWER_H
#define WOWMAPVIEWER_H

#include <QtGui/QMainWindow>
#include "ui_wowmapviewer.h"
#include "Ogre3d.h"
#include "World.h"
#include <QTimer>
#include <QMouseEvent>

class CD3D9Widget : public QWidget
{
	Q_OBJECT
public:
	CD3D9Widget( QWidget *parent = 0, Qt::WFlags flags = 0 );
	virtual ~CD3D9Widget();

	void InitD3D9Device();
	void SetViewport();
	void BeginScene();
	void EndScene();
public slots:
		void Idle();
protected:
	QPaintEngine *paintEngine() const { return 0; } // ‘ –ÌD3DªÊ÷∆
	virtual void paintEvent( QPaintEvent *paintE );
	virtual void resizeEvent( QResizeEvent *event );
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void wheelEvent(QWheelEvent *event);
	virtual void keyPressEvent(QKeyEvent *k);
	void UpdateKey();
	void UpdateMouse();
	void Render();

	IDirect3D9 *m_pD3D;
	IDirect3DDevice9 *m_pDevice;
	D3DPRESENT_PARAMETERS m_D3Dpp;
	QTimer	m_Timer;
	bool m_bMidBtnPressed;
	bool m_bLeftBtnPressed;
	bool m_bRightBtnPressed;
	QPoint m_ptPreMouse;
	QPoint m_ptCurMouse;
};

class WowMapViewer : public QMainWindow
{
	Q_OBJECT

public:
	WowMapViewer(QWidget *parent = 0, Qt::WFlags flags = 0);
	~WowMapViewer();

	void LoadWowResources();
	void WriteSetting(QString filename = "");
	void ReadSetting(QString filename = "");

	void closeEvent(QCloseEvent *event);
private:
	//Ui::WowMapViewerClass ui;

	CD3D9Widget *m_pD3D9Widget;
	std::vector<Ogre::CString> m_vecMap;
	CWorld *m_pWorld;
};

extern LPDIRECT3DDEVICE9 g_pDevice;

#endif // WOWMAPVIEWER_H
