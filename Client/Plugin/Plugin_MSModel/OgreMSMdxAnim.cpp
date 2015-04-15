

#include "OgreMSMdxAnim.h"

namespace Ogre
{
	//---------------------------------------
	//-------------CMSAnimTimeLine-------------
	//---------------------------------------
	CMSAnimTimeLine::CMSAnimTimeLine()
	{
		m_dwStartTime = 0;
		m_nStartFrameID = 0;
		m_nEndFrameID = 0;
		m_nOneLoopTime = 0;
		m_bLoopPlaying = FALSE;
	}

	CMSAnimTimeLine::~CMSAnimTimeLine()
	{

	}

	void CMSAnimTimeLine::Play( DWORD dwStartTime,
		int nStartFrameID, 
		int nEndFrameID, 
		DWORD dwOneLoopTime )
	{
		m_dwStartTime = dwStartTime;
		m_nStartFrameID = nStartFrameID;
		m_nEndFrameID = nEndFrameID;
		m_nOneLoopTime = dwOneLoopTime;
	}

	void CMSAnimTimeLine::Reset()
	{
		m_dwStartTime = 0;
		m_nStartFrameID = 0;
		m_nEndFrameID = 0;
		m_nOneLoopTime = 0;
		m_bLoopPlaying = FALSE;
	}

	DWORD CMSAnimTimeLine::GetStartTime()
	{
		return m_dwStartTime;
	}

	int CMSAnimTimeLine::GetOneLoopTime()
	{
		return m_nOneLoopTime;
	}

	int	CMSAnimTimeLine::GetStartFrameID()
	{
		return m_nStartFrameID;
	}

	int	CMSAnimTimeLine::GetEndFrameID()
	{
		return m_nEndFrameID;
	}

	BOOL CMSAnimTimeLine::IsLoopPlaying()
	{
		return m_bLoopPlaying;
	}

	void CMSAnimTimeLine::SetLoopPlay()
	{
		m_bLoopPlaying = TRUE;
	}

	int	CMSAnimTimeLine::GetNumLooped( DWORD dwTime )
	{
		DWORD dwPassed = dwTime - m_dwStartTime;
		return dwPassed / (m_nOneLoopTime ? m_nOneLoopTime : 1);
	}

	float CMSAnimTimeLine::GetCurPlayPercent( DWORD dwTime )
	{
		if( m_nOneLoopTime == 0 )
			return 255.0f;

		if( dwTime < m_dwStartTime )
			return 0;

		DWORD dwPassed = dwTime - m_dwStartTime;
		float fPercent = (float)dwPassed / m_nOneLoopTime;
		return fPercent;
	}

	float CMSAnimTimeLine::GetLoopPlayPercent( DWORD dwTime )
	{
		if( m_nOneLoopTime == 0 )
			return 255.0f;

		if( dwTime < m_dwStartTime )
			return 0;

		DWORD dwPassed = dwTime - m_dwStartTime;
		DWORD dwOffset = (dwPassed % m_nOneLoopTime);
		float fPercent = (float)dwOffset / m_nOneLoopTime;
		if( fPercent > 1.0f )
		{
			fPercent = 1.0f;
		}
		
		return fPercent;
	}

	//---------------------------------------
	//-------------SMSPlayTask-----------------
	//---------------------------------------
	SMSPlayTask::SMSPlayTask()
	{
		memset( this, 0, sizeof( SMSPlayTask ) );
	};

	SMSPlayTask::~SMSPlayTask()
	{
	}

	//---------------------------------------
	//-------------CMSMdxAnim------------------
	//---------------------------------------
	CMSMdxAnim::CMSMdxAnim()
	{
		m_bUpperBodyAndLowerBodyMode = false;
		m_bUpperBodyBlending = FALSE;
		m_nNumPlayTask = 0;
		m_dwUpperBodyStartBlendingTime = 0;
		m_dwUpperBodyBlendingStep = 0;
		m_fUpperBodyFactor = 0.0f;
		m_fCurUpperBodyFrame = 0.0f;

		m_pSeqUpperBody = NULL;
		m_nStartFrameId = -1;
		m_nEndFrameId = -1;

		m_amatBoneInWorldSpace = NULL;
		m_amatBoneInModelSpace = NULL;
		m_matSrcBones = NULL;
		m_nFrameId = -1;
		m_pMdx = NULL;
	}

	CMSMdxAnim::~CMSMdxAnim()
	{

	}

	void CMSMdxAnim::Update( DWORD dwTime, CMatrix* pMatrix, BOOL bUpdateComponent )
	{
		bool bUpdate = false;
		if (m_bUpperBodyAndLowerBodyMode)
		{
			bUpdate = UpdateUpperBodyAndLowerBodyMode(dwTime);
		}
		else
		{
			bUpdate = UpdateUpperBodyMode(dwTime);
		}

		if (!bUpdate)
		{
			return;
		}

		if (bUpdateComponent)
		{
			UpdateComponents(dwTime, pMatrix);
		}
	}

