

#include "ModelCanvas.h"
#include "OgreCamera.h"
#include "ddslib.h"


//--------------------------------
//-----------Common---------------
//--------------------------------
LPDIRECT3DDEVICE9 g_pDevice = NULL;
float g_fAspect = 1.333f;
CMpqTextureManager g_TextureManager;
Ogre::CCamera gCamera;
int globalTime = 0;

D3DVERTEXELEMENT9 ModelVertexElements[] = 
{
	{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
	{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{0, 32, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
	{0, 36, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	D3DDECL_END()
};

void DecompressDXTC(int format, int w, int h, size_t size, unsigned char *src, unsigned char *dest)
{	
	if (format == COMPRESSED_DXT1) 
	{
		DDSDecompressDXT1(src, w, h, dest);
		return;
	}
	
	// DXT3 Textures
	if (format == COMPRESSED_DXT3) 
	{
		DDSDecompressDXT3(src, w, h, dest);
		return;
	}

	// DXT5 Textures
	if (format == COMPRESSED_DXT5)	
	{
		DDSDecompressDXT5(src, w, h, dest);
		return;
	}
}

Ogre::CQuaternion Quat16ToQuat32(PACK_QUATERNION t)
{
		return Ogre::CQuaternion(
			float(t.x < 0? t.x + 32768 : t.x - 32767)/ 32767.0f, 
			float(t.y < 0? t.y + 32768 : t.y - 32767)/ 32767.0f,
			float(t.z < 0? t.z + 32768 : t.z - 32767)/ 32767.0f,
			float(t.w < 0? t.w + 32768 : t.w - 32767)/ 32767.0f);
};

Ogre::CVector FixCoordSystem(Ogre::CVector v)
{
	return Ogre::CVector(v.x, v.z, -v.y);
}

Ogre::CVector FixCoordSystem2(Ogre::CVector v)
{
	return Ogre::CVector(v.x, v.z, v.y);
}

Ogre::CQuaternion FixCoordSystemQuat(Ogre::CQuaternion v)
{
	return Ogre::CQuaternion(-v.x, -v.z, v.y, v.w);
}

void CModelTransparency::Init(CMPQFile &f, CModelTransDef &mcd, int *global)
{
	trans.init(mcd.trans, f, global);
}

void CModelColor::Init(CMPQFile &f, CModelColorDef &mcd, int *global)
{
	color.init(mcd.color, f, global);
	opacity.init(mcd.opacity, f, global);
}

//--------------------------------
//--------CAttachment-------------
//--------------------------------
CAttachment::CAttachment()
{
	m_matAtt.Unit();
	m_pParent = NULL;
	m_pModel = NULL;
	m_nId = -1;
	m_nSlot = -1;
}

CAttachment::CAttachment(CAttachment *pParent, CModel *pModel, int id, int slot)
{
	m_matAtt.Unit();
	m_pParent = pParent;
	m_pModel = pModel;
	m_nId = id;
	m_nSlot = slot;
}

CAttachment::~CAttachment()
{
	SAFE_DELETE(m_pModel);
}

void CAttachment::Setup()
{
	if (m_pParent == NULL) 
		return;
	
	if (m_pParent->m_pModel) 
	{
		m_matAtt = m_pParent->m_pModel->SetupAtt(m_nId);
	}
}

void CAttachment::Draw(CModelCanvas *canvas)
{
	Setup();

	if (m_pModel)
	{
		m_pModel->Draw(m_matAtt);
	}

	for (int i = 0; i < m_vecChildren.size(); i++)
	{
		CAttachment *pAtt = m_vecChildren[i];
		if (pAtt)
		{
			pAtt->Draw(canvas);
		}
	}
}

void CAttachment::Clear()
{

}

CAttachment* CAttachment::AddChild(CModel *pModel, int id, int slot)
{
	CAttachment *att = new CAttachment(this, pModel, id, slot);
	m_vecChildren.push_back(att);
	return att;
}

void CAttachment::DelSlot(int slot)
{
	for (size_t i = 0; i < m_vecChildren.size(); ) 
	{
		if (m_vecChildren[i]->m_nSlot == slot)
		{
			SAFE_DELETE(m_vecChildren[i]);
			m_vecChildren.erase(m_vecChildren.begin() + i);
		} 
		else 
			i++;
	}
}

CAttachment* CAttachment::AddChild(const char *modelfn, int id, int slot, int elseParam)
{
	if (!modelfn || !strlen(modelfn) || id < 0) 
		return NULL;

	CModel *m = new CModel(modelfn, true);

	if (m && m->m_bOK)
	{
		return AddChild(m, id, slot);
	} 
	else 
	{
		SAFE_DELETE(m);
		return NULL;
	}
}

//--------------------------------
//--------CModelCanvas------------
//--------------------------------
CModelCanvas::CModelCanvas()
{
	m_pModel = NULL;
	m_pRoot = new CAttachment;
	m_pCurAtt = NULL;
}

CModelCanvas::~CModelCanvas()
{

}

CAttachment* CModelCanvas::LoadCharModel(QString path)
{
	if (m_pModel)
	{
		SAFE_DELETE(m_pModel);
	}

	m_pRoot->Clear();

	m_pModel = new CModel(path.toStdString());
	m_pRoot->m_pModel = m_pModel;

	//CAttachment *att = m_pRoot->AddChild(m_pModel, -1, -1);
	//m_pCurAtt = att;
	//return att;
	m_pCurAtt = m_pRoot;
	return m_pCurAtt;
}

void CModelCanvas::LoadModel(QString path)
{
	if (m_pModel)
	{
		SAFE_DELETE(m_pModel);
	}

	m_pModel = new CModel(path.toStdString());
	m_pRoot->m_pModel = m_pModel;
}

void CModelCanvas::SetRotate(float fYaw, float fPitch)
{
	m_vRotate.x += fPitch;
	m_vRotate.z += fYaw;
}

void CModelCanvas::Render()
{
	RenderObjects();
}

void CModelCanvas::RenderObjects()
{
	D3DXMATRIX mView;
	D3DXMATRIX mProj;

	D3DXVECTOR3 vEye = gCamera.GetEye();
	D3DXVECTOR3 vDir = gCamera.GetViewDir();
	D3DXVECTOR3 vLookat = vEye + vDir;
	D3DXVECTOR3 vUp = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	D3DXMatrixLookAtLH(&mView, &vEye, &vLookat, &vUp);
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI / 4, g_fAspect, 0.1f, 1000.0f);
	g_pDevice->SetTransform(D3DTS_VIEW, &mView);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &mProj);

	Ogre::CMatrix mYaw, mPitch;
	Ogre::CMatrix::RotationAxis(&mYaw, &Ogre::CVector(0.0f, 1.0f, 0.0f), m_vRotate.z);
	Ogre::CMatrix::RotationAxis(&mPitch, &Ogre::CVector(1.0f, 0.0f, 0.0f), m_vRotate.x);
	D3DXMATRIX mWorld = mYaw * mPitch;
	//D3DXMATRIX mWorld = mPitch * mYaw;
	g_pDevice->SetTransform(D3DTS_WORLD, &mWorld);

	m_pRoot->Draw(this);
}

//--------------------------------
//--------CD3D9Widget-------------
//--------------------------------
CD3D9Widget::CD3D9Widget( QWidget *parent, Qt::WFlags flags )
:QWidget(parent, flags)
{	
	m_pD3D = NULL;
	m_pDevice = NULL;
	m_pModelCanvas = NULL;
	m_bMidBtnPressed = false;
	m_bLeftBtnPressed = false;
	m_bRightBtnPressed = false;
	
	QWidget::setAttribute(Qt::WA_PaintOnScreen);

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

void CD3D9Widget::SetModelCanvas(CModelCanvas *canvas)
{
	m_pModelCanvas = canvas;
}

void CD3D9Widget::Render()
{
	BeginScene();

	if (m_pModelCanvas)
	{
		m_pModelCanvas->Render();
	}

	EndScene();
}

void CD3D9Widget::paintEvent( QPaintEvent *paintE )
{
}

void CD3D9Widget::Idle()
{
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
	m_ptCurMouse = event->pos();
	int x = m_ptCurMouse.x() - m_ptPreMouse.x();
	int y = m_ptCurMouse.y() - m_ptPreMouse.y();
	
	if (m_bRightBtnPressed)
	{
		gCamera.Strafe(-x * 0.005f);
		gCamera.MoveUpward(y * 0.005f);
	}
	else if (m_bLeftBtnPressed)
	{
		m_pModelCanvas->SetRotate(-x * 0.01f, y * 0.005f);
	}

	m_ptPreMouse = m_ptCurMouse;
}

void CD3D9Widget::wheelEvent(QWheelEvent *event)
{
	float fDelta = event->delta() * 0.1f;
	gCamera.MoveForward(fDelta * 0.1f);
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

//--------------------------------
//------------CBone---------------
//--------------------------------
CBone::CBone()
{
	m_bCalc = false;
	m_bUsedTrans = false;
	m_bUsedScale = false;
	m_bUsedQuat = false;
	m_nParent = -1;
}

CBone::~CBone()
{

}

void CBone::Init(CMPQFile &f, CModelBoneDef &b, int *global)
{
	m_bCalc = false;

	m_BoneDef = b;
	//m_vPivot = b.pivot;
	m_vPivot = FixCoordSystem(b.pivot);
	m_nParent = b.parent;

	// tanslate ...
	m_nTransType = b.translation.type;
	m_bUsedTrans = (b.translation.nKeys > 0);
	m_nTransSeq = b.translation.seq;
	m_pnGlobals = global;
	
	uint32 *ptimes = (uint32*)(f.getBuffer() + b.translation.ofsTimes);
	for (size_t i = 0; i < b.translation.nTimes; i++)
	{
		m_vecTransTime.push_back(ptimes[i]);
	}

	if (b.translation.nRanges > 0) 
	{
		uint32 *pranges = (uint32*)(f.getBuffer() + b.translation.ofsRanges);
		for (size_t i = 0, k = 0; i < b.translation.nRanges; i++) 
		{
			AnimRange r;
			r.first = pranges[k++];
			r.second = pranges[k++];
			m_vecTransRange.push_back(r);
		}
	}

	Ogre::CVector* tKeys = (Ogre::CVector*)(f.getBuffer() + b.translation.ofsKeys);
	switch (m_nTransType)
	{
	case INTERPOLATION_NONE:
	case INTERPOLATION_LINEAR:
	case INTERPOLATION_HERMITE:
		{
			for (size_t i = 0; i < b.translation.nKeys; i++) 
				m_vecTrans.push_back(FixCoordSystem(tKeys[i]));
		}
		break;
	}

	// scaleing ...
	m_nScaleType = b.scaling.type;
	m_bUsedScale = (b.scaling.nKeys > 0);
	m_nScaleSeq = b.scaling.seq;

	ptimes = (uint32*)(f.getBuffer() + b.scaling.ofsTimes);
	for (size_t i = 0; i < b.scaling.nTimes; i++)
	{
		m_vecScaleTime.push_back(ptimes[i]);
	}

	if (b.scaling.nRanges > 0) 
	{
		uint32 *pranges = (uint32*)(f.getBuffer() + b.scaling.ofsRanges);
		for (size_t i = 0, k = 0; i < b.scaling.nRanges; i++) 
		{
			AnimRange r;
			r.first = pranges[k++];
			r.second = pranges[k++];
			m_vecScaleRange.push_back(r);
		}
	}

	Ogre::CVector *sKeys = (Ogre::CVector*)(f.getBuffer() + b.scaling.ofsKeys);
	switch(m_nScaleType)
	{
	case INTERPOLATION_NONE:
	case INTERPOLATION_LINEAR:
	case INTERPOLATION_HERMITE:
		{
			for (size_t i = 0; i < b.scaling.nKeys; i++) 
				m_vecScale.push_back(FixCoordSystem2(sKeys[i]));
		}
		break;
	}

	// quat ...
	m_nQuatType = b.rotation.type;
	m_bUsedQuat = (b.rotation.nKeys > 0);
	m_nQuatSeq = b.rotation.seq;
	
	ptimes = (uint32*)(f.getBuffer() + b.rotation.ofsTimes);
	for (size_t i = 0; i < b.rotation.nTimes; i++)
	{
		m_vecQuatTime.push_back(ptimes[i]);
	}

	if (b.rotation.nRanges > 0) 
	{
		uint32 *pranges = (uint32*)(f.getBuffer() + b.rotation.ofsRanges);
		for (size_t i = 0, k = 0; i < b.rotation.nRanges; i++) 
		{
			AnimRange r;
			r.first = pranges[k++];
			r.second = pranges[k++];
			m_vecQuatRange.push_back(r);
		}
	}
	
	PACK_QUATERNION* qKeys = (PACK_QUATERNION*)(f.getBuffer() + b.rotation.ofsKeys);
	switch(m_nQuatType)
	{
	case INTERPOLATION_NONE:
	case INTERPOLATION_LINEAR:
	case INTERPOLATION_HERMITE:
		{
			for (size_t i = 0; i < b.rotation.nKeys; i++) 
				m_vecQuat.push_back(FixCoordSystemQuat(Quat16ToQuat32(qKeys[i])));
		}
		break;
	}
}

void CBone::CalcMatrix(CBone* allbones, int anim, int time, bool rotate)
{
	if (m_bCalc)
	{
		return;
	}

	Ogre::CMatrix m;
	bool tr = m_bUsedTrans || m_bUsedScale || m_bUsedQuat;
	if (tr)
	{
		m.Translation(m_vPivot);

		if (m_bUsedTrans) 
		{
			Ogre::CVector tr = GetTrans(anim, time);
			m *= Ogre::CMatrix::NewTranslation(tr);
		}

		if (m_bUsedQuat) 
		{
			Ogre::CQuaternion q = GetQuat(anim, time);
			m *= Ogre::CMatrix::NewQuatRotate(q);
		}

		if (m_bUsedScale)
		{
			Ogre::CVector sc = GetScale(anim, time);
			m *= Ogre::CMatrix::NewScale(sc);
		}

		m *= Ogre::CMatrix::NewTranslation(m_vPivot * -1.0f);
	}
	else
	{
		m.Unit();
	}

	if (m_nParent > -1) 
	{
		allbones[m_nParent].CalcMatrix(allbones, anim, time, rotate);
		m_matBone = allbones[m_nParent].m_matBone * m;
	} 
	else
	{
		m_matBone = m;
	}

	m_bCalc = true;
}

Ogre::CQuaternion CBone::GetQuat(unsigned int anim, unsigned int time)
{
	if (m_vecQuat.size() > 1)
	{
		AnimRange range;
		if (m_nQuatSeq > -1)
		{
			if (m_pnGlobals[m_nQuatSeq] == 0) 
				time = 0;
			else 
				time = globalTime % m_pnGlobals[m_nQuatSeq];

			range.first = 0;
			range.second = m_vecQuat.size() - 1;
		} 
		else
		{	
			if (m_vecQuatRange.size() == 0)
			{
				return Ogre::CQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
			}

			range = m_vecQuatRange[anim];
			time %= m_vecQuatTime[ m_vecQuatTime.size() - 1];
		}

		if (range.first != range.second) 
		{
			size_t t1, t2;
			size_t pos = 0;
			for (size_t i = range.first; i < range.second; i++) 
			{
				if (time >= m_vecQuatTime[i] && time < m_vecQuatTime[i + 1]) 
				{
					pos = i;
					break;
				}
			}

			t1 = m_vecQuatTime[pos];
			t2 = m_vecQuatTime[pos + 1];
			float r = (time - t1) / (float)(t2 - t1);

			if (m_nQuatType == INTERPOLATION_LINEAR || m_nQuatType == INTERPOLATION_HERMITE)
			{
				return Ogre::Interpolate(r, m_vecQuat[pos], m_vecQuat[pos + 1]);
			}
			else if (m_nQuatType == INTERPOLATION_NONE) 
			{
				return m_vecQuat[pos];
			}
		}
	}
	else
	{
		if (m_vecQuat.size() == 0)
			return Ogre::CQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
		else
			return m_vecQuat[0];
	}
}

Ogre::CVector CBone::GetScale(unsigned int anim, unsigned int time)
{
	if (m_vecScale.size() > 1)
	{
		AnimRange range;
		if (m_nScaleSeq > -1)
		{
			if (m_pnGlobals[m_nScaleSeq] == 0) 
				time = 0;
			else 
				time = globalTime % m_pnGlobals[m_nScaleSeq];

			range.first = 0;
			range.second = m_vecScale.size() - 1;
		} 
		else
		{	
			if (m_vecScaleRange.size() == 0)
			{
				return Ogre::CVector(0.0f, 0.0f, 0.0f);
			}

			range = m_vecScaleRange[anim];
			time %= m_vecScaleTime[ m_vecScaleTime.size() - 1];
		}

		if (range.first != range.second) 
		{
			size_t t1, t2;
			size_t pos = 0;
			for (size_t i = range.first; i < range.second; i++) 
			{
				if (time >= m_vecScaleTime[i] && time < m_vecScaleTime[i + 1]) 
				{
					pos = i;
					break;
				}
			}

			t1 = m_vecScaleTime[pos];
			t2 = m_vecScaleTime[pos + 1];
			float r = (time - t1) / (float)(t2 - t1);

			if (m_nScaleType == INTERPOLATION_LINEAR || m_nScaleType == INTERPOLATION_HERMITE)
			{
				return Ogre::Interpolate(r, m_vecScale[pos], m_vecScale[pos + 1]);
			}
			else if (m_nScaleType == INTERPOLATION_NONE) 
			{
				return m_vecScale[pos];
			}
		}
	}
	else
	{
		if (m_vecScale.size() == 0)
			return Ogre::CVector(0.0f, 0.0f, 0.0f);
		else
			return m_vecScale[0];
	}
}

Ogre::CVector CBone::GetTrans(unsigned int anim, unsigned int time)
{
	if (m_vecTrans.size() > 1)
	{
		AnimRange range;
		if (m_nTransSeq > -1)
		{
			if (m_pnGlobals[m_nTransSeq] == 0) 
				time = 0;
			else 
				time = globalTime % m_pnGlobals[m_nTransSeq];
			
			range.first = 0;
			range.second = m_vecTrans.size() - 1;
		} 
		else
		{	
			if (m_vecTransRange.size() == 0)
			{
				return Ogre::CVector(0.0f, 0.0f, 0.0f);
			}
			
			range = m_vecTransRange[anim];
			time %= m_vecTransTime[ m_vecTransTime.size() - 1]; // I think this might not be necessary?
		}

		if (range.first != range.second) 
		{
			size_t t1, t2;
			size_t pos = 0;
			for (size_t i = range.first; i < range.second; i++) 
			{
				if (time >= m_vecTransTime[i] && time < m_vecTransTime[i + 1]) 
				{
					pos = i;
					break;
				}
			}

			t1 = m_vecTransTime[pos];
			t2 = m_vecTransTime[pos + 1];
			float r = (time - t1) / (float)(t2 - t1);

			if (m_nTransType == INTERPOLATION_LINEAR || m_nTransType == INTERPOLATION_HERMITE)
			{
				return Ogre::Interpolate(r, m_vecTrans[pos], m_vecTrans[pos + 1]);
			}
			else if (m_nTransType == INTERPOLATION_NONE) 
			{
				return m_vecTrans[pos];
			}
		}
	}
	else
	{
		if (m_vecTrans.size() == 0)
			return Ogre::CVector(0.0f, 0.0f, 0.0f);
		else
			return m_vecTrans[0];
	}
}

//--------------------------------
//--------CAnimManager------------
//--------------------------------
CAnimManager::CAnimManager(CModelAnimation *anim)
{
	m_pAnim = anim;
	m_nFrame = 0;
}

CAnimManager::~CAnimManager()
{

}

int CAnimManager::GetFrame()
{
	return m_nFrame;
}

//--------------------------------
//--------CModelCanvas------------
//--------------------------------
CModel::CModel(std::string name, bool forceAnim)
:m_pnGlobalSequences(NULL),
m_pOrigVertices(NULL),
m_fRadius(0.0f),
m_bAnimated(false),
m_bAnimBones(false),
m_bAnimGeometry(false),
m_pBone(NULL),
m_pAnim(NULL),
m_pbShowGeosets(FALSE),
m_nCurrentAnim(0)
{
	m_bOK = false;
	m_bForceAnim = forceAnim;
	m_pVertices = NULL;
	m_pNormals = NULL;
	m_pTexcoord = NULL;
	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;
	m_pVertexDecl = NULL;
	m_pTextureID = NULL;
	m_nNumTexture = 0;
	m_pAnimManager = NULL;
	m_CharModelDetails.Reset();
	m_pTransparency = NULL;
	m_pColors = NULL;
	m_bChar = false;
	m_nModelType = MT_NORMAL;
	dwLastTime = HQ_TimeGetTime();

	for (int i = 0; i < 32; i++)
	{
		m_nSpecialTextures[i] = -1;
		m_nReplaceTextures[i] = -1;
		m_bUseReplaceTextures[i] = false;
	}
	
	if (name == "")
		return;

	m_strName = name;

	// replace .MDX with .M2
	char tempname[256];
	strcpy(tempname, name.c_str());
	if (tempname[name.length()-1] != '2') 
	{
		tempname[name.length()-2] = '2';
		tempname[name.length()-1] = 0;
	}

	CMPQFile f(tempname);
	bool ok = false;
	if (f.isEof() || (f.getSize() < sizeof(CModelHeader))) 
	{
		m_bOK = FALSE;
		//qFatal("Error: Unable to load model: [%s]", tempname);
		f.close();
		return;
	}
	ok = true;

	memcpy(&m_ModelHeader, f.getBuffer(), sizeof(CModelHeader));

	// Error check
	if (m_ModelHeader.id[0] != 'M' && 
		m_ModelHeader.id[1] != 'D' && 
		m_ModelHeader.id[2] != '2' &&
		m_ModelHeader.id[3] != '0') 
	{
		qFatal("Error:\t\tInvalid model!  May be corrupted.");
		ok = false;
		m_bOK = FALSE;
		f.close();
		return;
	}

	// Error check
	// 4 1 0 0 = WoW 2.0 models
	// 0 1 0 0 = WoW 1.0 models
	if (m_ModelHeader.version[0] != 4 && 
		m_ModelHeader.version[1] != 1 && 
		m_ModelHeader.version[2] != 0 && 
		m_ModelHeader.version[3] != 0)
	{
		qFatal("Error:\t\tModel version is incorrect!\n\t\tMake sure you are loading models from World of Warcraft 2.0.1 or newer client.");
		ok = false;
		m_bOK = FALSE;
		f.close();

		if (m_ModelHeader.version[0] == 0)
		{
			qFatal("An error occured while trying to load the model %s.\nWoW Model Viewer 0.5.x only supports loading WoW 2.0 models\nModels from WoW 1.12 or earlier are not supported");
		}

		return;
	}

	if (f.getSize() < m_ModelHeader.ofsParticleEmitters)
	{
		qFatal("Error: Unable to load the Model \"%s\", appears to be corrupted.", tempname);
		m_bOK = FALSE;
	}

	if (m_ModelHeader.nGlobalSequences) 
	{
		m_pnGlobalSequences = new int[m_ModelHeader.nGlobalSequences];
		memcpy(m_pnGlobalSequences, (f.getBuffer() + m_ModelHeader.ofsGlobalSequences),
			m_ModelHeader.nGlobalSequences * 4);
	}

	m_bAnimated = IsAnimated(f) || forceAnim;

	if (m_bAnimated)
	{
		InitAnimated(f);
	}
	else 
	{
		InitStatic(f);
	}

	f.close();

	if (m_nNumVertex > 0)
	{
		CreateD3D9Res();
	}

	m_bOK = true;
}

void CModel::CreateD3D9Res()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pVertexDecl);

	// CreateDevice...
	HRESULT hr = g_pDevice->CreateVertexBuffer(	m_nNumVertex * sizeof(CModelVertex), 
									D3DUSAGE_WRITEONLY,
									0,
									D3DPOOL_DEFAULT,
									&m_pVertexBuffer,
									NULL);
	if (FAILED(hr))
	{
		qCritical("CModel CreateD3D9Res CreateVertexBuffer");
	}

	g_pDevice->CreateIndexBuffer(	m_nNumIndex * sizeof(int16),
									D3DUSAGE_WRITEONLY,
									D3DFMT_INDEX16,
									D3DPOOL_DEFAULT,
									&m_pIndexBuffer,
									NULL);
	if (FAILED(hr))
	{
		qCritical("CModel CreateD3D9Res CreateIndexBuffer");
	}

	g_pDevice->CreateVertexDeclaration(ModelVertexElements, &m_pVertexDecl);

	// Fill data...
	CModelDrawVertex *pModelVertex = NULL;
	m_pVertexBuffer->Lock(0, 0, (void**)&pModelVertex, 0);
	for (int nVertex = 0; nVertex < m_nNumVertex; nVertex++, pModelVertex++)
	{
		pModelVertex->pos = m_pVertices[nVertex];
		pModelVertex->normal = m_pNormals[nVertex];
		pModelVertex->tex = m_pTexcoord[nVertex];

		for (int i = 0; i < 4; i++)
		{
			pModelVertex->weight[i] = m_pOrigVertices[nVertex].weights[i];
			pModelVertex->index[i] = m_pOrigVertices[nVertex].bones[i];
		}
	}
	m_pVertexBuffer->Unlock();

	int16 *pIndexData = NULL;
	m_pIndexBuffer->Lock(0, 0, (void**)&pIndexData, 0);
	memcpy(pIndexData, m_pIndices, m_nNumIndex * sizeof(int16));
	m_pIndexBuffer->Unlock();
}

CModel::~CModel()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pVertexDecl);
	SAFE_DELETE_ARRAY(m_pnGlobalSequences);
	SAFE_DELETE_ARRAY(m_pVertices);
	SAFE_DELETE_ARRAY(m_pNormals);
	SAFE_DELETE_ARRAY(m_pTexcoord);

	m_vecPass.clear();
}

