/**********************************************************************
 *<
	FILE: PointCache.h

	DESCRIPTION:	Base class for OSM and WSM

	CREATED BY:

	HISTORY:

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include "PointCache.h"
#include "IAssetAccessor.h"
#include "IPathConfigMgr.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define ENABLE_DEBUGPRINT
#else
// Uncomment for DebugPrints in release builds
//#define ENABLE_DEBUGPRINT
#endif

#ifdef ENABLE_DEBUGPRINT
#pragma message("Debug prints are enabled!")
#define DEBUGPRINT	DebugPrint
#else
void DoNothing(...) { }
#define DEBUGPRINT	DoNothing
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

#if MAX_RELEASE <= 4200
#define DO_MISSING_FILES_PLCB
#endif

// Missing file handling //////////////////////////////////////////////////////////////////////////

#ifdef DO_MISSING_FILES_PLCB

#define MAX_MISSING_FILES	20

static Tab<TSTR*> missingFilenames;

// gather list of all missing cache files
class GetMissingFilesPLCB : public PostLoadCallback
{
public:
	GetMissingFilesPLCB(PointCache* pc)
	{
		m_pc = pc;
	}

	void proc(ILoad* iload)
	{
		TCHAR* fname;
		HANDLE tempFile = NULL;
		Interval iv;

		if (m_pc->m_pblock)
		{
			m_pc->m_pblock->GetValue(pc_cache_file,0,fname,iv);
			if (m_pc->IsValidFName(fname))
			{
				//if file is missing
				tempFile = m_pc->OpenCacheForRead(fname);
				if (tempFile)
				{
					m_pc->CloseFile(tempFile);
					tempFile = NULL;
				} else {
					TSTR* name = new TSTR(fname);

					//see if file is already in list
					bool alreadyInList = false;
					for (int i = 0; i < missingFilenames.Count(); i++)
					{
						if (*name == *missingFilenames[i]) alreadyInList = true;
					}

					//add to list
					if (!alreadyInList)
					{
						missingFilenames.SetCount(missingFilenames.Count()+1);
						missingFilenames[missingFilenames.Count()-1] = name;
					}

					m_pc->SetError(ERROR_CACHEMISSING);
				}
			}
		}

		delete this;
	}

private:
	PointCache* m_pc;
};

// post notice of all cache files that are missing
class NotifyMissingFilesPLCB : public PostLoadCallback
{
public:
	int Priority()
	{
		return 6;
	}

	void proc(ILoad *iload)
	{
		if (missingFilenames.Count())
		{
			Interface* iCore = GetCOREInterface();
			if (iCore)
			{
				if (iCore->IsNetServer())
				{
					for (int i = 0; i < missingFilenames.Count(); i++)
					{
						iCore->Log()->LogEntry(	SYSLOG_ERROR,
												DISPLAY_DIALOG,
												_T("Point Cache"),
												_T("Unable to find cache file(s): %s\n"),
												missingFilenames[i]->data());
					}
				} else {
					TSTR error = GetString(IDS_NOTIFY_MISSING_FILES);
					TSTR errorTitle = GetString(IDS_CLASS_NAME);
					int cnt = (missingFilenames.Count() < MAX_MISSING_FILES) ? missingFilenames.Count() : MAX_MISSING_FILES;
					for (int i = 0; i < cnt; i++)
					{
						error += missingFilenames[i]->data();
						error += "\n";
					}
					if (missingFilenames.Count() > MAX_MISSING_FILES)
						error += "...\n";
					MessageBox(iCore->GetMAXHWnd(),error,errorTitle,MB_OK|MB_ICONERROR);
				}
			}
			for (int i = 0; i < missingFilenames.Count(); i++)
				delete missingFilenames[i];
			missingFilenames.ZeroCount();
		}
		delete this;
	}
};

#endif // DO_MISSING_FILES_PLCB

// PointCache util stuff //////////////////////////////////////////////////////////////////////////

#define DELETE_BUFF(buff) if (buff) { delete[] buff; buff = NULL; }

TSTR GetLastErrorString()
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);

	TSTR err;
	err.printf("%s", (LPCTSTR)lpMsgBuf);

	LocalFree(lpMsgBuf);

	return err;
}

// private namespace
namespace
{
	class sMyEnumProc : public DependentEnumProc 
	{
	public :
		virtual int proc(ReferenceMaker *rmaker); 
		INodeTab Nodes;              
	};

	int sMyEnumProc::proc(ReferenceMaker *rmaker) 
	{ 
		if (rmaker->SuperClassID()==BASENODE_CLASS_ID)    
		{
			Nodes.Append(1, (INode **)&rmaker);  
			return DEP_ENUM_SKIP;
		}

		return DEP_ENUM_CONTINUE;
	}
}

// Static objects/
static PointCacheInstances g_PointCacheInstances;


// PointCache /////////////////////////////////////////////////////////////////////////////////////

IObjParam* PointCache::m_iCore = NULL;

PointCache::PointCache() :
	m_pblock			(NULL),
	m_hWnd				(NULL),
	m_hCacheFile		(NULL),

	m_firstSample		(NULL),
	m_sampleA			(NULL),
	m_sampleB			(NULL),
	m_samplea			(NULL),
	m_sampleb			(NULL),

	m_preloadBuffer		(NULL),
	m_recordBuffer		(NULL),

	m_fileVersion		(POINTCACHE_FILEVERSION),
	m_numPoints			(0),

	m_errorFlags		(ERROR_CACHEMISSING),
	m_doLoad			(true),
	m_tempDisable		(false),
	m_recordMode		(false),
	m_PTSPointCache		(NULL),
	m_modifierVersion	(POINTCACHE_MODIFIERVERSION)
{
	sprintf(m_signature,"POINTCACHE2");

	RegisterNotification(NotifyDelete, this, NOTIFY_SYSTEM_PRE_NEW);
	RegisterNotification(NotifyDelete, this, NOTIFY_FILE_PRE_OPEN);
	RegisterNotification(NotifyDelete, this, NOTIFY_SYSTEM_PRE_RESET);
//	RegisterNotification(NotifyDelete, this, NOTIFY_SCENE_PRE_DELETED_NODE);
}

PointCache::~PointCache()
{
	if(m_PTSPointCache)
	{
		delete m_PTSPointCache;
		m_PTSPointCache = NULL;
	}
	UnRegisterNotification(NotifyDelete, this, NOTIFY_SYSTEM_PRE_NEW);
	UnRegisterNotification(NotifyDelete, this, NOTIFY_FILE_PRE_OPEN);
	UnRegisterNotification(NotifyDelete, this, NOTIFY_SYSTEM_PRE_RESET);
//	UnRegisterNotification(NotifyDelete, this, NOTIFY_SCENE_PRE_DELETED_NODE);

	CloseCacheFile(true);
}

// Animatable /////////////////////////////////////////////////////////////////////////////////////

void PointCache::GetClassName(TSTR& s)
{
	s = GetString(IDS_CLASS_NAME);
}

void PointCache::DeleteThis()
{
	delete this;
}

int PointCache::NumSubs()
{
	return 1;
}

Animatable* PointCache::SubAnim(int i)
{
	return m_pblock;
}

TSTR PointCache::SubAnimName(int i)
{
	return GetString(IDS_PARAMS);
}

class PointCacheAssetAccessor : public IAssetAccessor	{
public:

	PointCacheAssetAccessor(PointCache* aPointCache);

	// path accessor functions
	virtual MaxSDK::Util::Path GetPath() const;
	virtual void SetPath(const MaxSDK::Util::Path& aNewPath);
	virtual bool IsInputAsset() const ;

	// asset client information
	virtual int GetAssetType() const ;
	virtual const TCHAR* GetAssetDesc() const;
	virtual const TCHAR* GetAssetClientDesc() const ;
	virtual bool IsAssetPathWritable() const;

protected:
	PointCache* mPointCache;
};

PointCacheAssetAccessor::PointCacheAssetAccessor(PointCache* aPointCache) : 
mPointCache(aPointCache)
{

}

MaxSDK::Util::Path PointCacheAssetAccessor::GetPath() const	{
	TCHAR *fname = NULL;
	Interval iv;
	mPointCache->m_pblock->GetValue(pc_cache_file,0,fname,iv);
	return MaxSDK::Util::Path(fname);
}


void PointCacheAssetAccessor::SetPath(const MaxSDK::Util::Path& aNewPath)	{
	mPointCache->m_pblock->SetValue(
		pc_cache_file, 
		0, 
		const_cast<TCHAR*>(aNewPath.GetCStr()));
}

bool PointCacheAssetAccessor::IsInputAsset() const	{
	return true;
}

int PointCacheAssetAccessor::GetAssetType() const	{
	return IAssetAccessor::kAnimationAsset;
}

const TCHAR* PointCacheAssetAccessor::GetAssetDesc() const	{
	return NULL;
}
const TCHAR* PointCacheAssetAccessor::GetAssetClientDesc() const		{
	return NULL;
}
bool PointCacheAssetAccessor::IsAssetPathWritable() const	{
	return true;
}

void PointCache::EnumAuxFiles(NameEnumCallback& nameEnum, DWORD flags)
{
	if ((flags&FILE_ENUM_CHECK_AWORK1)&&TestAFlag(A_WORK1)) return; // LAM - 4/11/03

	if(flags & FILE_ENUM_ACCESSOR_INTERFACE)	{
		PointCacheAssetAccessor accessor(this);
		if(!accessor.GetPath().IsEmpty())	{
			IEnumAuxAssetsCallback* callback = static_cast<IEnumAuxAssetsCallback*>(&nameEnum);
			callback->DeclareAsset(accessor);
		}
	}
	else	{
		TCHAR *fname = NULL;
		Interval iv;
		m_pblock->GetValue(pc_cache_file,0,fname,iv);
		if(NULL != fname) 
		{
			MaxSDK::Util::Path cachePath(fname);
			IPathConfigMgr::GetPathConfigMgr()->RecordInputAsset(
				cachePath,
				nameEnum, 
				flags);
		}

		Modifier::EnumAuxFiles( nameEnum, flags ); // LAM - 4/21/03

	}

}

int PointCache::NumParamBlocks()
{
	return 1;
}

IParamBlock2* PointCache::GetParamBlock(int i)
{
	return m_pblock;
}

IParamBlock2* PointCache::GetParamBlockByID(BlockID id)
{
	return (m_pblock->ID() == id) ? m_pblock : NULL;
}

// ReferenceMaker /////////////////////////////////////////////////////////////////////////////////

int PointCache::NumRefs()
{
	return 1;
}

RefTargetHandle PointCache::GetReference(int i)
{
	return m_pblock;
}

void PointCache::SetReference(int i, RefTargetHandle rtarg)
{
	m_pblock = (IParamBlock2*)rtarg;
}

RefResult PointCache::NotifyRefChanged
(
	Interval changeInt, RefTargetHandle hTarget,
	PartID& partID,  RefMessage message
) {
	return REF_SUCCEED;
}

// BaseObject /////////////////////////////////////////////////////////////////////////////////////

CreateMouseCallBack* PointCache::GetCreateMouseCallBack()
{
	return NULL;
}

BOOL PointCache::ChangeTopology()
{
	return FALSE;
}

BOOL PointCache::HasUVW()
{
	return TRUE;
}

void PointCache::SetGenUVW(BOOL sw)
{
	if (sw==HasUVW()) return;
}






// Modifier ///////////////////////////////////////////////////////////////////////////////////////

Interval PointCache::LocalValidity(TimeValue t)
{
	int playType;
	m_pblock->GetValue(pc_playback_type,t,playType,FOREVER);
	switch (playType)
	{
		case playback_original:
		{
			int tpf = GetTicksPerFrame();

			TimeValue startTime = (TimeValue)m_cacheStartFrame * tpf;

			float frameLength = (m_cacheNumSamples-1) / (1.0f/m_realCacheSampleRate);
			TimeValue endTime = startTime + (TimeValue)(tpf * frameLength);

			if (t < startTime)
				return Interval(TIME_NegInfinity, startTime);
			else if (t > endTime)
				return Interval(endTime, TIME_PosInfinity);
			else
				return Interval(t,t);
		}
		case playback_start:
		{
			int tpf = GetTicksPerFrame();

			TimeValue startTime = (TimeValue)m_pblock->GetFloat(pc_playback_start, 0) * tpf;

			float frameLength = (m_cacheNumSamples-1) / (1.0f/m_realCacheSampleRate);
			TimeValue endTime = startTime + (TimeValue)(tpf * frameLength);

			if (t < startTime)
				return Interval(TIME_NegInfinity, startTime);
			else if (t > endTime)
				return Interval(endTime, TIME_PosInfinity);
			else
				return Interval(t,t);
		}
		case playback_range:
		{
			int tpf = GetTicksPerFrame();
			Interval iv(t,t);

			float sf, ef;
			m_pblock->GetValue(pc_playback_start,t,sf,FOREVER);
			m_pblock->GetValue(pc_playback_end,t,ef,FOREVER);
			if (ef < sf)
			{
				float tmp = sf;
				sf = ef;
				ef = tmp;
			}
			TimeValue s = int(sf * tpf);
			TimeValue e = int(ef * tpf);


			int ort;
			m_pblock->GetValue(pc_playback_before,0,ort,FOREVER);
			if (ort != ort_hold)
				s = TIME_NegInfinity;
			m_pblock->GetValue(pc_playback_after,0,ort,FOREVER);
			if (ort != ort_hold)
				e = TIME_PosInfinity;

			if (t > e)
				iv.Set(e+1,TIME_PosInfinity);
			else if (t < s)
				iv.Set(TIME_NegInfinity,s-1);

			return iv;
		}
		case playback_graph:
		{
			Interval iv = FOREVER;

			float v;
			m_pblock->GetValue(pc_playback_frame,t,v,iv);

			return iv;
		}
	}

	Interval iv(t,t);
	return iv;
}

ChannelMask PointCache::ChannelsUsed()
{
	return (PART_GEOM | PART_TOPO);
}

ChannelMask PointCache::ChannelsChanged()
{
	return GEOM_CHANNEL;
}

void PointCache::ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node)
{
#ifdef _DEBUG
	{
		TCHAR* fname = m_pblock->GetStr(pc_cache_file, 0);
		if (_tcsicmp(fname, _T("c:\\foo.pc2"))==0)
		{
			int stophere = 1;
		}
		DEBUGPRINT("PointCache::ModifyObject %s\n", fname);
	}
#endif

	Object* obj = os->obj;
	m_numPoints = obj->NumPoints();

	// check if applied to more than one object
    sMyEnumProc dep;
	DoEnumDependents(&dep);
	if (dep.Nodes.Count()>1)
	{
		SetError(ERROR_INSTANCE);
	} else {
		ClearError(ERROR_INSTANCE);
	}

	int loadType = (GetCOREInterface()->InSlaveMode()) ?	m_pblock->GetInt(pc_load_type_slave, 0) :
															m_pblock->GetInt(pc_load_type, 0);

	if (m_recordMode)
	{
		DEBUGPRINT("PointCache::ModifyObject: In record mode at time %d\n", t);

		if ((m_hCacheFile) && (m_recordTime == t))
		{
			if (m_numPoints == m_cacheNumPoints)
			{
				for (int i = 0; i < m_cacheNumPoints; i++)
				{
					m_recordBuffer[i] = os->obj->GetPoint(i);
				}

				DWORD numWrite;
				DEBUGPRINT("PointCache::ModifyObject: Writing File: m_hCacheFile: %d\n", m_hCacheFile);
				BOOL res = WriteFile(m_hCacheFile, m_recordBuffer, sizeof(Point3)*m_cacheNumPoints, &numWrite, NULL);
				if (!res)
				{
					DEBUGPRINT("Failed: %s\n", GetLastErrorString());
				}
				//TODO: if !res, handle error
			}
			else
			{
				// The point count changed underneath us... so skip dumping data.  The outer record loop will
				// see this and deal with the rest...
			}
		}
		++m_recordTime;
	}
	else if (!m_tempDisable && m_doLoad)
	{
		TCHAR* fname = m_pblock->GetStr(pc_cache_file, 0);

		if (IsValidFName(fname))
		{
			int interpType = m_pblock->GetInt(pc_interp_type, 0);
			BOOL relativeOffset = m_pblock->GetInt(pc_relative, 0);
			float strength = m_pblock->GetFloat(pc_strength, t);
			// Go into file opening loop if:
			// - we're pre-loading and haven't actually loaded anything yet, or
			// - we're not pre-loading and the cacheFile needs to be opened still

			if(m_fileType==ePTS)
			{
				m_hCacheFile = NULL;
				if(m_PTSPointCache==NULL)
					m_PTSPointCache = new OLDPointCacheBase();
				float startTime = m_pblock->GetFloat(pc_playback_start, 0);

				int error = m_PTSPointCache->PerformPTSMod(fname,startTime,relativeOffset,strength,t,
					m_cacheNumPoints,m_cacheSampleRate,m_cacheNumSamples,mc,os,node);
				m_realCacheSampleRate = m_cacheSampleRate; //we use this and it's the same for old files mz defect #806998
				m_cacheStartFrame = startTime;
				if(error==2)
					SetError(ERROR_NUMPOINTS);
				else if(error==3)
					SetError(ERROR_CACHEINVALID);
				else if (error==0)
				{
					ClearError(ERROR_CACHEMISSING);
					ClearError(ERROR_NUMPOINTS);
					ClearError(ERROR_CACHEINVALID);
				}
				Interval iv = LocalValidity(t);
				iv = iv & os->obj->ChannelValidity(t, GEOM_CHAN_NUM);
				os->obj->UpdateValidity(GEOM_CHAN_NUM, iv);
				return;
			}
			else if (
				((loadType == load_preload) && !m_preloadBuffer) ||
				((loadType != load_preload) && !m_hCacheFile)
			) {
				m_hCacheFile = OpenCacheForRead(fname);
				//read header and first frame
				if (m_hCacheFile)
				{
					ClearError(ERROR_CACHEMISSING);

					if (ReadHeader())
					{
						ClearError(ERROR_CACHEINVALID);

						// try to preload cache if desired...
						if (loadType == load_preload)
						{
							FreeBuffers();
							GetPointCacheManager().PreLoadPointCache(this);

							CloseFile(m_hCacheFile);
							m_hCacheFile = NULL;
						}

						if ((loadType == load_preload) && m_preloadBuffer)
						{
							m_firstSample =
							m_sampleA = m_sampleB =
							m_samplea = m_sampleb = m_preloadBuffer;
						} else {
							FreeBuffers();

							m_sampleA = new Point3[m_cacheNumPoints];
							m_sampleB = new Point3[m_cacheNumPoints];
							m_firstSample = new Point3[m_cacheNumPoints];

							if (interpType == interp_cubic)
							{
								m_samplea = new Point3[m_cacheNumPoints];
								m_sampleb = new Point3[m_cacheNumPoints];
							}

							//load first frame
							ClearError(ERROR_NO_DEFORM_IN_FILE);
							if(ReadSample(0, m_firstSample)==false)
							{
								SetError(ERROR_NO_DEFORM_IN_FILE);
								CloseCacheFile(true);
							}
						}
					} else {
						SetError(ERROR_CACHEINVALID);

						CloseCacheFile(true);
					}
				} else {
					SetError(ERROR_CACHEMISSING);
				}
			}

			BOOL applyToSpline = m_pblock->GetInt(pc_apply_to_spline, 0);

			//load up cache data
			if (
				// worlds ugliest if statement...
				(
					// check that all our normal samples are allocated...
					(m_hCacheFile || m_preloadBuffer) && m_firstSample && m_sampleA && m_sampleB
				) &&
				(
					// ...and if using cubic interpolation, check that our extra samples are allocated...
					(interpType == interp_cubic) ? (m_samplea && m_sampleb) : true
				) &&
				(
					// ...and lastly check that the points count matches
					(!applyToSpline && (m_numPoints == m_cacheNumPoints)) ||
					(applyToSpline && (m_numPoints == m_cacheNumPoints*3))
				)
			) {
				ClearError(ERROR_NUMPOINTS);

				float fIndex = GetSampleIndex(t);
				//fix for fIndex near integer but less than, we want to make sure it is equal to one!
				float iIndex = (float)(int) fIndex;
				if((iIndex-fIndex)<1e-6f)
					fIndex =iIndex;
				fIndex = fmodf(fIndex, (float)m_cacheNumSamples);
				if (fIndex < 0.0f) fIndex += m_cacheNumSamples;

				int indexA = (int)fmodf(fIndex, (float)m_cacheNumSamples);
				int indexB = (int)fmodf(float(indexA+1), (float)m_cacheNumSamples);
				float perc = fIndex - (int)fIndex;

				DEBUGPRINT("t: %d, indexA: %d, indexB: %d, perc: %f\n", t, indexA, indexB, perc);

				// load up the buffers based on the computed indexes
				if (m_preloadBuffer)
				{
					m_sampleA = m_preloadBuffer + (indexA * m_cacheNumPoints);
					m_sampleB = m_preloadBuffer + (indexB * m_cacheNumPoints);
				} else {
					ReadSample(indexA, m_sampleA);

					if (indexB == indexA)
					{
						m_sampleB = m_sampleA;
					} else {
						ReadSample(indexB, m_sampleB);
					}
				}

				bool useSubObjectSelection = (obj->GetSubselState() != 0) && (!m_pblock->GetInt(pc_apply_to_whole_object));

				// now apply the samples
				switch (interpType)
				{
					case interp_linear:
					{
						for (int i=0; i<m_numPoints; ++i)
						{
							if (applyToSpline)
							{
								if (((i-1) % 3) != 0) // check if i is a knot point
									continue; // if not, bail (don't mess with tangents)
								else
									i = int(float(i-1) / 3.0f); // otherwise, turn knot index into point index
							}

							if (i < m_cacheNumPoints)
							{
								Point3 p = obj->GetPoint(i);

								Point3 newPoint = m_sampleA[i] + (m_sampleB[i] - m_sampleA[i]) * perc;

								float selWeight = (useSubObjectSelection) ? obj->PointSelection(i) : 1.0f;

								if (relativeOffset)
								{
									p += (newPoint - m_firstSample[i]) * strength * selWeight;
								} else {
									p += (newPoint - p) * strength * selWeight;
								}

								obj->SetPoint(i, p);
							}
						}
						break;
					}
					case interp_cubic:
					{
						// using cubic, so we need two extra samples...
						int indexa = indexA - 1;
						if (indexa < 0) indexa = 0; else if (indexa > (m_cacheNumSamples-1)) indexa = (m_cacheNumSamples-1);
						int indexb = indexB + 1;
						if (indexb < 0) indexb = 0; else if (indexb > (m_cacheNumSamples-1)) indexb = (m_cacheNumSamples-1);

						// read our extra samples (but only if really necessary)
						if (m_preloadBuffer)
						{
							m_samplea = m_preloadBuffer + (indexa * m_cacheNumPoints);
							m_sampleb = m_preloadBuffer + (indexb * m_cacheNumPoints);
						} else {
							if (indexa == indexA)
							{
								m_samplea = m_sampleA;
							} else {
								ReadSample(indexa, m_samplea);
							}

							if (indexb == indexB)
							{
								m_sampleb = m_sampleB;
							} else {
								ReadSample(indexb, m_sampleb);
							}
						}

						// ...and deform the object points
						for (int i=0; i<m_numPoints; i++)
						{
							if (applyToSpline)
							{
								if (((i-1) % 3) != 0) // check if i is a knot point
									continue; // if not, bail (don't mess with tangents)
								else
									i = int(float(i-1) / 3.0f); // otherwise, turn knot index into point index
							}

							if (i < m_cacheNumPoints)
							{
								Point3 p = obj->GetPoint(i);

								Point3 P = (m_sampleb[i] - m_sampleB[i]) - (m_samplea[i] - m_sampleA[i]);
								Point3 Q = (m_samplea[i] - m_sampleA[i]) - P;
								Point3 R = m_sampleB[i] - m_samplea[i];
								Point3 S = m_sampleA[i];
								float a = powf(perc, 3);
								float b = powf(perc, 2);

								Point3 newPoint = (P * a) + (Q * b) + (R * perc) + S;

								float selWeight = (useSubObjectSelection) ? obj->PointSelection(i) : 1.0f;

								if (relativeOffset)
								{
									p += (newPoint - m_firstSample[i]) * strength * selWeight;
								} else {
									p += (newPoint - p) * strength * selWeight;
								}

								obj->SetPoint(i, p);
							}
						}
						break;
					}
				}

				obj->PointsWereChanged();
			} else {
				if(m_hCacheFile)
					SetError(ERROR_NUMPOINTS);
			}
		} else {
			SetError(ERROR_CACHEMISSING);
		}

		if (loadType == load_persample)
		{
			CloseCacheFile(true);
		}
	}
	if (m_hWnd&&m_pblock->GetMap()) m_pblock->GetMap()->Invalidate(); //fix for 791963 mz. Also don't want to completely update the UI, but just want to invalidate it
												  //so the key brackets are correctly redrawn

	Interval iv = LocalValidity(t);
	iv = iv & os->obj->ChannelValidity(t, GEOM_CHAN_NUM);
	os->obj->UpdateValidity(GEOM_CHAN_NUM, iv);
}

Class_ID PointCache::InputType()
{
	return defObjectClassID;
}

// Handle old versions of the modifier ////////////////////////////////////////////////////////////

class FixUpModifierVersionPLCB : public PostLoadCallback
{
public:
	PointCache* m_pc;
	FixUpModifierVersionPLCB(PointCache* pc) { m_pc = pc; }

	void proc(ILoad *iload)
	{
		if (m_pc->m_modifierVersion < 1)
		{
			int oldType = m_pc->m_pblock->GetInt(pc_playback_type, 0);
			int newType = playback_original;
			switch (oldType)
			{
				case old_playback_range: newType = playback_range; break;
				case old_playback_graph: newType = playback_graph; break;
			}
			m_pc->m_pblock->SetValue(pc_playback_type, 0, newType);
		}

		if (m_pc->m_modifierVersion < 2)
		{
			// old versions were always clamped... new default is to be not clamped
			m_pc->m_pblock->SetValue(pc_clamp_graph, 0, TRUE);
		}

		if (m_pc->m_modifierVersion < 3)
		{
			int loadType = (m_pc->m_pblock->GetInt(pc_preload_cache)) ? load_preload : load_stream;
			m_pc->m_pblock->SetValue(pc_load_type, 0, loadType);
		}

		if (m_pc->m_modifierVersion < 4)
		{
			m_pc->m_pblock->SetValue(pc_load_type_slave, 0, load_persample);
		}

		m_pc->m_modifierVersion = POINTCACHE_MODIFIERVERSION;
	}
};

#define MODIFIERVERSION_CHUNK	0x0000
#define ENABLE_CHUNK		0x2120
#define ENABLEINRENDER_CHUNK		0x2130


class ErrorPLCB : public PostLoadCallback {
	public:
		PointCache *obj;
		ErrorPLCB(PointCache *o) {obj=o;}
		void proc(ILoad *iload);
	};

void ErrorPLCB::proc(ILoad *iload)
{
	TCHAR *fname = NULL;
	Interval iv;
	obj->m_pblock->GetValue(pc_cache_file,0,fname,iv);
	obj->SetFileType(fname);

	if(obj->m_pblock && 
		( (obj->m_pcacheEnabled && obj->m_pcacheEnabledInRender) ||
		  (obj->m_pcacheEnabled && !obj->m_pcacheEnabledInRender) ) )
	{
		if (fname)
		{
			MaxSDK::Util::Path cachePath(fname);
			IPathConfigMgr* pathMgr = IPathConfigMgr::GetPathConfigMgr();
			if (!pathMgr->SearchForExternalFiles(cachePath)) 
			{
				obj->SetError(ERROR_CACHEMISSING);
			}
			
		}	
	}
	delete this;
}


IOResult PointCache::Load(ILoad* iload)
{
	ULONG nb;
	IOResult res;

	res = Modifier::Load(iload);
    if (res != IO_OK) return res;

	int loadVer = 0;

	m_pcacheEnabled = TRUE;
	m_pcacheEnabledInRender = TRUE;



	while (IO_OK==(res=iload->OpenChunk()))
	{
		switch(iload->CurChunkID())
		{
			case MODIFIERVERSION_CHUNK:
			{
				iload->Read(&loadVer, sizeof(int), &nb);
				break;
			}
			case ENABLE_CHUNK:
				iload->Read(&m_pcacheEnabled,sizeof(m_pcacheEnabled), &nb);
				break;
			case ENABLEINRENDER_CHUNK:
				iload->Read(&m_pcacheEnabledInRender,sizeof(m_pcacheEnabledInRender), &nb);
				break;


		}
		iload->CloseChunk();
		if (res!=IO_OK)
			return res;
	}

	if (loadVer != POINTCACHE_MODIFIERVERSION)
	{
		m_modifierVersion = loadVer;
		iload->RegisterPostLoadCallback(new FixUpModifierVersionPLCB(this));
	}
	iload->RegisterPostLoadCallback(new ErrorPLCB(this));	

#ifdef DO_MISSING_FILES_PLCB //these are older missing files routines that are no longer used. Left in code in case we want to revisit.
	iload->RegisterPostLoadCallback(new GetMissingFilesPLCB(this));
	iload->RegisterPostLoadCallback(new NotifyMissingFilesPLCB());
#endif

	return IO_OK;
}

IOResult PointCache::Save(ISave* isave)
{
	ULONG nb;
	IOResult res;

	res = Modifier::Save(isave);
	if (res != IO_OK) return res;

	m_pcacheEnabled = IsEnabled();
	m_pcacheEnabledInRender = IsEnabledInRender();

	isave->BeginChunk(ENABLE_CHUNK);
	isave->Write(&m_pcacheEnabled,sizeof(m_pcacheEnabled),&nb);
	isave->EndChunk();

	isave->BeginChunk(ENABLEINRENDER_CHUNK);
	isave->Write(&m_pcacheEnabledInRender,sizeof(m_pcacheEnabledInRender),&nb);
	isave->EndChunk();


	isave->BeginChunk(MODIFIERVERSION_CHUNK);
	isave->Write(&m_modifierVersion, sizeof(m_modifierVersion), &nb);
	isave->EndChunk();

	return IO_OK;
}

// PointCache /////////////////////////////////////////////////////////////////////////////////////

void PointCache::SetError(DWORD error)
{
	m_errorFlags |= error;
}

void PointCache::ClearError(DWORD error)
{
	m_errorFlags &= ~error;
}

bool PointCache::TestError(DWORD error)
{
	return ((m_errorFlags & error) ? true : false);
}

void PointCache::UpdateUI()
{
	TCHAR *fname;
	m_pblock->GetValue(pc_cache_file,0,fname,FOREVER);
	TSTR str(fname);
	SetFileName(str);
	if (m_hWnd) m_pblock->GetMap()->Invalidate();
}

bool PointCache::IsValidFName(TCHAR* fname)
{
	if (fname && (*fname != _T('\0')) )
		return true;
	else
		return false;
}

bool PointCache::ReadHeader()
{
	SetFilePointer(m_hCacheFile, 0, NULL, FILE_BEGIN);

	DWORD numRead;
	if (!ReadFile(m_hCacheFile, &m_cacheSignature, sizeof(m_cacheSignature), &numRead, NULL) || numRead == 0) return false;
	if (strcmp(m_signature,m_cacheSignature) != 0) return false;

	if (!ReadFile(m_hCacheFile, &m_cacheVersion, sizeof(m_cacheVersion), &numRead, NULL) || numRead == 0) return false;
	if (!ReadFile(m_hCacheFile, &m_cacheNumPoints, sizeof(m_cacheNumPoints), &numRead, NULL) || numRead == 0) return false;
	if (!ReadFile(m_hCacheFile, &m_cacheStartFrame, sizeof(m_cacheStartFrame), &numRead, NULL) || numRead == 0) return false;
	if (!ReadFile(m_hCacheFile, &m_cacheSampleRate, sizeof(m_cacheSampleRate), &numRead, NULL) || numRead == 0) return false;
	if (!ReadFile(m_hCacheFile, &m_cacheNumSamples, sizeof(m_cacheNumSamples), &numRead, NULL) || numRead == 0) return false;

	TimeValue tpf = GetTicksPerFrame();

	TimeValue recordInc = (m_cacheSampleRate * tpf);
	TimeValue endFrame = m_cacheStartFrame*tpf + (recordInc * m_cacheNumSamples); //cachestart frame is in frames need to make sure in ticks
	TimeValue numFrames = 	endFrame/tpf - m_cacheStartFrame ;
	m_realCacheSampleRate = (float)numFrames/(float)(m_cacheNumSamples);

	return true;
}

HANDLE PointCache::OpenCacheForRead(TCHAR* fname)
{
	DEBUGPRINT("PointCache::OpenCacheForRead: %s\n", fname);

	HANDLE hFile = NULL;
	if (IsValidFName(fname))
	{
		MaxSDK::Util::Path path(fname);
		IPathConfigMgr* pathMgr = IPathConfigMgr::GetPathConfigMgr();
		if (pathMgr->SearchForExternalFiles(path)) 
		{


			g_PointCacheInstances.OpenCacheForRead(this,path.GetCStr());
			hFile = ::CreateFile(	path.GetCStr(),
									GENERIC_READ,
									FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,	// allow file to be overwritten or deleted out from under us!
									0,
									OPEN_EXISTING,
									0,
									NULL);
		}
		else
			hFile = INVALID_HANDLE_VALUE;

		if (hFile == INVALID_HANDLE_VALUE)
		{
			hFile = NULL;
			DEBUGPRINT("\tfail: '%s' - %s\n", fname, GetLastErrorString());
		} else {
			DEBUGPRINT("\tsuccess: %d - '%s'\n", hFile, fname);
			
		}
	} else {
		DEBUGPRINT("\tfail: Invalid fname: '%s'\n", fname);
	}

	return hFile;
}

HANDLE PointCache::OpenCacheForRecord(TCHAR* fname,HWND hWnd)
{
	DEBUGPRINT("PointCache::OpenCacheForRecord: %s\n", fname);

	if(m_fileType==ePTS)
	{
		Interface *iCore = GetCOREInterface();
		if (iCore->IsNetServer())
		{
				iCore->Log()->LogEntry(	SYSLOG_ERROR,
										DISPLAY_DIALOG,
										_T("Point Cache"),
										_T("Can't record .pts files.  Need to save as .pc2"));
			return NULL;
		} 
		else
		{
			TSTR error = GetString(IDS_PTS_TO_PC2);
			TSTR errorTitle = GetString(IDS_CLASS_NAME);
			MessageBox(iCore->GetMAXHWnd(),error,errorTitle,MB_OK|MB_ICONERROR);
			SaveFilePrompt(hWnd);
			m_pblock->GetValue(pc_cache_file,0,fname,FOREVER);

		}
	}

	//first we try to create given the passed in file name.. if that doesn't work we then
	//try to find it using searching for an external path..
	HANDLE hFile;
	MaxSDK::Util::Path path(fname);
	path.ConvertToAbsolute();

	g_PointCacheInstances.OpenCacheForRecord(this,path.GetCStr());

	hFile = ::CreateFile(	path.GetCStr(),
									GENERIC_WRITE,
									0,						// don't share it
									0,
									CREATE_ALWAYS,			// overwrite existing files
									FILE_ATTRIBUTE_ARCHIVE,
									NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		//okay now we search along the 
		DEBUGPRINT("\tfail: but now searching along external path'%s' - %s\n",path.GetCStr(), GetLastErrorString());
		MaxSDK::Util::Path path(fname);		
		IPathConfigMgr* pathMgr = IPathConfigMgr::GetPathConfigMgr();

		if (pathMgr->SearchForExternalFiles(path)) //just use the orignal then
		{
			g_PointCacheInstances.OpenCacheForRecord(this,path.GetCStr());
			hFile = ::CreateFile(	path.GetCStr(),
											GENERIC_WRITE,
											0,						// don't share it
											0,
											CREATE_ALWAYS,			// overwrite existing files
											FILE_ATTRIBUTE_ARCHIVE,
											NULL);
			if (hFile != INVALID_HANDLE_VALUE)
				return hFile;
			//else just print fail and exit
		}
		DEBUGPRINT("\tfail: '%s' - %s\n",path.GetCStr(), GetLastErrorString());
	} else {
		DEBUGPRINT("\tsuccess: %d - '%s'\n", hFile, fname);
	}

	return hFile;
}

bool PointCache::CloseFile(HANDLE hFile)
{
	DEBUGPRINT("PointCache::CloseFile: %d\n", hFile);
	if(hFile)
	{
		if (!CloseHandle(hFile))
		{
			DEBUGPRINT("\tfail: %d - %s\n", hFile, GetLastErrorString());
			return false;
		} else {
			DEBUGPRINT("\tsuccess: %d\n", hFile);
		}

		hFile = NULL;
	}
	return true;
}

bool PointCache::CloseCacheFile(bool notifyFileManager)
{
	FreeBuffers();

	CloseFile(m_hCacheFile);
	m_hCacheFile = NULL;
	if(notifyFileManager==true)
	{
		g_PointCacheInstances.DeletePointCache(this);
	}
	//also handle old point caches
	if(m_PTSPointCache)
	{
		delete m_PTSPointCache;
		m_PTSPointCache = NULL;
	}
	return true;
}

// Checks if t is in in the range defined by [a, b] (inclusive).
static bool InTimeRange(TimeValue a, TimeValue b, TimeValue t)
{
	return (a <= b) ? ( (t >= a) && (t <= b) ) : ( (t >= b) && (t <= a) );
}

//TODO: make sure this returns true/false properly
bool PointCache::RecordToFile(HWND hWnd,bool &canceled)
{
	canceled = false; //assume we aren't.
	DEBUGPRINT("PointCache::RecordToFile\n");

	ClearError(ERROR_POINTCOUNTCHANGED);
	ClearError(ERROR_NONODEFOUND);

	TCHAR *fname;
	m_pblock->GetValue(pc_cache_file,0,fname,FOREVER);
	if (!IsValidFName(fname))
	{
		if(SaveFilePrompt(hWnd)==false)
		{
			canceled = true;
			return false;
		}
		m_pblock->GetValue(pc_cache_file,0,fname,FOREVER);
	}

	if (IsValidFName(fname))
	{
		CloseCacheFile(true);

		// create the cacheFile
		float endFrame;
		m_pblock->GetValue(pc_record_start,0,m_cacheStartFrame,FOREVER);
		m_pblock->GetValue(pc_record_end,0,endFrame,FOREVER);
		m_pblock->GetValue(pc_sample_rate,0,m_cacheSampleRate,FOREVER);

		int tpf = GetTicksPerFrame();
		// Setup variables used to control the record loop.
		TimeValue recordBegin = TimeValue(m_cacheStartFrame * tpf);
		TimeValue recordEnd = TimeValue(endFrame * tpf);
		TimeValue recordInc = TimeValue(m_cacheSampleRate * tpf);
		if (recordBegin > recordEnd) recordInc = -recordInc;	// we're going backwards through time, so loop counter must be flipped
		if (recordBegin == recordEnd) recordEnd += recordInc;	// make sure we're recording at least two frames


		// Update m_numPoints and
		// Update objectstate to current time + 1. (If we didn't do this, and current time == start time, then the first frame wouldn't
		// be recorded because ModifyObject wouldn't get called due to the objectstate not being invalid).
		sMyEnumProc dep;
		DoEnumDependents(&dep);
		if (!dep.Nodes[0])
		{
			SetError(ERROR_NONODEFOUND);
			return false;
		}

		m_tempDisable = true;
		dep.Nodes[0]->EvalWorldState(GetCOREInterface()->GetTime() + 1);
		m_tempDisable = false;

		m_cacheNumPoints = m_numPoints;

		// Count the number of samples to take
		TimeValue sampleTime;
		m_cacheNumSamples = 0;
		for (sampleTime = recordBegin; InTimeRange(recordBegin, recordEnd, sampleTime); sampleTime += recordInc)
		{
			++m_cacheNumSamples;
		}
		TimeValue numFrames = 	endFrame- m_cacheStartFrame ;
		m_realCacheSampleRate = (float)numFrames/(float)(m_cacheNumSamples);

		m_hCacheFile = OpenCacheForRecord(fname,hWnd);
		if (m_hCacheFile == INVALID_HANDLE_VALUE) 
		{
			m_hCacheFile = NULL; //make sure we close it
			return false;
		}

		m_recordBuffer = new Point3[m_cacheNumPoints];
		if (!m_recordBuffer)
		{
			CloseCacheFile(true);
			return false;
		}

		//TODO: breaks if header changes
		DWORD numWrite;
		WriteFile(m_hCacheFile, &m_signature, sizeof(m_signature), &numWrite, NULL);
		WriteFile(m_hCacheFile, &m_fileVersion, sizeof(m_fileVersion), &numWrite, NULL);
		WriteFile(m_hCacheFile, &m_cacheNumPoints, sizeof(m_cacheNumPoints), &numWrite, NULL);
		WriteFile(m_hCacheFile, &m_cacheStartFrame, sizeof(m_cacheStartFrame), &numWrite, NULL);
		WriteFile(m_hCacheFile, &m_cacheSampleRate, sizeof(m_cacheSampleRate), &numWrite, NULL);
		WriteFile(m_hCacheFile, &m_cacheNumSamples, sizeof(m_cacheNumSamples), &numWrite, NULL);

		TSTR name;
		m_recordMode = true;
		bool abort = FALSE;
		int numRecorded = 0;

		for (sampleTime = recordBegin; InTimeRange(recordBegin, recordEnd, sampleTime) && !abort; sampleTime += recordInc)
		{
			m_recordTime = sampleTime;
			dep.Nodes[0]->EvalWorldState(m_recordTime);

			if (m_numPoints == m_cacheNumPoints)
			{
				numRecorded += 1;

				name.printf("Sample %d of %d",numRecorded,m_cacheNumSamples);
				SetWindowText(GetDlgItem(hWnd,IDC_MESSAGE_LABEL),name);

				SHORT iret = GetAsyncKeyState(VK_ESCAPE);
				if (iret == -32767) abort = true;
			}
			else
			{
				abort = true;
				SetError(ERROR_POINTCOUNTCHANGED);
			}
		}

		if (abort)
		{
			// write the correct number of samples back to the file if they cancel
			// TODO: breaks if header changes
			int offset = sizeof(m_signature) + (sizeof(int)*2) + (sizeof(float)*2);
			SetFilePointer(m_hCacheFile, offset, NULL, FILE_BEGIN);
			WriteFile(m_hCacheFile, &numRecorded, sizeof(numRecorded), &numWrite, NULL);
		}

		name.printf(" ");
		SetWindowText(GetDlgItem(hWnd,IDC_MESSAGE_LABEL),name);

		CloseCacheFile(true);
		delete[] m_recordBuffer;
		m_recordBuffer = NULL;

		m_recordMode = false;
		m_doLoad = true;

		NotifyDependents(FOREVER, GEOM_CHANNEL, REFMSG_CHANGE);

		return true; //was returning false if we abort but we still get some frames so we are good!
	} else {
		return false;
	}
}



class PC2HistoryList : public bmmHistoryList
{
public:

	void Init();
};

TCHAR PointCache::m_InitialDir[MAX_PATH];


void PC2HistoryList::Init()
{
	_tcscpy(title,GetString(IDS_CLASS_NAME));
	initDir[0] = 0;
	bmmHistoryList::LoadDefaults();
	
	if (bmmHistoryList::DefaultPath()[0]) 
		_tcscpy(PointCache::m_InitialDir, bmmHistoryList::DefaultPath());
	else _tcscpy(PointCache::m_InitialDir,GetCOREInterface()->GetDir(APP_EXPORT_DIR));

};


class CustomData {
public:
	PC2HistoryList history;
	BOOL save;
	CustomData(BOOL s) {
		save = s; //if save we show the plus button
		history.Init();
		history.LoadDefaults();
	};
};



INT_PTR WINAPI PC2HookProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static CustomData* custom = 0;
	if (message == WM_INITDIALOG)
	{
		//ShowWindow(GetDlgItem(hDlg,IDC_PLUS_BUTTON),(_tcsicmp(GetCurExt(), ARCHEXT)!=0)?SW_SHOW:SW_HIDE); 
		custom = (CustomData*)((OPENFILENAME *)lParam)->lCustData;
		if (custom) {
			custom->history.SetDlgInfo(GetParent(hDlg),hDlg,IDC_HISTORY);
			custom->history.LoadList();
		}

		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		HWND hParent = GetParent(hDlg);
		if ( GetDlgItem(hDlg, IDC_PLUS_BUTTON) ) {
			GetWindowPlacement(GetDlgItem(hParent,IDOK),&wp);
			wp.rcNormalPosition.left += 32;
			wp.rcNormalPosition.right = wp.rcNormalPosition.left + 24;
			SetWindowPlacement(GetDlgItem(hDlg, IDC_PLUS_BUTTON), &wp);
		}
		if(custom->save==TRUE)
			ShowWindow(GetDlgItem(hDlg,IDC_PLUS_BUTTON),TRUE);
		else
			ShowWindow(GetDlgItem(hDlg,IDC_PLUS_BUTTON),FALSE);

			

		// DC - 18/08/05
		//>SetPlusPushed(false);
	} else if (message == WM_COMMAND) {
		if (LOWORD(wParam) == IDC_HISTORY) {
			if (HIWORD(wParam) == CBN_SELCHANGE && custom)
				custom->history.SetPath();
		} 
	}
	else if(message==WM_NOTIFY)
		 {
		     OFNOTIFY *ofnot = (OFNOTIFY *) lParam;
			 NMHDR *nmh = &(ofnot->hdr);

			 if (nmh->code == CDN_INITDONE)
				 custom->history.NewPath(ofnot->lpOFN->lpstrInitialDir);

			 if (nmh->code == CDN_FILEOK)
			 {
				 TCHAR buf[MAX_PATH];
				 if( CommDlg_OpenSave_GetFolderPath(GetParent(hDlg),buf,MAX_PATH) < 1 )
					 buf[0] = _T('\0');
				 custom->history.NewPath(buf);
				 custom->history.SaveList();
			 }
	}
	
	else if (message == WM_DESTROY)
		custom = NULL;
	else if (message == WM_SIZE) {
		// Resize plus button and history combo
		HWND hParent = GetParent(hDlg);
		if ( hParent ) {
			WINDOWPLACEMENT wp;
			wp.length = sizeof(WINDOWPLACEMENT);
			if ( GetDlgItem(hDlg, IDC_PLUS_BUTTON) ) {
				GetWindowPlacement(GetDlgItem(hParent,IDOK),&wp);
				wp.rcNormalPosition.left += 32;
				wp.rcNormalPosition.right = wp.rcNormalPosition.left + 24;
				SetWindowPlacement(GetDlgItem(hDlg, IDC_PLUS_BUTTON), &wp);
			}
		if(custom->save==TRUE)
			ShowWindow(GetDlgItem(hDlg,IDC_PLUS_BUTTON),TRUE);
		else
			ShowWindow(GetDlgItem(hDlg,IDC_PLUS_BUTTON),FALSE);

			
			GetWindowPlacement(GetDlgItem(GetParent(hDlg),IDOK),&wp);
			GetWindowPlacement(GetDlgItem(GetParent(hDlg),IDCANCEL),&wp);
			int right = wp.rcNormalPosition.right;
			GetWindowPlacement(GetDlgItem(hDlg,IDC_HISTORY),&wp);
			wp.rcNormalPosition.right = right;
			SetWindowPlacement(GetDlgItem(hDlg,IDC_HISTORY),&wp);
		}
	}
	return FALSE;
}


bool PointCache::SaveFilePrompt(HWND hWnd)
{
	Interface* iCore = GetCOREInterface();
	if (!iCore) return true;

	OPENFILENAME ofn = {sizeof(OPENFILENAME)};
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400; // has OFN_ENABLEHOOK

	FilterList fl;
	fl.Append(GetString(IDS_PW_CACHEFILES));
	fl.Append(GetString(IDS_CACHE_EXT_WILDCARD));
	TSTR title = GetString(IDS_PW_SAVEOBJECT);

	TCHAR fname[MAX_PATH] = {'\0'};
	ModContextList mcList;
	INodeTab nodes;
	iCore->GetModContexts(mcList,nodes);
	if (nodes.Count() != 0)
	{
		TSTR str(nodes[0]->GetName());

		for (int ii=0; ii<str.Length(); ++ii)
		{
			// TODO: implement these restrictions (looser than the ones below):
			// - do not use a path separator, a character in the range 0 through 31, or
			//   any character explicitly disallowed by the file system. A name can contain
			//   characters in the extended character set (128-255).
			// - disallowed characters: < > : " / \ |
			// - The following reserved words cannot be used as the name of a file:
			//   CON, PRN, AUX, CLOCK$, NUL, COM1, COM2, COM3, COM4, COM5, COM6, COM7, COM8, COM9,
			//   LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, and LPT9. Also, reserved words
			//   followed by an extension - for example, NUL.tx7 - are invalid file names.
			// - Process a path as a null-terminated string. The maximum length for a path, including a
			//   trailing backslash, is given by MAX_PATH.
			char c = str[ii];
			if ((c<'0' || c>'9') &&
				(c<'A' || c>'Z') &&
				(c<'a' || c>'z')
			) str[ii] = '_';
		}

		str += TSTR(".");
		str += GetString(IDS_CACHE_EXT);

		strncpy(fname, str.data(), str.length());
	}

	ofn.hwndOwner       = hWnd;
	ofn.lpstrFilter     = fl;
	ofn.lpstrFile       = fname;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrInitialDir = PointCache::m_InitialDir;
	ofn.Flags           = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_ENABLETEMPLATE |OFN_ENABLEHOOK|OFN_PATHMUSTEXIST;
	ofn.FlagsEx         = OFN_EX_NOPLACESBAR;
	ofn.Flags |= OFN_ENABLESIZING;


	CustomData custom(TRUE);
	ofn.lCustData       = LPARAM(&custom);
	ofn.lpstrDefExt     = GetString(IDS_CACHE_EXT);
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILE_DIALOG);
	ofn.lpfnHook = (LPOFNHOOKPROC)PC2HookProc;
	ofn.hInstance = hInstance;

	ofn.lpstrTitle      = title;

	tryAgain:
	if (GetSaveFileName(&ofn))
	{
		if (DoesFileExist(fname))
		{
			TSTR buf1;
			TSTR buf2 = GetString(IDS_PW_SAVEOBJECT);
			buf1.printf(GetString(IDS_PW_FILEEXISTS),fname);
			if (IDYES!=MessageBox(hWnd,buf1,buf2,MB_YESNO|MB_ICONQUESTION))
				goto tryAgain;

			// Delete original file to avoid loading header
			CloseCacheFile(true);
			remove(fname);
		}
		//save stuff here
		MaxSDK::Util::Path path(fname);
		IPathConfigMgr::GetPathConfigMgr()->NormalizePathAccordingToSettings(path);
		m_pblock->SetValue(pc_cache_file,0,const_cast<TCHAR *>(path.GetCStr()));
		m_fileType = ePC2; //only save out pc2!
	}
	else 
		return false;  //if we cancel return false
	return true;
}


void PointCache::SetFileType(const TCHAR *filename)
{
	m_fileType	= ePC2; 
	//now change it to pts if it is...
	size_t length;
	if(filename && (length= _tcslen(filename))>0)
	{
		if(filename[length-1]!='2') //assume its' an s or S so we are done...
		{
			m_fileType = ePTS;
			if(m_PTSPointCache==NULL)
				m_PTSPointCache = new OLDPointCacheBase();

			m_pblock->SetValue(pc_playback_type,0,playback_original);
			m_pblock->SetValue(pc_load_type,0,load_stream);
		}
		else
		{
			if(m_PTSPointCache)
			{
				delete m_PTSPointCache;
				m_PTSPointCache = NULL;
			}
		}
	}
}


void PointCache::OpenFilePrompt(HWND hWnd)
{
	CloseCacheFile(true);

	// set initial file/dir
	TCHAR fname[MAX_PATH] = {'\0'};
	TCHAR* str = m_pblock->GetStr(pc_cache_file);


	if(str&& _tcslen(str)>0)
	{
		MaxSDK::Util::Path cachePath(str);
		IPathConfigMgr* pathMgr = IPathConfigMgr::GetPathConfigMgr();
		if (pathMgr->SearchForExternalFiles(cachePath)) 
		{
			TSTR what = cachePath.GetString();
			str = what.data();
			if (IsValidFName(str))
				strcpy(fname, str);
		}
	}


	OPENFILENAME ofn = {sizeof(OPENFILENAME)};
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400; // has OFN_ENABLEHOOK

	FilterList fl;
	fl.Append(GetString(IDS_PW_CACHEFILES));
	fl.Append(GetString(IDS_CACHE_EXT_WILDCARD));

	TSTR extensionWildCard; //for old files
	extensionWildCard.printf("*.pts");

	fl.Append( GetString(IDS_OLD_CACHE_FILES));
	fl.Append( _T(extensionWildCard));	


	TSTR title = GetString(IDS_PW_LOADOBJECT);

	ofn.hwndOwner       = hWnd;
 	ofn.lpstrFilter     = fl;
	ofn.lpstrFile       = fname;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrInitialDir = PointCache::m_InitialDir;
	ofn.Flags           = OFN_HIDEREADONLY| OFN_ENABLETEMPLATE |OFN_PATHMUSTEXIST |OFN_ENABLEHOOK|OFN_EXPLORER;
	ofn.FlagsEx         = OFN_EX_NOPLACESBAR;
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILE_DIALOG);
	ofn.lpfnHook = (LPOFNHOOKPROC)PC2HookProc;
	ofn.hInstance = hInstance;

	ofn.Flags |= OFN_ENABLESIZING;

	CustomData custom(FALSE);
	ofn.lCustData       = LPARAM(&custom);

	ofn.lpstrDefExt     = GetString(IDS_CACHE_EXT);
	ofn.lpstrTitle      = title;


	if (GetOpenFileName(&ofn))
	{
		SetFileType(fname);
		MaxSDK::Util::Path path(fname);
		IPathConfigMgr::GetPathConfigMgr()->NormalizePathAccordingToSettings(path);
		TSTR what = path.GetString();
		m_pblock->SetValue(pc_cache_file,0,what.data());
		NotifyDependents(FOREVER, GEOM_CHANNEL, REFMSG_CHANGE);

		ClearError(ERROR_CACHEMISSING);
	}
	else
	{
		DWORD val = CommDlgExtendedError();
		if(val==3)
		{
			int t=2;
		}
	}

	UpdateUI();
}

void PointCache::Unload()
{
	CloseCacheFile(true);
	m_doLoad = false;
	NotifyDependents(FOREVER, GEOM_CHANNEL, REFMSG_CHANGE);
	UpdateUI();
}

void PointCache::Reload()
{
	DEBUGPRINT("PointCache::Reload\n");

	CloseCacheFile(true);
	m_doLoad = true;
	NotifyDependents(FOREVER, GEOM_CHANNEL, REFMSG_CHANGE);
	UpdateUI();
}

bool PointCache::DeleteCacheFile(HWND hWnd)
{
	TCHAR* fname;
	m_pblock->GetValue(pc_cache_file,0,fname,FOREVER);
	Interface* iCore = GetCOREInterface();
	if (iCore)
	{
		HWND maxWnd = iCore->GetMAXHWnd();
		if (IsValidFName(fname))
		{
			TSTR buf1;
			TSTR buf2 = GetString(IDS_DELETE_TITLE);
			buf1.printf(GetString(IDS_DELETE_PROMPT),fname);

			if (IDYES==MessageBox(maxWnd,buf1,buf2,MB_YESNO|MB_ICONEXCLAMATION))
			{
				CloseCacheFile(true);
				return (remove(fname) == 0) ? TRUE : FALSE;
			}
		}
	}
	return FALSE;
}

void PointCache::EnableModsBelow(BOOL enable)
{
	int				i;
	SClass_ID		sc;
	IDerivedObject* dobj;

// return the indexed modifier in the mod chain
	INode *node;
	INodeTab nodes;
	if (!m_iCore)
		{
		sMyEnumProc dep;              
		DoEnumDependents(&dep);
		node = dep.Nodes[0];
		}
	else
		{
		ModContextList mcList;
		m_iCore->GetModContexts(mcList,nodes);
		assert(nodes.Count());
		node = nodes[0];
		}
	BOOL found = TRUE;

// then osm stack
	Object* obj = node->GetObjectRef();
	int ct = 0;

	if ((dobj = node->GetWSMDerivedObject()) != NULL)
		{
		for (i = 0; i < dobj->NumModifiers(); i++)
			{
			Modifier *m = dobj->GetModifier(i);
			BOOL en = m->IsEnabled();
			BOOL env = m->IsEnabledInViews();
			if (!enable)
				{
				if (!found)
					{
					m->DisableMod();
					}
				}
			else 
				{
				if (!found)
					{
					m->EnableMod();
					}
				}
			if (this == dobj->GetModifier(i))
				found = FALSE;

			}
		}

	if ((sc = obj->SuperClassID()) == GEN_DERIVOB_CLASS_ID)
		{
		dobj = (IDerivedObject*)obj;

		while (sc == GEN_DERIVOB_CLASS_ID)
			{
			for (i = 0; i < dobj->NumModifiers(); i++)
				{
				TSTR name;
				Modifier *m = dobj->GetModifier(i);
				m->GetClassName(name);


				BOOL en = m->IsEnabled();
				BOOL env = m->IsEnabledInViews();
				if (!enable)
					{
					if (!found)
						{
						m->DisableMod();
						}
					}
				else 
					{
					if (!found)
						{
						m->EnableMod();
						}
					}
				if (this == dobj->GetModifier(i))
					found = FALSE;


				}
			dobj = (IDerivedObject*)dobj->GetObjRef();
			sc = dobj->SuperClassID();
			}
		}

nodes.DisposeTemporary();

GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
}

void PointCache::NotifyDelete(void* param, NotifyInfo* arg)
{
	PointCache* pc = (PointCache*)param;

	if (pc)
	{
		pc->CloseCacheFile(true);
		pc->m_doLoad = true;
	}
}

float PointCache::GetSampleIndex(TimeValue t)
{
	int playbackType;
	m_pblock->GetValue(pc_playback_type,0,playbackType,FOREVER);
	switch (playbackType)
	{
		case playback_original:
		{
			float startFrame = m_cacheStartFrame;
			float sampleminus = (float) m_cacheNumSamples-1;
			float endFrame = (float)startFrame + ((sampleminus) / (1.0f/m_realCacheSampleRate));
			float curFrame = (float)t / (float)GetTicksPerFrame();
			if (curFrame < startFrame) curFrame = startFrame;
			if (curFrame > endFrame) curFrame = endFrame;
			float normFrame = (float)(curFrame - startFrame) / (float)(endFrame - startFrame);
			return (normFrame * (sampleminus));
		}
		case playback_start:
		{
			float startFrame = m_pblock->GetFloat(pc_playback_start, 0);

			float endFrame = startFrame + ((m_cacheNumSamples-1) / (1.0f/m_realCacheSampleRate));
			float curFrame = (float)t / (float)GetTicksPerFrame();
			if (curFrame < startFrame) curFrame = startFrame;
			if (curFrame > endFrame) curFrame = endFrame;
			float normFrame = (curFrame - startFrame) / (endFrame - startFrame);
			return (normFrame * (m_cacheNumSamples-1));
		}
		case playback_range:
		{
			float sFrame, eFrame;
			m_pblock->GetValue(pc_playback_start,0,sFrame,FOREVER);
			m_pblock->GetValue(pc_playback_end,0,eFrame,FOREVER);

			if (sFrame == eFrame)
				return 0.0f;

			float cFrame = (float)t / (float)GetTicksPerFrame();

			float lo = (sFrame < eFrame) ? sFrame : eFrame;
			float hi = (eFrame > sFrame) ? eFrame : sFrame;
			if (cFrame < lo)
				cFrame = lo;
			else if (cFrame > hi)
				cFrame = hi;

			float normFrame = (cFrame - sFrame) / (eFrame - sFrame);

			float idx = normFrame * (m_cacheNumSamples-1);

			return idx;
		}
		case playback_graph:
		{
			float fCurFrame = m_pblock->GetFloat(pc_playback_frame, t);

			float fIdx = 0.0f;
			if (m_pblock->GetInt(pc_clamp_graph, 0))
			{
				fIdx = fCurFrame * (1.0f / m_realCacheSampleRate);

				if (fIdx > (m_cacheNumSamples-1))
					fIdx = float(m_cacheNumSamples-1);
				else if (fIdx < 0.0f)
					fIdx = 0.0f;
			} else {
				fIdx = fCurFrame * (1.0f / m_realCacheSampleRate);
			}

			return fIdx;
		}
	}

	return 0.0f;
}

int PointCache::ComputeOffset(int index)
{
	//TODO: breaks if header changes
	int offset = sizeof(m_cacheSignature) + (sizeof(int)*2) + (sizeof(float)*2) + (sizeof(int));
	offset += sizeof(Point3) * m_cacheNumPoints * index;
	return offset;
}

bool PointCache::ReadSample(int sampleIndex, Point3* sampleBuffer)
{
	if (m_hCacheFile && m_hCacheFile != INVALID_HANDLE_VALUE)
	{
		int offset = ComputeOffset(sampleIndex);
		SetFilePointer(m_hCacheFile, offset, NULL, FILE_BEGIN);
		DWORD numRead = 0;
		DWORD res = ReadFile(m_hCacheFile, sampleBuffer, sizeof(Point3)*m_cacheNumPoints, &numRead, NULL);
		if (res && numRead != 0)
		{
			return true;
		} else {
			memset((void*)sampleBuffer, 0, sizeof(Point3)*m_cacheNumPoints);
			return false;
		}
	}

	return false;
}

void PointCache::FreeBuffers()
{
	if (m_preloadBuffer)
	{
		GetPointCacheManager().UnLoadPointCache(this);
	} else {
		DELETE_BUFF(m_firstSample)
		DELETE_BUFF(m_sampleA)
		DELETE_BUFF(m_sampleB)
		DELETE_BUFF(m_samplea)
		DELETE_BUFF(m_sampleb)
	}
}





PointCacheInstances::~PointCacheInstances()
{
	Delete();
}

void PointCacheInstances::Delete()
{
	for(int i=0;i<m_fileAndCaches.Count();++i)
	{
		if(m_fileAndCaches[i])
			delete m_fileAndCaches[i];
	}
	m_fileAndCaches.ZeroCount();
}

void PointCacheInstances::DeletePointCache(PointCache *pC)  //happens when cache closes too!
{
	FileAndCaches *fC;
	for(int i=0;i<m_fileAndCaches.Count();++i)
	{
		fC= m_fileAndCaches[i];
		if(fC)
		{
			for(int j=0;j<fC->m_PCs.Count();++j)
			{
				if(fC->m_PCs[j]==pC)
				{
					fC->m_PCs.Delete(j,1);
					if(fC->m_PCs.Count()==0)
					{
						delete fC;
						m_fileAndCaches.Delete(i,1);
					}
					return;
				}
			}
		}
	}
}


void PointCacheInstances::OpenCacheForRead(PointCache *pc, const TCHAR *fileName)
{
	FileAndCaches *fC;
	bool wasAdded = false;
	if(fileName==NULL||_tcslen(fileName)<=0)
		return;
	for(int i=0;i<m_fileAndCaches.Count();++i)
	{
		fC= m_fileAndCaches[i];
		if(fC)
		{
			if(_tcsicmp(fileName,fC->m_fileName)==0)
			{

				for(int j=0;j<fC->m_PCs.Count();++j)
				{
					if(fC->m_PCs[j]==pc)
					{
						return; //same file name.. no need to do anything... and we know it's only one!
					}
				}
				//so doesn't exist here.. we need to add it.. here.. note we don't return since it may still exists elsewhere
				//so we'll need to delete it if it does.
				fC->m_PCs.Append(1,&pc);
				wasAdded = true;
			}
			else
			{
				for(int j=0;j<fC->m_PCs.Count();++j)
				{
					if(fC->m_PCs[j]==pc)
					{
						//it's here but doesn't exist delete it
						fC->m_PCs.Delete(j,1);
						if(fC->m_PCs.Count()==0)
						{
							delete fC;
							m_fileAndCaches.Delete(i,1);
							--i;
							break;
						}
					}
				}
			}
		}
	}
	//okay we got here need to create a new one.
	if(wasAdded==false)
	{
		fC = new FileAndCaches;
		fC->m_PCs.Append(1,&pc);
		_tcscpy(fC->m_fileName,fileName);
		m_fileAndCaches.Append(1,&fC);
	}
}


void PointCacheInstances::OpenCacheForRecord(PointCache *pc, const TCHAR *fileName)
{
	FileAndCaches *fC;
	for(int i=0;i<m_fileAndCaches.Count();++i)
	{
		fC= m_fileAndCaches[i];
		if(fC)
		{
			if(_tcsicmp(fileName,fC->m_fileName)==0)
			{
				for(int j=0;j<fC->m_PCs.Count();++j)
				{
					PointCache *npc = fC->m_PCs[j];
					if(npc!=pc)
						npc->CloseCacheFile(false);
				}
			}
		}
	}
}


