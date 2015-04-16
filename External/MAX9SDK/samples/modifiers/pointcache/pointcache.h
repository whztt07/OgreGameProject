/**********************************************************************
 *<
	FILE: PointCache.h

	DESCRIPTION:	Base class for OSM and WSM

	CREATED BY:

	HISTORY:

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#ifndef __POINTCACHE_H__
#define __POINTCACHE_H__

#include "Max.h"

#include "modstack.h"
#include "macrorec.h"
#include "simpobj.h"

#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "notify.h"

#include "PointCacheManager.h"
#include "IPointCache.h"

extern TCHAR *GetString(int id);

class OLDPointCacheBase;
//funciton that replaces old point caches on load up, with the
//new point cache system.
void ReplaceOldPointCache(OLDPointCacheBase *obj);

#define POINTCACHE_FILEVERSION		1

// Used for handling old versions of .max files
// 1/2 - playback_type was redefined
// 3 - Added pc_load_type
// 4 - Added pc_load_type_slave
#define POINTCACHE_MODIFIERVERSION	4

typedef struct {
	char cacheSignature[12];
	int fileVersion;
	int numPoints;
	float startFrame;
	float sampleRate;
	int numSamples;
} PCHeader;

/*
typedef unsigned char BYTE;

typedef struct {
	char cacheSignature[12];	// header signature
	int fileVersion;			// cache file version
	int numPoints;				// num points per sample
	float startFrame;			// original starting frame
	float sampleRate;			// sample rate (there are (1./sampleRate) samples per frame)
	int numSamples;				// total number of samples in cache file
	BYTE bakeSpace;				// original space back space. 0=local, 1=world
								// this is prolly a dumb way to do this... maybe store TM instead?
} PCHeader2;
*/



//error codes
#define ERROR_NONE			0
#define ERROR_INSTANCE		(1<<0)
#define ERROR_NUMPOINTS		(1<<1)
#define ERROR_CACHEMISSING	(1<<2)
#define ERROR_CACHEINVALID	(1<<3)
#define ERROR_POINTCOUNTCHANGED	(1<<4)
#define ERROR_NONODEFOUND	(1<<5)
#define ERROR_NO_DEFORM_IN_FILE	(1<<6)

//playback types
//POINTCACHE_MODIFIERVERSION == 0
enum {
	old_playback_range,
	old_playback_graph,
};
//POINTCACHE_MODIFIERVERSION == 1
enum {
	playback_original,
	playback_start,
	playback_range,
	playback_graph,
};

//POINTCACHE_MODIFIERVERSION == 3
enum {
	load_stream,
	load_persample,
	load_preload,
};

//out-of-range types
enum {
	ort_hold,
	ort_repeat,
	ort_pingpong,
};

//interpolation types
enum {
	interp_linear,
	interp_cubic,
};

//pblock stuff
#define PBLOCK_REF	0

enum {
	pointcache_params
};

enum {
	pc_cache_file,
	pc_record_start,
	pc_record_end,
	pc_sample_rate,
	pc_strength,
	pc_relative,
	pc_playback_type,
	pc_playback_start,
	pc_playback_end,
	pc_playback_before,
	pc_playback_after,
	pc_playback_frame,
	pc_interp_type,
	pc_apply_to_spline,
	pc_preload_cache,
	pc_clamp_graph,
	pc_force_unc_path,
	pc_load_type,
	pc_apply_to_whole_object,
	pc_load_type_slave,
};

class PointCache : public Modifier
{
friend class PointCacheManager;
friend class FixUpModifierVersionPLCB;
public:
	//enum for the file type that we have loaded.. currently supporting pts and pc2.
	enum FileType { ePC2=0, ePTS};
	PointCache();
	~PointCache();

	// Animatable /////////////////////////////////////////////////////////////////////////////
	void					GetClassName(TSTR& s);
	void					DeleteThis();

	int						NumSubs();
	Animatable*				SubAnim(int i);
	TSTR					SubAnimName(int i);

	void					EnumAuxFiles(NameEnumCallback& nameEnum, DWORD flags);

	int						NumParamBlocks();
	IParamBlock2*			GetParamBlock(int i);
	IParamBlock2*			GetParamBlockByID(BlockID id);


	// ReferenceMaker /////////////////////////////////////////////////////////////////////////
	int						NumRefs();
	RefTargetHandle			GetReference(int i);
	void					SetReference(int i, RefTargetHandle rtarg);