void CModel::InitAnimated(CMPQFile &f)
{
	if (m_pOrigVertices) 
	{
		delete [] m_pOrigVertices;
		m_pOrigVertices = NULL;
	}

	m_pOrigVertices = new CModelVertex[m_ModelHeader.nVertices];
	memcpy(m_pOrigVertices, f.getBuffer() + m_ModelHeader.ofsVertices, 
		m_ModelHeader.nVertices * sizeof(CModelVertex));

	InitCommon(f);

	if (m_bAnimBones) 
	{
		// init bones...
		m_pBone = new CBone[m_ModelHeader.nBones];
		CModelBoneDef *mb = (CModelBoneDef*)(f.getBuffer() + m_ModelHeader.ofsBones);
		for (size_t i = 0; i < m_ModelHeader.nBones; i++) 
		{
			m_pBone[i].Init(f, mb[i], m_pnGlobalSequences);
		}

		int16 *p = (int16*)(f.getBuffer() + m_ModelHeader.ofsF);
		for (size_t i = 0; i < m_ModelHeader.nF; i++) 
		{
			m_BoneLookup[i] = p[i];
		}
	}

	if (m_ModelHeader.nAnimations > 0)
	{
		m_pAnim = new CModelAnimation[m_ModelHeader.nAnimations];
		memcpy(m_pAnim, f.getBuffer() + m_ModelHeader.ofsAnimations, 
			m_ModelHeader.nAnimations * sizeof(CModelAnimation));
		m_pAnimManager = new CAnimManager(m_pAnim);
	}

	if (!m_bForceAnim)
	{
		gCamera.SetViewParam(
			Ogre::CVector(0.0f, -m_fRadius * 2.0f, m_fRadius * 2.0f),
			Ogre::CVector(0.0f, 0.0f, 0.0f), 
			Ogre::CVector(0.0f, 0.0f, 1.0f));
	}
}