	BOOL CMSMdxAnim::UpdateUpperBodyMode(DWORD dwTime)
	{
		DWORD dwStart = HQ_TimeGetTime();
		BOOL bFinish = FALSE;

		if (m_nNumPlayTask == 0)
		{
			return FALSE;
		}

		if( m_bUpperBodyBlending )
		{
			DWORD dwBlendingTime = dwStart - m_dwUpperBodyStartBlendingTime;
			if( dwBlendingTime > m_dwUpperBodyBlendingStep )
			{
				m_bUpperBodyBlending = FALSE;
			}
			else
			{
				m_fUpperBodyFactor = (float)dwBlendingTime / m_dwUpperBodyBlendingStep;
			}
		}
		
		if(!m_bUpperBodyBlending)
		{
			SMSPlayTask* pTask = &m_PlayTasks[0];
			switch( pTask->dwPlayMode )
			{
			case eHoldOnFirstFrame:
				m_fUpperBodyFactor = 0.0f;
				break;
			case eHoldOnLastFrame:
				m_fUpperBodyFactor = 1.0f;
				break;
			case eSingleFrame:
				break;
			case eBlendFrame:
				break;
			case eNormalPlay:
				{
					int nLoopTimes = m_UpperBodyAnimTimeLine.GetNumLooped( dwTime );
					DWORD dwStartTime = m_UpperBodyAnimTimeLine.GetStartTime();
					if ( dwTime < dwStartTime )
					{
						nLoopTimes = 0;
					}
					
					if( dwTime > dwStartTime && nLoopTimes >= pTask->dwLoopTimes )
					{
						bFinish = TRUE;
					}
					
					if( dwTime > dwStartTime && dwTime-dwStartTime >= pTask->dwTimeOut )
					{
						bFinish = TRUE;
					}
					
					if( m_UpperBodyAnimTimeLine.IsLoopPlaying() )
					{
						m_fUpperBodyFactor = m_UpperBodyAnimTimeLine.GetLoopPlayPercent( dwTime );
					}
					else
					{
						m_fUpperBodyFactor = m_UpperBodyAnimTimeLine.GetCurPlayPercent( dwTime );
					}
				}
				break;
			}

			if( bFinish )
			{
				assert( m_nNumPlayTask >= 2 && "task count must >=2" );
				memcpy( &m_PlayTasks[0], &m_PlayTasks[1], (m_nNumPlayTask-1)*sizeof(SMSPlayTask) );
				m_nNumPlayTask--;
				assert( m_nNumPlayTask > 0 && "next task is null" );
				pTask = &m_PlayTasks[0];
				assert( pTask->dwTimeOut > 2000 && "second task timeout too small" );
				PlayAnim( pTask );
				switch( pTask->dwPlayMode )
				{
				case eHoldOnFirstFrame:
					m_fUpperBodyFactor = 0;
					break;
				case eHoldOnLastFrame:
					m_fUpperBodyFactor = 1;
					break;
				}
			}
		}

		return TRUE;
	}

	BOOL CMSMdxAnim::UpdateUpperBodyAndLowerBodyMode(DWORD dwTime)
	{
		return TRUE;
	}

	void CMSMdxAnim::UpdateComponents(DWORD dwTime, CMatrix* pMatrix)
	{
		if (NULL == pMatrix)
		{
			return;
		}

		if (NULL == m_amatBoneInModelSpace)
		{
			return;
		}

		if (NULL == m_amatBoneInWorldSpace)
		{
			return;
		}

		if (NULL == m_pMdx)
		{
			return;
		}

		CMSMdxSkeleton *pSkeleton = m_pMdx->GetSkeleton();
		if (NULL == pSkeleton)
		{
			return;
		}

		int nBoneNum = pSkeleton->GetBoneNum();
		if (nBoneNum == 0)
		{
			return;
		}

		if (m_nFrameId < 0)
		{
			m_nFrameId = 0;
		}

		if (m_bUpperBodyAndLowerBodyMode)
		{
			UpdateUpperBodyAndLowerBodyComponent(dwTime, pMatrix);
		}
		else
		{
			UpdateUpperBodyComponent(dwTime, pMatrix);
		}
	}