	RefResult				NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID,  RefMessage message);

	// BaseObject /////////////////////////////////////////////////////////////////////////////
	CreateMouseCallBack*	GetCreateMouseCallBack();

	BOOL					ChangeTopology();

	BOOL					HasUVW();
	void					SetGenUVW(BOOL sw);

	// Modifier ///////////////////////////////////////////////////////////////////////////////
	Interval				LocalValidity(TimeValue t);
	ChannelMask				ChannelsUsed();
	ChannelMask				ChannelsChanged();
	void					ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node);
	Class_ID				InputType();

	IOResult				Load(ILoad* iload);
	IOResult				Save(ISave* isave);

	// PointCache /////////////////////////////////////////////////////////////////////////////
	void					SetError(DWORD error);
	void					ClearError(DWORD error);
	bool					TestError(DWORD error);

	void					UpdateUI();

	bool					IsValidFName(TCHAR* fname);
	bool					ReadHeader();
	HANDLE					OpenCacheForRead(TCHAR* fname);
	HANDLE					OpenCacheForRecord(TCHAR* fname,HWND hwnds);
	bool					CloseCacheFile(bool notifyFileManager);

	bool					RecordToFile(HWND hWnd,bool &canceled); //if we canceled that canceled will be true
	bool					SaveFilePrompt(HWND hWnd); //returns false if we cancel
	void					OpenFilePrompt(HWND hWnd);

	void					Unload();
	void					Reload();

	bool					DeleteCacheFile(HWND hWnd);
	void					EnableModsBelow(BOOL enable);

	void					SetFileType(const TCHAR *filename);//set it after we set the cache file

	static void				NotifyDelete(void* param, NotifyInfo* arg);

	float					GetRealCacheSampleRate();
	//functions for setting up our values based upon the OLDPointCache object we are replacing

	void SetCacheFile(const TCHAR *file);
	void SetRecordStart(float f);
	void SetRecordEnd(float f);
	void SetPlaybackStart(float f);
	void SetPlaybackEnd(float f);
	void SetSampleRate(float f);
	void SetStrength(float f);
	void SetRelative(BOOL b);

	///////////////////////////////////////////////////////////////////////////////////////////
	// TODO: private, private, private
	IParamBlock2*			m_pblock;
	static IObjParam*		m_iCore;
	HWND					m_hWnd;

	HANDLE					m_hCacheFile;

	Point3*					m_firstSample;
	Point3*					m_sampleA;
	Point3*					m_sampleB;
	Point3*					m_samplea;
	Point3*					m_sampleb;

	float					m_cacheStartFrame;
	float					m_cacheSampleRate;
	float					m_realCacheSampleRate;

	DWORD					m_errorFlags;
	char					m_cacheSignature[12];
	char					m_signature[12];

	TimeValue				m_recordTime;
	int						m_fileVersion;
	int						m_numPoints;
	int						m_cacheVersion;
	int						m_cacheNumPoints;
	int						m_cacheNumSamples;

	bool					m_doLoad;

	int						m_modifierVersion;

    BOOL					m_pcacheEnabled;  // this is the state of the enable state of modifier at save time
	BOOL					m_pcacheEnabledInRender; // this is the state of the enablerender state of modifier at save time

	FileType				m_fileType; //the type of the file that's loaded

	static	TCHAR				m_InitialDir[MAX_PATH];

	
private:

	bool					CloseFile(HANDLE hFile);


	/// Get index of sample used at time t (index is a float because t can be between two samples)
	float					GetSampleIndex(TimeValue t);
	/// Get offset into cache file for indexed cache sample
	int						ComputeOffset(int index);
	/// Read a sample from the open cache into sampleBuffer (assumed to be big enough for m_cacheNumPoints Point3's)
	bool					ReadSample(int sampleIndex, Point3* sampleBuffer);

	void					FreeBuffers();

	Point3*					m_preloadBuffer;	// holds entire cache in memory, if pre-loading is enabled
	Point3*					m_recordBuffer;		// temporary holding spot while recording each sample

	bool					m_tempDisable;
	bool					m_recordMode;

	OLDPointCacheBase*		m_PTSPointCache; //if we have loaded a pts file, we need to use this to view it.


	virtual void SetFileName(const TCHAR *filename) = 0; //used to set the filename in the UI efficiently  implemented by the OSM and WSM subclasses.
};

//This class contains instances of all the point caches that have been constructed and not deleted.
//This is used so that we we properly close and then reopen cache files that have been re-recorded.

struct FileAndCaches
{
	TCHAR m_fileName[MAX_PATH];
	Tab<PointCache*> m_PCs;
	
};
struct PointCacheInstances
{
	Tab<FileAndCaches*> m_fileAndCaches;

	~PointCacheInstances();
	void Delete();

	void AddPointCache(PointCache *pC);
	void DeletePointCache(PointCache *pC);