bool CModel::IsAnimated(CMPQFile &f)
{
	// see if we have any animated bones
	CModelBoneDef *bo = (CModelBoneDef*)(f.getBuffer() + m_ModelHeader.ofsBones);

	m_bAnimGeometry = false;
	m_bAnimBones = false;
	bool ind = false;

	CModelVertex *verts = (CModelVertex*)(f.getBuffer() + m_ModelHeader.ofsVertices);
	for (size_t i = 0; i < m_ModelHeader.nVertices && !m_bAnimGeometry; i++) 
	{
		for (size_t b = 0; b < 4; b++)
		{
			if (verts[i].weights[b] > 0) 
			{
				CModelBoneDef &bb = bo[verts[i].bones[b]];
				if (bb.translation.type || bb.rotation.type || bb.scaling.type || (bb.flags&8)) 
				{
					if (bb.flags&8) 
					{
						// if we have billboarding, the model will need per-instance animation
						ind = true;
					}

					m_bAnimGeometry = true;
					break;
				}
			}
		}
	}

	if (m_bAnimGeometry)
	{
		m_bAnimBones = true;
	}
	else
	{
		for (size_t i = 0; i < m_ModelHeader.nBones; i++) 
		{
			CModelBoneDef &bb = bo[i];
			if (bb.translation.type || bb.rotation.type || bb.scaling.type) 
			{
				m_bAnimBones = true;
				break;
			}
		}
	}

	bool animTextures = m_ModelHeader.nTexAnims > 0;

	bool animMisc = m_ModelHeader.nCameras > 0 || 
		// why waste time, pretty much all models with cameras need animation
		m_ModelHeader.nLights > 0 || // same here
		m_ModelHeader.nParticleEmitters > 0 ||
		m_ModelHeader.nRibbonEmitters > 0;

	if (animMisc) 
		m_bAnimBones = true;

	// animated colors
	if (m_ModelHeader.nColors) 
	{
		CModelColorDef *cols = (CModelColorDef*)(f.getBuffer() + m_ModelHeader.ofsColors);
		for (size_t i = 0; i < m_ModelHeader.nColors; i++) 
		{
			if (cols[i].color.type!=0 || cols[i].opacity.type!=0)
			{
				animMisc = true;
				break;
			}
		}
	}

	// animated opacity
	if (m_ModelHeader.nTransparency && !animMisc) 
	{
		CModelTransDef *trs = (CModelTransDef*)(f.getBuffer() + m_ModelHeader.ofsTransparency);
		for (size_t i = 0; i < m_ModelHeader.nTransparency; i++) 
		{
			if (trs[i].trans.type != 0)
			{
				animMisc = true;
				break;
			}
		}
	}

	// guess not...
	return m_bAnimGeometry || animTextures || animMisc;
}