	BOOL CMSMdxAnim::PlayAnim( SMSPlayTask* pTask )
	{
		assert( pTask != NULL && "anim play task is null" );
		assert( pTask->szActionname[0] != 0 && "anim play task action name is NULL" );
		assert( pTask->dwTimeOut != 0 && "anim play task time out is 0" );
		assert( pTask->dwStartTime != 0 && "anim play task start time is 0" );

		CMSMdxCfgSequence* pNewSequence = m_pMdxCfg->GetSequence( pTask->szActionname );
		if( !pNewSequence )
		{
			return false;
		}

		if( m_bUpperBodyAndLowerBodyMode )
		{
		}
		else
		{
			if( m_bUpperBodyBlending )
			{
				if( m_pSeqUpperBody )
				{
					m_fCurUpperBodyFrame = m_pSeqUpperBody->GetBlendFrame( 1.0f );
				}
			}

			m_pSeqUpperBody = pNewSequence;
			
			m_nStartFrameId = pNewSequence->m_npStartFrameId.GetNumber();
			m_nEndFrameId = pNewSequence->m_npEndFrameId.GetNumber();

			if( pTask->dwOneLoopTime == 0 )
			{
				int nFrameCount = m_nEndFrameId - m_nStartFrameId + 1;
				m_dwOneLoopTime = (nFrameCount - 1) * ONEFRAMEINTERVAL;
			}
			else
			{
				m_dwOneLoopTime = pTask->dwOneLoopTime;
			}
		}

		if( m_bUpperBodyAndLowerBodyMode )
		{
		}
		else
		{
			m_UpperBodyAnimTimeLine.Reset();
			m_bUpperBodyBlending = TRUE;
			m_dwUpperBodyStartBlendingTime = HQ_TimeGetTime();
			m_dwUpperBodyBlendingStep = pTask->dwNeedBlendingTime;

			if( m_dwUpperBodyBlendingStep == 0)
			{
				m_bUpperBodyBlending = FALSE;
				m_dwUpperBodyStartBlendingTime = 0;
				m_dwUpperBodyBlendingStep = 0;
			}

			m_UpperBodyAnimTimeLine.Play( 
				HQ_TimeGetTime() + m_dwUpperBodyBlendingStep,
				m_nStartFrameId,
				m_nEndFrameId,
				m_dwOneLoopTime );

			if( pTask->bLoopPlay )
			{
				m_UpperBodyAnimTimeLine.SetLoopPlay();
			}
		}

		return TRUE;
	}

	BOOL CMSMdxAnim::UpdateUpperBodyAndLowerBodyComponent(DWORD dwTime, CMatrix* pMatrix)
	{

		return TRUE;
	}

	BOOL CMSMdxAnim::UpdateUpperBodyComponent(DWORD dwTime, CMatrix* pMatrix)
	{
		if (NULL == m_pMdx)
		{
			return FALSE;
		}

		CMSMdxSkeleton *pSkeleton = m_pMdx->GetSkeleton();
		if (NULL == pSkeleton)
		{
			return FALSE;
		}

		if (!m_bUpperBodyBlending)
		{
			m_fCurUpperBodyFrame = m_pSeqUpperBody->GetBlendFrame(m_fUpperBodyFactor);
		}
		m_nFrameId = m_fCurUpperBodyFrame / ONEFRAMEINTERVAL;
		m_pMdx->SetFrameId(m_nFrameId);

		int nBoneNum = pSkeleton->GetBoneNum();
		for (int nBone = 0; nBone < nBoneNum; nBone++)
		{
			CMSMdxBone *pBone = pSkeleton->GetBone(nBone);
			if (NULL == pBone)
			{
				continue;
			}

			m_amatBoneInWorldSpace[nBone] = pBone->GetFrame0Inv() * pBone->GetMatrices( m_nFrameId );
			memcpy( &m_amatBoneInModelSpace[nBone], &m_amatBoneInWorldSpace[nBone], sizeof(CMatrix) );

			m_amatBoneInWorldSpace[nBone].m[0][0] *= -1;
			m_amatBoneInWorldSpace[nBone].m[1][0] *= -1;
			m_amatBoneInWorldSpace[nBone].m[2][0] *= -1;
			m_amatBoneInWorldSpace[nBone].m[3][0] *= -1;

			m_amatBoneInWorldSpace[nBone] = m_amatBoneInWorldSpace[nBone] * (*pMatrix);
		}

		return TRUE;
	}

	void CMSMdxAnim::RenderGeometry(float fTransparent)
	{
		if( m_pMdx->GetGeometry() )
		{
			int nChunkCount = m_pMdx->GetGeometry()->GetChunkCount();

			for( int i = 0; i < nChunkCount; i++ )
			{
				RenderSubset( i, fTransparent );
			} 
		}
	}

	void CMSMdxAnim::RenderSubset(int nSubset, float fTransparent)
	{
		CMSMdxGeoChunk* pSubset = m_pMdx->GetGeometry()->GetChunk(nSubset);
		if( pSubset == NULL )
		{
			return;
		}


	}

	void CMSMdxAnim::RenderParticle()
	{

	}
}