	void OpenCacheForRead(PointCache *pc, const TCHAR *fileName);
	void OpenCacheForRecord(PointCache *pc, const TCHAR *fileName);
};


class OLDPointCacheBase : public Modifier {

	protected:
		// I have to store these off and save them since the modifier loads first then the enable state
	    BOOL pcacheEnabled;  // this is the state of the enable state of modifier at save time
		BOOL pcacheEnabledInRender; // this is the state of the enablerender state of modifier at save time

	public:

		BOOL isPCacheEnabled() { return pcacheEnabled;}
		BOOL isPCacheEnabledInRender() { return pcacheEnabledInRender;}


 		int errorMessage;

		BOOL tempDisable;
		BOOL beforeCache, afterCache;
//point cache version start at -100
//particle cache version start at -200
		TimeValue lastCount;
		int version;
		BOOL staticCount;
		int fileVersion;
		Tab<int> pointCountList;
		int maxCount;
		int ComputeStartOffset(int index);

		SimpleParticle *spobj;

		TSTR extension;
		TSTR extensionWildCard;
		TSTR loadName;
		// Parameter block
		IParamBlock2	*pblock;	//ref 0
		static IObjParam *ip;			//Access to the interface
		HWND hWnd;
		
		// From Animatable
		TCHAR *GetObjectName() { return GetString(IDS_CLASS_NAME_OLD); }

		//From Modifier
		//TODO: Add the channels that the modifier needs to perform its modification
		ChannelMask ChannelsUsed()  { return PART_GEOM|PART_TOPO; }
		//TODO: Add the channels that the modifier actually modifies
		ChannelMask ChannelsChanged() { return GEOM_CHANNEL; }
		void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);
		int PerformPTSMod(TCHAR *fname,float startTime,	BOOL rO,float s,  TimeValue t, int &numPoints,float &sampleRate, int &numSamples,ModContext &mc, ObjectState * os, INode *node);//without UI used by new pc2
		//TODO: Return the ClassID of the object that the modifier can modify
		Class_ID InputType() {return defObjectClassID;}
		Interval LocalValidity(TimeValue t);

		// From BaseObject
		//TODO: Return true if the modifier changes topology
		BOOL ChangeTopology() {return FALSE;}		

		
		CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;}
		void BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev);
		void EndEditParams(IObjParam *ip, ULONG flags,Animatable *next);

		Interval GetValidity(TimeValue t);

		// Automatic texture support
		BOOL HasUVW();
		void SetGenUVW(BOOL sw);
		
		// Loading/Saving
		IOResult Load(ILoad *iload);
		IOResult Save(ISave *isave);

		//From Animatable
		Class_ID ClassID() {return OLDPOINTCACHE_CLASS_ID;}		
		SClass_ID SuperClassID() { return OSM_CLASS_ID; }
		void GetClassName(TSTR& s) {s = GetString(IDS_CLASS_NAME_OLD);}
		
		RefTargetHandle Clone( RemapDir &remap );
		RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
			PartID& partID,  RefMessage message);

		int NumSubs() { return 1; }
		TSTR SubAnimName(int i) { return GetString(IDS_PARAMS); }				
		Animatable* SubAnim(int i) { return pblock; }
		int NumRefs() { return 1; }
		RefTargetHandle GetReference(int i) { return pblock; }
		void SetReference(int i, RefTargetHandle rtarg) { pblock=(IParamBlock2*)rtarg; }

		int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
		IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock
		void EnumAuxFiles(NameEnumCallback& nameEnum, DWORD flags);

		void DeleteThis() { delete this; }		
		//Constructor/Destructor
		OLDPointCacheBase();
		~OLDPointCacheBase();	
		
//point cache
		Point3 *cFrame;
		Point3 *nFrame;
		Point3 *firstFrame;
		int cTime,nTime;
		FILE *outFile;
		void RecordToFile(HWND hWnd);
		void SaveFile(HWND hWnd);
		void OpenFile(HWND hWnd);
		void SetupButtons(HWND hWnd);

		BOOL recordMode;
		BOOL relativeOffset;
		float strength;

		int numberPoints;

		int cacheSamples, cacheNumberPoints;
		int cacheNumberFrames;
		float per;
		TimeValue recordTime;
		void DisableModsBelow(BOOL enable);
		FILE* OpenFileForRead( TCHAR *filename);
		FILE* OpenFileForWrite( TCHAR *filename);

		TimeValue ComputeIndex(TimeValue t,float startTime);

		void InvalidateUI();

		//used to transfer oldpointcache to new
		float GetStartTime();
		float GetEndTime();
		int GetSamples();
		TCHAR * GetCacheFile();
		BOOL GetRelative();
		float GetStrength();


};


#endif // __POINTCACHE_H__