void CModel::InitStatic(CMPQFile &f)
{
	m_pOrigVertices = (CModelVertex*)(f.getBuffer() + m_ModelHeader.ofsVertices);
	
	InitCommon(f);

	gCamera.SetViewParam(
		Ogre::CVector(0.0f, -m_fRadius * 2.0f, m_fRadius * 2.0f),
		Ogre::CVector(0.0f, 0.0f, 0.0f), 
		Ogre::CVector(0.0f, 0.0f, 1.0f));
}

void CModel::InitCommon(CMPQFile &f)
{
	// This data is needed for both VBO and non-VBO cards.
	m_pVertices = new Ogre::CVector[m_ModelHeader.nVertices];
	m_pNormals = new Ogre::CVector[m_ModelHeader.nVertices];
	m_nNumVertex = m_ModelHeader.nVertices;

	// Correct the data from the model, so that its using the Y-Up axis mode.
	for (size_t i = 0; i < m_ModelHeader.nVertices; i++)
	{
		m_pOrigVertices[i].pos = FixCoordSystem(m_pOrigVertices[i].pos);
		m_pOrigVertices[i].normal = FixCoordSystem(m_pOrigVertices[i].normal);
		m_pVertices[i] = m_pOrigVertices[i].pos;
		m_pNormals[i] = m_pOrigVertices[i].normal;

		float len = m_pOrigVertices[i].pos.Length();
		if (len > m_fRadius)
		{ 
			m_fRadius = len;
		}
	}

	// indices - allocate space, too
	CModelView *view = (CModelView*)(f.getBuffer() + m_ModelHeader.ofsViews);
	uint16 *indexLookup = (uint16*)(f.getBuffer() + view->ofsIndex);
	uint16 *triangles = (uint16*)(f.getBuffer() + view->ofsTris);
	m_nNumIndex = view->nTris;
	m_pIndices = new uint16[m_nNumIndex];
	for (size_t i = 0; i < m_nNumIndex; i++) 
	{
		m_pIndices[i] = indexLookup[triangles[i]];
	}

	// texcoord...
	m_pTexcoord = new Ogre::CVector2D[m_ModelHeader.nVertices];
	for (size_t i = 0; i < m_ModelHeader.nVertices; i++) 
		m_pTexcoord[i] = m_pOrigVertices[i].texcoords;

	// texture...
	SAFE_DELETE_ARRAY(m_pTextureID);
	CModelTextureDef *texdef = (CModelTextureDef*)(f.getBuffer() + m_ModelHeader.ofsTextures);
	if (m_ModelHeader.nTextures) 
	{
		if (m_ModelHeader.nTextures <= 32) 
		{
			m_nNumTexture = m_ModelHeader.nTextures;
			m_pTextureID = new int[m_ModelHeader.nTextures];
			for (size_t i = 0; i < m_ModelHeader.nTextures; i++) 
			{
				char texname[256];
				if (texdef[i].type == 0)
				{
					strncpy(texname, (const char*)f.getBuffer() + texdef[i].nameOfs, texdef[i].nameLen);
					texname[texdef[i].nameLen] = 0;
					std::string path(texname);
					m_pTextureID[i] = g_TextureManager.Add(texname);
				}
				else
				{
					m_pTextureID[i] = -1;
					m_nSpecialTextures[i] = texdef[i].type;

					if (texdef[i].type <= 32)
					{
						m_bUseReplaceTextures[texdef[i].type] = true;
					}

					if (texdef[i].type == 3) 
					{
						// a fix for weapons with type-3 textures.
						m_nReplaceTextures[3] = g_TextureManager.Add("Item\\ObjectComponents\\Weapon\\ArmorReflect4.BLP");
					}
				}
			}
		}
	}

	// init colors
	if (m_ModelHeader.nColors) 
	{
		m_pColors = new CModelColor[m_ModelHeader.nColors];
		CModelColorDef *colorDefs = (CModelColorDef*)(f.getBuffer() + m_ModelHeader.ofsColors);
		for (size_t i = 0; i < m_ModelHeader.nColors; i++) 
			m_pColors[i].Init(f, colorDefs[i], m_pnGlobalSequences);
	}

	// init transparency
	int16 *transLookup = (int16*)(f.getBuffer() + m_ModelHeader.ofsTransparencyLookup);
	if (m_ModelHeader.nTransparency) 
	{
		m_pTransparency = new CModelTransparency[m_ModelHeader.nTransparency];
		CModelTransDef *trDefs = (CModelTransDef*)(f.getBuffer() + m_ModelHeader.ofsTransparency);
		for (size_t i = 0; i < m_ModelHeader.nTransparency; i++) 
		{
			m_pTransparency[i].Init(f, trDefs[i], m_pnGlobalSequences);
		}
	}

	// attachments
	// debug code here
	if (m_ModelHeader.nAttachments)
	{
		CModelAttachmentDef *attachments = (CModelAttachmentDef*)(f.getBuffer() + m_ModelHeader.ofsAttachments);
		for (size_t i = 0; i < m_ModelHeader.nAttachments; i++) 
		{
			CModelAttachment att;
			att.model = this;
			att.init(f, attachments[i], m_pnGlobalSequences);
			atts.push_back(att);
		}
	}

	if (m_ModelHeader.nAttachLookup) 
	{
		int16 *p = (int16*)(f.getBuffer() + m_ModelHeader.ofsAttachLookup);
		for (size_t i=0; i<m_ModelHeader.nAttachLookup; i++)
		{
			attLookup[i] = p[i];
		}
	}

	// render ops
	CModelGeoset *ops = (CModelGeoset*)(f.getBuffer() + view->ofsSub);
	CModelTexUnit *tex = (CModelTexUnit*)(f.getBuffer() + view->ofsTex);
	uint16 *texlookup = (uint16*)(f.getBuffer() + m_ModelHeader.ofsTexLookup);
	CModelRenderFlags *renderFlags = (CModelRenderFlags*)(f.getBuffer() + m_ModelHeader.ofsTexFlags);
	int16 *texunitlookup = (int16*)(f.getBuffer() + m_ModelHeader.ofsTexUnitLookup);

	m_pbShowGeosets = new bool[view->nSub];
	for (size_t i=0; i<view->nSub; i++)
	{
		m_vecGeoset.push_back(ops[i]);
		m_pbShowGeosets[i] = true;
	}

	for (int i = 0; i < view->nTex; i++)
	{
		CModelRenderPass pass;
		size_t geoset = tex[i].op;
		pass.geoset = (int)geoset;
		pass.tex = texlookup[tex[i].textureid];
		pass.indexStart = ops[geoset].istart;
		pass.indexCount = ops[geoset].icount;
		pass.vertexStart = ops[geoset].vstart;
		pass.vertexEnd = pass.vertexStart + ops[geoset].vcount;
		
		pass.opacity = transLookup[tex[i].transid];
		pass.color = tex[i].colorIndex;
		CModelRenderFlags &rf = renderFlags[tex[i].flagsIndex];
		pass.noZWrite = (pass.blendmode > 1);
		pass.useEnvMap = (texunitlookup[tex[i].texunit] == -1) && ((rf.flags & 0x10) !=0) && rf.blend>2;
		pass.order = tex[i].order;
		pass.blendmode = rf.blend;
		pass.p = ops[geoset].v.z;
		m_vecPass.push_back(pass);
	}

	std::sort(m_vecPass.begin(), m_vecPass.end());
}

