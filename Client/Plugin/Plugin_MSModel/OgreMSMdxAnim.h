

#ifndef __OgreMdxAnim_h__
#define __OgreMdxAnim_h__

#include "OgrePrerequisites.h"
#include "OgreMath.h"
#include "OgreUseful.h"
#include "OgreMSMdxConfig.h"
#include "OgreMSMdx.h"

namespace Ogre
{
	class OgreExport CMSAnimTimeLine
	{
	public:
		CMSAnimTimeLine();
		~CMSAnimTimeLine();

		void Reset();
		DWORD GetStartTime();
		int	GetOneLoopTime();
		int	GetStartFrameID();
		int	GetEndFrameID();
		BOOL IsLoopPlaying();
		void SetLoopPlay();
		
		void Play( DWORD dwStartTime, int nStartFrameID, int nEndFrameID, DWORD dwOneLoopTime );
		int	GetNumLooped( DWORD dwTime );
		float GetCurPlayPercent( DWORD dwTime );
		float GetLoopPlayPercent( DWORD dwTime );
	private:
		DWORD	m_dwStartTime;
		int		m_nStartFrameID;
		int		m_nEndFrameID;
		int		m_nOneLoopTime;
		BOOL	m_bLoopPlaying;
	};

	struct OgreExport SMSPlayTask
	{
		SMSPlayTask();
		~SMSPlayTask();

		char	szActionname[MDX_MAX_NAME];	// 动作名称
		DWORD	dwStartTime;
		DWORD	dwOneLoopTime;			// 播放一遍的时间
		DWORD	dwLoopTimes;			// 播放次数
		DWORD	dwTimeOut;				// 超时，一旦超时，播放完成
		DWORD	dwPlayMode;				// 播放模式
		int		nSingleFrameID;			// 单帧播放模式下，播放帧编号
		int		nBlendFrame0;
		int		nBlendFrame1;
		float	fBlendFactor;
		BOOL	bUpperBody;				// 判断是上半身动作还是下半身动作
		BOOL	bLoopPlay;				// 判断是否是个循环动画
		DWORD	dwNeedBlendingTime;		// 需要和上一个动画进行Blending的时间
	};

	class OgreExport CMSMdxAnim
	{
	public:
		CMSMdxAnim();
		~CMSMdxAnim();

		enum
		{
			eNormalPlayTime = 0,
			eDefaultBlendingTime = 100,
			eMaxLoopTimes	= 0x7fffffff,
			eMaxTimeout		= 0x7fffffff,
			eMaxPlayTask	= 2,
		};

		enum
		{
			eNormalPlay,
			eHoldOnFirstFrame,
			eHoldOnLastFrame,
			eHoldOnHitFrame,
			eSingleFrame,
			eBlendFrame,
			eHideFrame
		};

		void Update( DWORD dwTime, CMatrix* pMatrix, BOOL bUpdateComponent = TRUE );
		BOOL UpdateUpperBodyMode(DWORD dwTime);
		BOOL UpdateUpperBodyAndLowerBodyMode(DWORD dwTime);
		void UpdateComponents(DWORD dwTime, CMatrix* pMatrix);
		BOOL UpdateUpperBodyAndLowerBodyComponent(DWORD dwTime, CMatrix* pMatrix);
		BOOL UpdateUpperBodyComponent(DWORD dwTime, CMatrix* pMatrix);
		BOOL PlayAnim( SMSPlayTask* pTask );
		void RenderGeometry(float fTransparent);
		void RenderSubset(int nSubset, float fTransparent);
		void RenderParticle();
	private:
		BOOL m_bUpperBodyAndLowerBodyMode;
		BOOL m_bUpperBodyBlending;
		DWORD m_dwUpperBodyStartBlendingTime;
		DWORD m_dwUpperBodyBlendingStep;
		float m_fUpperBodyFactor;
		float m_fCurUpperBodyFrame;
		int m_nNumPlayTask;
		SMSPlayTask m_PlayTasks[eMaxPlayTask + 1];	//以前全身的动画任务
		CMSAnimTimeLine	m_UpperBodyAnimTimeLine;

		CMSMdxCfgSequence *m_pSeqUpperBody;
		int	m_nStartFrameId;
		int	m_nEndFrameId;
		DWORD m_dwOneLoopTime;

		CMatrix* m_amatBoneInWorldSpace;
		CMatrix* m_amatBoneInModelSpace;
		CMatrix* m_matSrcBones;
		int m_nFrameId;
		CMSMdx* m_pMdx;
		CMSMdxCfg *m_pMdxCfg;
	};
}

#endif