void CModel::Draw(Ogre::CMatrix &matAtt)
{
	if (m_bAnimated)
	{
		Animate(m_nCurrentAnim, matAtt);
	}

	DrawModel();
}

float CModel::GetRadius()
{
	return m_fRadius;
}

Ogre::CMatrix CModel::SetupAtt(int id)
{
	int l = attLookup[id];
	if (l>-1)
		atts[l].setup();

	return atts[l].m_matAtt;
}

void CModel::Animate(int anim, Ogre::CMatrix &matAtt)
{
	if (m_nNumVertex == 0)
	{
		return;
	}

	int t = 0;
	if (m_pAnim)
	{
		CModelAnimation &a = m_pAnim[anim];
		int tmax = (a.timeEnd - a.timeStart);
		
		if (tmax==0)
		{
			tmax = 1;
		}
		else
		{
			//static DWORD dwLastTime = HQ_TimeGetTime();
			DWORD dwCurTime = HQ_TimeGetTime();
			t = a.timeStart + (dwCurTime - dwLastTime);
			// if (t > a.timeEnd) 会造成闪烁的问题 
			
			// 见一次不一定在范围内，还是会闪烁
			//if (t >= a.timeEnd)
			//{
			//	t -= (a.timeEnd - a.timeStart);
			//	dwLastTime = HQ_TimeGetTime();
			//}

			// 确保在范围内
			while (t >= a.timeEnd)
			{
				t -= (a.timeEnd - a.timeStart);
				dwLastTime = HQ_TimeGetTime();
			}
		}
	}
	
	m_nFrame = t;

	if (m_bAnimBones)
	{
		CalcBones(anim, t, matAtt);
	}

	if (true)
	{
		if (m_bAnimGeometry)
		{
			CModelVertex *ov = m_pOrigVertices;
			for (size_t i = 0; i < m_ModelHeader.nVertices; ++i,++ov) 
			{
				Ogre::CVector v;

				for (size_t b = 0; b < 4; b++)
				{
					int nWeight = ov->weights[b];
					if (nWeight>0) 
					{
						Ogre::CVector vPos = ov->pos;
						int nBoneId = ov->bones[b];
						Ogre::CMatrix mMatrix = m_pBone[nBoneId].m_matBone;
						Ogre::CVector tv = mMatrix * vPos;
						//tv = matAtt * tv;
						v += tv * ((float)nWeight / 255.0f);
					}
				}

				m_pVertices[i] = Ogre::CVector(-v.x, v.y, v.z);
			}
		}
		else
		{
			CModelVertex *ov = m_pOrigVertices;
			for (size_t i = 0; i < m_ModelHeader.nVertices; ++i,++ov) 
			{
				Ogre::CVector v = matAtt * ov->pos;
				m_pVertices[i] = Ogre::CVector(-v.x, v.y, v.z);
			}
		}
	}

	CModelDrawVertex *pModelVertex = NULL;
	m_pVertexBuffer->Lock(0, 0, (void**)&pModelVertex, 0);
	if (pModelVertex != NULL)
	{
		for (int nVertex = 0; nVertex < m_nNumVertex; nVertex++, pModelVertex++)
		{
			pModelVertex->pos = m_pVertices[nVertex];
		}
	}
	m_pVertexBuffer->Unlock();
}

void CModel::CalcBones(int anim, int time, Ogre::CMatrix &matAtt)
{
	// Reset all bones to 'false' which means they haven't been animated yet.
	for (size_t i = 0; i < m_ModelHeader.nBones; i++)
	{
		m_pBone[i].m_bCalc = false;
	}

	// Character specific bone animation calculations.
	if (m_CharModelDetails.isChar)
	{

	}
	else
	{
		for (int i = 0; i < m_BoneLookup[BONE_ROOT]; i++)
		{
			m_pBone[i].CalcMatrix(m_pBone, anim, time);
		}
	}

	// Animate everything thats left with the 'default' animation
	for (size_t i = 0; i < m_ModelHeader.nBones; i++) 
	{
		m_pBone[i].CalcMatrix(m_pBone, anim, time);
	}
}

int CModel::GetTextureID(int nChunk)
{
	if (nChunk < 0 || (nChunk > m_vecPass.size() - 1))
	{
		return -1;
	}

	CModelRenderPass &pass = m_vecPass[nChunk];

	int nBindTex = -1;
	if (m_nSpecialTextures[pass.tex]==-1) 
		nBindTex = m_pTextureID[pass.tex];
	else 
		nBindTex = m_nReplaceTextures[m_nSpecialTextures[pass.tex]];

	return nBindTex;
}

int CModel::GetOutputTexID(int nChunk)
{
	if (nChunk < 0 || (nChunk > m_vecOutputPass.size() - 1))
	{
		return -1;
	}

	CModelRenderPass &pass = m_vecOutputPass[nChunk];

	int nBindTex = -1;
	if (m_nSpecialTextures[pass.tex]==-1) 
		nBindTex = m_pTextureID[pass.tex];
	else 
		nBindTex = m_nReplaceTextures[m_nSpecialTextures[pass.tex]];

	return nBindTex;
}

void CModel::DrawModel()
{
	if (m_nNumVertex == 0)
	{
		return;
	}

	for (int nTex = 0; nTex < 8; nTex++)
	{
		g_pDevice->SetTexture(nTex, NULL);
		g_pDevice->SetSamplerState(nTex, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		g_pDevice->SetSamplerState(nTex, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	}

	g_pDevice->SetVertexDeclaration(m_pVertexDecl);
	g_pDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(CModelDrawVertex));
	g_pDevice->SetIndices(m_pIndexBuffer);

	for (int i = 0; i < m_vecPass.size(); i++)
	{
		CModelRenderPass &pass = m_vecPass[i];

		if (pass.Setup(this))
		{
			int nBindTex = -1;
			if (m_nSpecialTextures[pass.tex]==-1) 
				nBindTex = m_pTextureID[pass.tex];
			else 
				nBindTex = m_nReplaceTextures[m_nSpecialTextures[pass.tex]];

			LPDIRECT3DTEXTURE9 pTex = g_TextureManager.FindD3D9Texture(nBindTex);
			if (pTex == NULL)
			{
				g_pDevice->SetTexture(0, NULL);
			}
			else
			{
				g_pDevice->SetTexture(0, pTex);
			}

			HRESULT hr = g_pDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST,
				0,
				pass.vertexStart, 
				pass.vertexEnd - pass.vertexStart,
				pass.indexStart,
				pass.indexCount / 3 );
			if (FAILED(hr))
			{
				qCritical("CModel DrawModel DrawIndexedPrimitive");
			}
		}
	}
}

bool CModelRenderPass::IsRender(CModel *pModel)
{
	//Ogre::CVector4 oColor;
	//oColor.w = 1.0f;

	//if (pModel->m_pColors)
	//{
	//	if (color != -1)
	//	{
	//		Ogre::CVector c = pModel->m_pColors[color].color.getValue(pModel->m_nCurrentAnim, pModel->m_nFrame);
	//		float o = pModel->m_pColors[color].opacity.getValue(pModel->m_nCurrentAnim, pModel->m_nFrame);
	//		oColor.w = o;
	//	}
	//}

	//if (opacity != -1)
	//{
	//	oColor.w *= pModel->m_pTransparency[opacity].trans.getValue(
	//		pModel->m_nCurrentAnim, pModel->m_nFrame);
	//}

	//if (!(oColor.w > 0))
	//{
	//	return false;
	//}

	return true;
}

bool CModelRenderPass::Setup(CModel *pModel)
{
	if (pModel->m_pbShowGeosets[geoset])
	{
		Ogre::CVector4 oColor;
		oColor.w = 1.0f;

		if (pModel->m_pColors)
		{
			if (color != -1)
			{
				Ogre::CVector c = pModel->m_pColors[color].color.getValue(pModel->m_nCurrentAnim, pModel->m_nFrame);
				float o = pModel->m_pColors[color].opacity.getValue(pModel->m_nCurrentAnim, pModel->m_nFrame);
				oColor.w = o;
			}
		}

		if (opacity != -1)
		{
			oColor.w *= pModel->m_pTransparency[opacity].trans.getValue(
				pModel->m_nCurrentAnim, pModel->m_nFrame);
		}

		if (!(oColor.w > 0))
		{
			return false;
		}

		switch(blendmode)
		{
		case BM_OPAQUE:
			{
				g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
				g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			}
			break;
		case BM_TRANSPARENT:
			{
				g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
				g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, true);
				g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
				g_pDevice->SetRenderState(D3DRS_ALPHAREF, (DWORD)(0.7f * 255) );
			}
			break;
		case BM_ADDITIVE:
			{
				g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
				g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			}
			break;
		case BM_ADDITIVE_ALPHA:
			{
				g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			}
			break;
		case BM_MODULATEX2:
			{
				g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
				g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
			}
			break;
		default:
			{
				g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
				g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
				g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			}
			break;
		}

		if (useEnvMap)
		{
			g_pDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX,
				D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR  | 1 );
		}
		else
		{
			g_pDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0);
		}

		g_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		return true;
	}

	return false;
}

//--------------------------------
//----------CMpqTexture--------------
//--------------------------------
CMpqTexture::CMpqTexture(std::string name)
{
	m_strName = name;
	m_nWidth = 0;
	m_nHeight = 0;
	m_bCompressed = false;
}

CMpqTexture::~CMpqTexture()
{

}

//--------------------------------
//-------CMpqTextureManager----------
//--------------------------------
CMpqTextureManager::CMpqTextureManager()
{
	nIDCount = 0;
}

CMpqTextureManager::~CMpqTextureManager()
{

}

LPDIRECT3DTEXTURE9 CMpqTextureManager::FindD3D9Texture(int nID)
{
	if (m_mapTextureID.find(nID) != m_mapTextureID.end())
	{
		return m_mapTextureID[nID];
	}

	return NULL;
}

CMpqTexture* CMpqTextureManager::FindTexture(int nID)
{
	if (m_mapTexture.find(nID) != m_mapTexture.end())
	{
		return m_mapTexture[nID];
	}

	return NULL;
}

int CMpqTextureManager::Add(std::string name)
{
	int id = -1;

	// if the item already exists, return the existing ID
	if (m_mapName.find(name) != m_mapName.end())
	{
		id = m_mapName[name];
		return id;
	}

	CMpqTexture *tex = new CMpqTexture(name);
	if (tex)
	{
		bool bLoad = LoadBLP(tex);
		if (bLoad)
		{
			m_mapTexture[nIDCount] = tex;
			m_mapName[name] = nIDCount;
			return nIDCount;
		}
	}

	return id;
}

void CMpqTextureManager::Del(int nTexID)
{
	TextureIDMap::iterator iterTexID = m_mapTextureID.find(nTexID);
	if (iterTexID != m_mapTextureID.end())
	{
		LPDIRECT3DTEXTURE9 pD3DTex = iterTexID->second;
		SAFE_RELEASE(pD3DTex);
	}

	TextureMap::iterator iterTex = m_mapTexture.find(nTexID);
	if (iterTex != m_mapTexture.end())
	{
		CMpqTexture *pTex = iterTex->second;
		SAFE_DELETE(pTex);
	}

	NameMap::iterator iterName = m_mapName.begin();
	for (; iterName != m_mapName.end(); iterName++)
	{
		if(nTexID == iterName->second)
		{
			break;
		}
	}

	m_mapName.erase(iterName);
	m_mapTexture.erase(iterTex);
	m_mapTextureID.erase(iterTexID);
}

bool CMpqTextureManager::LoadBLP(CMpqTexture *tex)
{
	int offsets[16], sizes[16], w=0, h=0, type=0;
	int format = 0;
	char attr[4];

	CMPQFile f(tex->m_strName.c_str());
	if (f.isEof())
	{
		qCritical("Error: Could not load the texture '%s'", tex->m_strName.c_str());
		return false;
	} 
	else 
	{
		qDebug("Loading texture: %s", tex->m_strName.c_str());
	}

	f.seek(4);
	f.read(&type,4);
	f.read(attr,4);
	f.read(&w,4);
	f.read(&h,4);
	f.read(offsets,4*16);
	f.read(sizes,4*16);

	tex->m_nWidth = w;
	tex->m_nHeight = h;

	bool hasmipmaps = true;
	int mipmax = hasmipmaps ? 16 : 1;

	if (attr[0] == 2)
	{
		format = COMPRESSED_DXT1;
		int blocksize = 8;

		if (attr[1] == 8)
		{
			format = COMPRESSED_DXT3;
			blocksize = 16;
		}

		if (attr[1] == 8 && attr[2] == 7)
		{
			format = COMPRESSED_DXT5;
			blocksize = 16;
		}

		std::vector<unsigned char*> vecBuf;
		std::vector<int> vecSize;
		int nRealMipNum = 0;

		tex->m_bCompressed = true;
		unsigned char *buf = new unsigned char[sizes[0]];
		for (int i=0; i < mipmax; i++)
		{
			if (w==0) 
				w = 1;

			if (h==0) 
				h = 1;

			if (offsets[i] && sizes[i]) 
			{
				f.seek(offsets[i]);
				f.read(buf,sizes[i]);

				int size = ((w+3)/4) * ((h+3)/4) * blocksize;
				
				unsigned char *tempBuf = new unsigned char[size];
				memcpy((void*)tempBuf, (void*)buf, size);
				vecBuf.push_back(tempBuf);
				vecSize.push_back(size);
				nRealMipNum++;
			} 
			else 
				break;

			w >>= 1;
			h >>= 1;
		}

		CreateD3D9Texture(tex->m_nWidth, tex->m_nHeight, nRealMipNum, format, vecSize, vecBuf);
		
		for (int nBuf = 0; nBuf < vecBuf.size(); nBuf++)
		{
			SAFE_DELETE_ARRAY(vecBuf[nBuf]);
		}
		vecBuf.clear();
		vecSize.clear();
		SAFE_DELETE_ARRAY(buf);
	}
	else if (attr[0] == 1)
	{
		unsigned int pal[256];
		f.read(pal,1024);

		unsigned char *buf = new unsigned char[sizes[0]];
		unsigned int *buf2 = new unsigned int[w * h];
		unsigned int *p = NULL;
		unsigned char *c = NULL, *a = NULL;

		int alphabits = attr[1];
		bool hasalpha = alphabits!=0;

		tex->m_bCompressed = false;

		std::vector<unsigned int*> vecBuf;
		std::vector<int> vecSize;
		int nRealMipNum = 0;
		for (int i=0; i<mipmax; i++) 
		{
			if (w==0) 
				w = 1;

			if (h==0) 
				h = 1;

			if (offsets[i] && sizes[i]) 
			{
				f.seek(offsets[i]);
				f.read(buf,sizes[i]);

				int cnt = 0;
				int alpha = 0;

				p = buf2;
				c = buf;
				a = buf + w*h;
				for (int y=0; y<h; y++) 
				{
					for (int x=0; x<w; x++)
					{
						unsigned int k = pal[*c++];

						k = ((k&0x00FF0000)>>16) | ((k&0x0000FF00)) | ((k& 0x000000FF)<<16);

						if (hasalpha)
						{
							if (alphabits == 8) 
							{
								alpha = (*a++);
							} 
							else if (alphabits == 1) 
							{
								alpha = (*a & (1 << cnt++)) ? 0xff : 0;
								if (cnt == 8) 
								{
									cnt = 0;
									a++;
								}
							}
						}
						else 
						{
							alpha = 0xff;
						}

						k |= alpha << 24;
						*p++ = k;
					}
				}

				// gen texture...
				//unsigned int *tempBuf = new unsigned int[sizes[i]];
				//memcpy((void*)tempBuf, (void*)buf2, sizes[i] * 4);
				unsigned int *tempBuf = new unsigned int[w * h * 4];
				memcpy((void*)tempBuf, (void*)buf2, w * h * 4);
				vecBuf.push_back(tempBuf);
				//vecSize.push_back(sizes[i] * 4);
				vecSize.push_back( w * h * 4);
				nRealMipNum++;
			}
			else
				break;

			w >>= 1;
			h >>= 1;
		}

		CreateD3D9Texture2(tex->m_nWidth, tex->m_nHeight, nRealMipNum, UNCOMPRESSED_RGBA8, vecSize, vecBuf);

		for (int nBuf = 0; nBuf < vecBuf.size(); nBuf++)
		{
			SAFE_DELETE_ARRAY(vecBuf[nBuf]);
		}
		vecBuf.clear();
		vecSize.clear();

		SAFE_DELETE_ARRAY(buf2);
		SAFE_DELETE_ARRAY(buf);
	}

	f.close();

	return true;
}

void CMpqTextureManager::CreateD3D9Texture2(int width, 
						int height,
						int numMip,
						int format,
						std::vector<int> vecSize, 
						std::vector<unsigned int*> vecBuf)
{
	D3DFORMAT d3dFormat = D3DFMT_A8R8G8B8;
	if (format == UNCOMPRESSED_RGBA8)
	{
		d3dFormat = D3DFMT_A8R8G8B8;
	}

	LPDIRECT3DTEXTURE9 pD3D9Texture = NULL;
	HRESULT hr = g_pDevice->CreateTexture(width, height, numMip, 0, 
		d3dFormat, D3DPOOL_MANAGED, &pD3D9Texture, NULL);
	if (FAILED(hr))
	{
		qCritical("CMpqTextureManager CreateD3D9Texture CreateTexture");
		return;
	}

	for (int i = 0; i < numMip; i++)
	{
		D3DLOCKED_RECT lockedrect;
		hr = pD3D9Texture->LockRect(i, &lockedrect, NULL, 0);
		if (FAILED(hr))
		{
			qCritical("CMpqTextureManager CreateD3D9Texture LockRect");
			return;
		}

		memcpy((void*)lockedrect.pBits, (void*)vecBuf[i], vecSize[i]);
		hr = pD3D9Texture->UnlockRect(i);
		if (FAILED(hr))
		{
			qCritical("CMpqTextureManager CreateD3D9Texture UnlockRect");
			return;
		}
	}

	nIDCount++;
	m_mapTextureID[nIDCount] = pD3D9Texture;
}

void CMpqTextureManager::CreateD3D9Texture(int width, int height,
								int numMip,
								int format, 
								std::vector<int> vecSize, 
								std::vector<unsigned char*> vecBuf)
{
	D3DFORMAT d3dFormat = D3DFMT_DXT1;
	if (format == COMPRESSED_DXT1)
	{
		d3dFormat = D3DFMT_DXT1;
	}
	else if (format == COMPRESSED_DXT3)
	{
		d3dFormat = D3DFMT_DXT3;
	}
	else if (format == COMPRESSED_DXT5)
	{
		d3dFormat = D3DFMT_DXT5;
	}

	LPDIRECT3DTEXTURE9 pD3D9Texture = NULL;
	HRESULT hr = g_pDevice->CreateTexture(width, height, numMip, 0, 
			d3dFormat, D3DPOOL_MANAGED, &pD3D9Texture, NULL);
	if (FAILED(hr))
	{
		qCritical("CMpqTextureManager CreateD3D9Texture CreateTexture");
		return;
	}

	for (int i = 0; i < numMip; i++)
	{
		D3DLOCKED_RECT lockedrect;
		hr = pD3D9Texture->LockRect(i, &lockedrect, NULL, 0);
		if (FAILED(hr))
		{
			qCritical("CMpqTextureManager CreateD3D9Texture LockRect");
			return;
		}

		memcpy((void*)lockedrect.pBits, (void*)vecBuf[i], vecSize[i]);
		hr = pD3D9Texture->UnlockRect(i);
		if (FAILED(hr))
		{
			qCritical("CMpqTextureManager CreateD3D9Texture UnlockRect");
			return;
		}
	}

	nIDCount++;
	m_mapTextureID[nIDCount] = pD3D9Texture;
}

void CMpqTextureManager::GetPixel(unsigned char *buf, int nTexID)
{
	LPDIRECT3DTEXTURE9 pD3D9Texture = FindD3D9Texture(nTexID);
	CMpqTexture *pTexture = FindTexture(nTexID);
	if (pD3D9Texture && pTexture)
	{
		D3DLOCKED_RECT lockedrect;
		HRESULT hr = pD3D9Texture->LockRect(0, &lockedrect, NULL, 0);
		if (FAILED(hr))
		{
			qCritical("CMpqTextureManager GetPixel LockRect");
			return;
		}

		int size = pTexture->m_nWidth * pTexture->m_nHeight * 4;
		memcpy((void*)buf, (void*)lockedrect.pBits, size);
		hr = pD3D9Texture->UnlockRect(0);
		if (FAILED(hr))
		{
			qCritical("CMpqTextureManager GetPixel UnlockRect");
			return;
		}
	}
}

void CMpqTextureManager::ComposePixel(unsigned char *buf, int width, int height, int& nTexID)
{
	HRESULT hr;
	LPDIRECT3DTEXTURE9 pD3D9Texture = pD3D9Texture = FindD3D9Texture(nTexID);
	if (pD3D9Texture == NULL)
	{
		hr = g_pDevice->CreateTexture(width, height, 1, 0, 
			D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pD3D9Texture, NULL);
		
		nIDCount++;
		m_mapTextureID[nIDCount] = pD3D9Texture;
		nTexID = nIDCount;
	}

	if (FAILED(hr))
	{
		qCritical("CMpqTextureManager CreateD3D9Texture CreateTexture");
		return;
	}

	if (pD3D9Texture)
	{
		D3DLOCKED_RECT lockedrect;
		HRESULT hr = pD3D9Texture->LockRect(0, &lockedrect, NULL, 0);
		if (FAILED(hr))
		{
			qCritical("CMpqTextureManager GetPixel LockRect");
			return;
		}

		int size = width * height * 4;
		memcpy((void*)lockedrect.pBits, (void*)buf, size);
		hr = pD3D9Texture->UnlockRect(0);
		if (FAILED(hr))
		{
			qCritical("CMpqTextureManager GetPixel UnlockRect");
			return;
		}
	}
}

//---------------------------------
//-------CModelAttachment----------
//---------------------------------
void CModelAttachment::init(CMPQFile &f, CModelAttachmentDef &mad, int *global)
{
	pos = FixCoordSystem(mad.pos);
	bone = mad.bone;
	id = mad.id;
	m_matAtt.Unit();
}

void CModelAttachment::setup()
{
	Ogre::CMatrix m = model->m_pBone[bone].m_matBone;
	m_matAtt = m;

	Ogre::CMatrix matTrans;
	matTrans.Unit();
	matTrans._14 = pos.x;
	matTrans._24 = pos.y;
	matTrans._34 = pos.z;

	m_matAtt = m_matAtt * matTrans;
}