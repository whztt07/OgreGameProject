//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
/*===========================================================================*\
	FILE: sunlight.cpp

	DESCRIPTION: Sunlight system plugin.

	HISTORY: Created Oct.15 by John Hutchinson
			Derived from the ringarray

	Copyright (c) 1996, All Rights Reserved.
 \*==========================================================================*/
/*===========================================================================*\
 | Include Files:
\*===========================================================================*/
#pragma unmanaged
#include "NativeInclude.h"
HINSTANCE hInstance = nullptr;

#pragma managed
#include "compass.h"
#include "sunlightOverride.h"
#include "sunlight.h"
#include "natLight.h"
#include "suntypes.h"
#include "autovis.h"
#include "sunclass.h"
#include "citylist.h"
#include "DaylightSystemFactory.h"
#include "DaylightSystemFactory2.h"
#include "weatherdata.h"
#include "NatLight.h"


// russom - August 16, 2006 - 775527
#define SUBANIM_PBLOCK		0
#define SUBANIM_LOOKAT		1
#define SUBANIM_MULTIPLIER	2

// Parameter block indices
//#define PB_RAD	0
//#define PB_LAT	1
//#define PB_LONG	2
//#define PB_DATE	4
//#define PB_TIME 3
//#define PB_ZONE 5
//#define PB_DST 6

#define PB_RAD	4
#define PB_LAT	2
#define PB_LONG	3
#define PB_DATE	1
#define PB_TIME 0
#define PB_ZONE 5
#define PB_DST	6
#define PB_MANUAL 7
#define PB_SKY	8

//default data
//The latitudes and longitudes for these three cities below were obtained from the
//file "sitename.txt"
#if defined(LOCALIZED_JPN)	// Japanese
	#define SF_LAT 35.685f
	#define SF_LONG 139.751f
	static char *defcityname ="Tokyo Japan";
	#define SF_ZONE 9
#elif defined(LOCALIZED_CHS)	// Chinese
	#define SF_LAT 39.9167f
	#define SF_LONG 116.4333f
	static char *defcityname ="Beijing China";
	#define SF_ZONE 8
#elif defined(LOCALIZED_KOR)	// Seoul Korea
	#define SF_LAT 36.5333f
	#define SF_LONG 126.9583f
	static char *defcityname ="Seoul Korea";
	#define SF_ZONE 9
#else
	#define SF_LAT 37.795f
	#define SF_LONG -122.394f
	static char *defcityname ="San Francisco, CA";
	#define SF_ZONE -8
#endif // LOCALIZED_XXX

#define MINYEAR 1583
#define MAXYEAR 3000
#define MINRADIUS 0.0f
#define MAXRADIUS 100000000.0f
#define SUN_RGB Point3(0.88235294f, 0.88235294f, 0.88235294f)  // 225

// functions from GEOLOC.CPP
extern BOOL doLocationDialog(HWND hParent, IObjParam* ip, float* latitude,
							 float* longitude, char* cityName);


namespace 
{
// holdLastName's role (and holdLastCity's as well) is to keep the City picking 
// dialog's default selection up to date with the current selection.  
// Unfortunately, it doesn't work all that well - it didn't even in Vesper -
// but I'm reluctant to completely remove it in case it is necessary for 
// some reason that I didn't see.
// Note that CityRestore does not update these values. It used to, but that
// did more harm than good, since the SunMaster affected may not be active
// in the UI.  The results were that, to the user, the dialog appeared to 
// be randomly initialized.
int holdLastCity;
const int HOLD_LAST_NAME_LENGTH = 64;
char holdLastName[HOLD_LAST_NAME_LENGTH] = {0};
}

// externals from GEOLOC.CPP
extern int lastCity;
extern char* lastCityName;

static float stdang_to_compass(float stdang);
float compass_to_stdang(float compass);
static float getZRot(TimeValue t, INode* node);

static bool inCreate = false;

class CityRestore : public RestoreObj, MaxSDK::SingleWeakRefMaker {
public:
	CityRestore(SunMaster* master, const TCHAR* name);

	virtual void Restore(int isUndo);
	virtual void Redo();

	virtual int Size() { return sizeof(*this); }
	virtual TSTR Description() { return "Undo Sunlight City Name"; }

	virtual void DeleteThis();

private:
	std::basic_string<char> _undoCity;
	std::basic_string<char> _redoCity;
	};

CityRestore::CityRestore(SunMaster* master, const TCHAR* name)
{
	SetRef(master);
	if(NULL != master && NULL != master->GetCity())
	{
		_redoCity = master->GetCity();
	}

	if(NULL != name)
	{
		_undoCity = name;
	}
	}


void CityRestore::Restore(int isUndo)
{
	SunMaster *master = static_cast<SunMaster*>(GetRef());
	if(NULL != master)
	{
		master->SetCity(_undoCity.c_str());
		master->UpdateUI(GetCOREInterface()->GetTime());
	}
	}

void CityRestore::Redo() 
{
	SunMaster *master = static_cast<SunMaster*>(GetRef());
	if(NULL != master)
	{
		master->SetCity(_redoCity.c_str());
		master->UpdateUI(GetCOREInterface()->GetTime());
	}
}

void CityRestore::DeleteThis()
{
	delete this;
}




class ManualRestore : public RestoreObj, MaxSDK::SingleWeakRefMaker {
public:
	ManualRestore(SunMaster* master, int oldManual);

	virtual void Restore(int isUndo);
	virtual void Redo();

	virtual int Size() { return sizeof(*this); }
	virtual TSTR Description() { return "Undo Set Daylight Control Type"; }

	virtual void DeleteThis(){delete this;}

private:
	int mUndoManual;
	int mRedoManual;
};

ManualRestore::ManualRestore(SunMaster* master, int oldManual)
{
	SetRef(master);
	if(master!=NULL)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		mRedoManual = master->GetManual(t);
	}
	mUndoManual = oldManual;

}


void ManualRestore::Restore(int isUndo)
{
	SunMaster *master = static_cast<SunMaster*>(GetRef());
	if(master !=NULL)
	{
		master->NotifyNatLightManualChanged((IDaylightSystem2::DaylightControlType)(mUndoManual));
	}
}

void ManualRestore::Redo()
{
	SunMaster *master = static_cast<SunMaster*>(GetRef());
	if(NULL != master)
	{
		master->NotifyNatLightManualChanged((IDaylightSystem2::DaylightControlType)(mRedoManual));
	}
}



/*===========================================================================*\
 | Sun Master Methods:
\*===========================================================================*/

// This method returns a new instance of the slave controller.
Control* GetNewSlaveControl(SunMaster *master, int i);

// Initialize the class variables...
//HWND SunMaster::hMasterParams = NULL;
IObjParam *SunMaster::iObjParams;


ISpinnerControl *SunMaster::radSpin;
ISpinnerControl *SunMaster::latSpin;
ISpinnerControl *SunMaster::longSpin;
ISpinnerControl *SunMaster::hourSpin;
ISpinnerControl *SunMaster::minSpin;
ISpinnerControl *SunMaster::secSpin;
ISpinnerControl *SunMaster::yearSpin;
ISpinnerControl *SunMaster::monthSpin;
ISpinnerControl *SunMaster::daySpin;
ISpinnerControl *SunMaster::northSpin;
ISpinnerControl *SunMaster::zoneSpin;
#ifndef NO_DAYLIGHT_SKY_COVERAGE_SLIDER
ISliderControl  *SunMaster::skySlider;
#endif // NO_DAYLIGHT_SKY_COVERAGE_SLIDER
ICustStatus	*SunMaster::altEdit;
ICustStatus	*SunMaster::azEdit;
ICustStatus	*SunMaster::cityDisplay;
// no longer static 
// SYSTEMTIME SunMaster::theTime;
HIMAGELIST SunMaster::hIconImages = NULL;



float stdang_to_compass(float stdang){
	float rel =  - stdang;
	if (rel >=0.0f) 
		return rel;
	else 
		return FTWO_PI - (float)fabs(rel);
}

float compass_to_stdang(float compass){
		return  - compass;
}

// Returns the local Zrotation of the node less any winding number
// measured relative to the local z axis 
// positive values are clockwise

float getZRot(TimeValue t,INode *node){

	AffineParts lparts;
	float ang[3];
	INode *parent;
	Matrix3 nodeTM, parentTM, localTM, newTM;
	if (node ){
		nodeTM = node->GetNodeTM(t);
		parent = node->GetParentNode();
		if(parent){
			parentTM = parent->GetNodeTM(t);
			localTM = nodeTM * Inverse(parentTM);
			decomp_affine(localTM, &lparts);
			QuatToEuler(lparts.q, ang);
			float test1 = (ang[2]/FTWO_PI);
			int test2 = (int) test1;
			float temp = (float) test1-test2;
			return frtd(stdang_to_compass(temp*FTWO_PI));
		} else
			return 0.0f;
	}  else
		return 0.0f;
}


// Get the APPROXIMATE time zone from the longitude.
int getTimeZone(float longi)
{
	// Transform the supplied longitude to [-180, 180]
	float t_lon = longi;
	if (longi > 180.0 || longi < -180.0)
		 t_lon = longi - (LONG(longi)/180)*360.0;

    int tz;
	//switch t_lon to keep negative logic here.
	t_lon *= -1.0f;
	if (t_lon >= 0)
		tz = -(int)((t_lon + 7.5) / 15);
    else
		tz = (int)((-t_lon + 7.5) / 15);

	DbgAssert(tz >= -12 && tz <= 12);
    return tz;
}

void SunMaster::SetPosition(const Point3& position)
{
	Interface* ip = GetCOREInterface();
	
	//Set the position of the compass - which sets the global position of the system
	if(thePoint)
	{
		Matrix3 mat(TRUE);
		mat.SetTranslate(position);
		thePoint->SetNodeTM(ip->GetTime(), mat);
	}
}

Point3 SunMaster::GetPosition()
{
	Interface* ip = GetCOREInterface();
	
	Point3 pos(0.0f,0.0f,0.0f);
	//Get the position of the compass - which Gets the global position of the system
	if(thePoint)
	{
		Matrix3 mat(TRUE);
		mat = thePoint->GetNodeTM(ip->GetTime());
		mat.PointTransform(pos);
	}
	return pos;
}

void SunMaster::SetCompassDiameter(float compassDiameter)
{
	//Find the compass object
	CompassRoseObject* pCompass = NULL;
	Object* pObjref = thePoint->GetObjectRef();
	if (pObjref)
	{
		Object* base = pObjref->FindBaseObject();
		assert(base);
		if (base && base->ClassID() == COMPASS_CLASS_ID && base->SuperClassID() == HELPER_CLASS_ID)
			pCompass = static_cast<CompassRoseObject*>(base);
	}

	if(pCompass)
	{
		pCompass->axisLength = compassDiameter;
	}
}

float SunMaster::GetCompassDiameter()
{
	float compassDiameter = 0.0f;

	//Find the compass object
	CompassRoseObject* pCompass = NULL;
	Object* pObjref = thePoint->GetObjectRef();
	if (pObjref)
	{
		Object* base = pObjref->FindBaseObject();
		assert(base);
		if (base && base->ClassID() == COMPASS_CLASS_ID && base->SuperClassID() == HELPER_CLASS_ID)
			pCompass = static_cast<CompassRoseObject*>(base);
	}

	if(pCompass)
	{
		compassDiameter = pCompass->axisLength;
	}
	return compassDiameter;
}


void SunMaster::GetClassName(TSTR& s) {
	s = GetString(daylightSystem ? IDS_DAY_CLASS : IDS_SUN_CLASS);
}
Animatable* SunMaster::SubAnim(int i)
{
	switch (i) {
	case SUBANIM_PBLOCK:
		return pblock;
	case SUBANIM_LOOKAT:
		return theLookat;
	case SUBANIM_MULTIPLIER:
		return theMultiplier;
	}
	return NULL;
}

TSTR SunMaster::SubAnimName(int i)
{
	switch (i) {
	case SUBANIM_PBLOCK:
		return GetString(IDS_DB_PARAMETERS);
	case SUBANIM_LOOKAT:
		return GetString(IDS_LOOKAT);
	case SUBANIM_MULTIPLIER:
		return GetString(IDS_MULTIPLIER);
	}
	return TSTR();
}

// russom - August 16, 2006 - 775527
BOOL SunMaster::CanAssignController(int subAnim)
{
	return( daylightSystem && ((subAnim == SUBANIM_LOOKAT) || (subAnim == SUBANIM_MULTIPLIER)) );
}

// russom - August 16, 2006 - 775527
BOOL SunMaster::AssignController(Animatable *control,int subAnim)
{
	if( !daylightSystem )
		return FALSE;

	if( subAnim == SUBANIM_LOOKAT )
		ReplaceReference(REF_LOOKAT,(RefTargetHandle)control);
	else if( subAnim == SUBANIM_MULTIPLIER )
		ReplaceReference(REF_MULTIPLIER,(RefTargetHandle)control);
	else
		return FALSE;

	NotifyDependents(FOREVER,0,REFMSG_CONTROLREF_CHANGE);  //fix for 894778 MZ
	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);  

	return TRUE;
}

#define SUNMASTER_VERSION_SUNLIGHT 2 //2 is same as 1 except we revert the long
#define SUNMASTER_VERSION_DAYLIGHT 5 //4 is same as 5 except we revert the long

/* here's what it looked like in VIZ
	ParamBlockDesc desc[] = {
		{ TYPE_FLOAT, NULL, FALSE },//radius
		{ TYPE_FLOAT, NULL, FALSE },//lat
		{ TYPE_FLOAT, NULL, FALSE },//long
		{ TYPE_FLOAT, NULL, TRUE }, //date
		{ TYPE_INT, NULL, TRUE },//time
		{ TYPE_INT, NULL, FALSE },//zone
		{ TYPE_BOOL, NULL, FALSE },//dst
		};

*/

static ParamBlockDescID desc[] = {
		{ TYPE_FLOAT, NULL, TRUE, PB_RAD },//radius
		{ TYPE_FLOAT, NULL, TRUE, PB_LAT },//lat
		{ TYPE_FLOAT, NULL, TRUE, PB_LONG },//long
		{ TYPE_FLOAT, NULL, TRUE, PB_DATE}, //date
		{ TYPE_INT, NULL, TRUE , PB_TIME},//time
		{ TYPE_INT, NULL, FALSE, PB_ZONE },//zone
		{ TYPE_BOOL, NULL, FALSE, PB_DST},//dst
		};

	//Paramneters for MAX 2.0 version
/*
static ParamBlockDescID desc1[] = {
		{ TYPE_FLOAT, NULL, TRUE, PB_RAD },//radius
		{ TYPE_FLOAT, NULL, TRUE, PB_LAT },//lat
		{ TYPE_FLOAT, NULL, TRUE, PB_LONG },//long
		{ TYPE_INT, NULL, TRUE, PB_TIME },//time
		{ TYPE_FLOAT, NULL, TRUE , PB_DATE}, //date
		{ TYPE_INT, NULL, FALSE, PB_ZONE },//zone
		{ TYPE_BOOL, NULL, FALSE, PB_DST},//dst
		};*/
static ParamBlockDescID desc1[] = {
		{ TYPE_INT, NULL, TRUE, PB_TIME },//time
		{ TYPE_FLOAT, NULL, TRUE , PB_DATE}, //date
		{ TYPE_FLOAT, NULL, TRUE, PB_LAT },//lat
		{ TYPE_FLOAT, NULL, TRUE, PB_LONG },//long
		{ TYPE_FLOAT, NULL, TRUE, PB_RAD },//radius
		{ TYPE_INT, NULL, FALSE, PB_ZONE },//zone
		{ TYPE_BOOL, NULL, FALSE, PB_DST},//dst
		};

static ParamBlockDescID desc2Daylight[] = {
		{ TYPE_INT, NULL, TRUE, PB_TIME },//time
		{ TYPE_FLOAT, NULL, TRUE , PB_DATE}, //date
		{ TYPE_FLOAT, NULL, TRUE, PB_LAT },//lat
		{ TYPE_FLOAT, NULL, TRUE, PB_LONG },//long
		{ TYPE_FLOAT, NULL, TRUE, PB_RAD },//radius
		{ TYPE_INT, NULL, FALSE, PB_ZONE },//zone
		{ TYPE_BOOL, NULL, FALSE, PB_DST},//dst
		{ TYPE_BOOL, NULL, FALSE, PB_MANUAL},//manual override
		};

static ParamBlockDescID desc3Daylight[] = {
		{ TYPE_INT, NULL, TRUE, PB_TIME },//time
		{ TYPE_FLOAT, NULL, TRUE , PB_DATE}, //date
		{ TYPE_FLOAT, NULL, TRUE, PB_LAT },//lat
		{ TYPE_FLOAT, NULL, TRUE, PB_LONG },//long
		{ TYPE_FLOAT, NULL, TRUE, PB_RAD },//radius
		{ TYPE_INT, NULL, FALSE, PB_ZONE },//zone
		{ TYPE_BOOL, NULL, FALSE, PB_DST},//dst
		{ TYPE_BOOL, NULL, FALSE, PB_MANUAL},//manual override  //
		{ TYPE_FLOAT, NULL, TRUE, PB_SKY },//skyCondition
		};
static ParamBlockDescID desc4Daylight[] = {
		{ TYPE_INT, NULL, TRUE, PB_TIME },//time
		{ TYPE_FLOAT, NULL, TRUE , PB_DATE}, //date
		{ TYPE_FLOAT, NULL, TRUE, PB_LAT },//lat
		{ TYPE_FLOAT, NULL, TRUE, PB_LONG },//long
		{ TYPE_FLOAT, NULL, TRUE, PB_RAD },//radius
		{ TYPE_INT, NULL, FALSE, PB_ZONE },//zone
		{ TYPE_BOOL, NULL, FALSE, PB_DST},//dst
		{ TYPE_INT, NULL, FALSE, PB_MANUAL},//manual override  //
		{ TYPE_FLOAT, NULL, TRUE, PB_SKY },//skyCondition
		};

//static funcs for notifications
void NotifyMaxReset(void *ptr, NotifyInfo *info)
{
	//make sure it's a max file getting opened if we are doing an open
	DbgAssert(info != NULL);
	if (info->intcode == NOTIFY_FILE_PRE_OPEN)
		if (NULL != info->callParam && IOTYPE_MAX != *(static_cast<FileIOType*>(info->callParam)))
			return;

	//set the natLight to NULL!
	SunMaster *sl = (SunMaster*)ptr;
	sl->natLight = NULL;

}

// Constructor.
SunMaster::SunMaster(bool daylight) {

	pblock = NULL;
	thePoint = NULL;
	theLegacyObject = NULL;
	theLookat = NULL;
	theMultiplier = NULL;
	 theObjectMon = NULL;
	ignore_point_msgs = FALSE;
	hMasterParams = NULL;
	daylightSystem = daylight;
	controlledByWeatherFile  = false;
	natLight=NULL;

	// Create a parameter block and make a reference to it.
	if (daylightSystem)
		ReplaceReference( REF_PARAM, CreateParameterBlock( desc4Daylight, 9, SUNMASTER_VERSION_DAYLIGHT ) );
	else
		ReplaceReference( REF_PARAM, CreateParameterBlock( desc1, 7, SUNMASTER_VERSION_SUNLIGHT ) );

	 ReplaceReference(REF_OBJECT_MONITOR, (ReferenceTarget*)CreateInstance(REF_TARGET_CLASS_ID, NODEMONITOR_CLASS_ID));

	//Make the controllers linear
	Control *c = (Control *) CreateInstance(CTRL_FLOAT_CLASS_ID,Class_ID(LININTERP_FLOAT_CLASS_ID,0)); 
	pblock->SetController(PB_DATE, c, TRUE);
	c = (Control *) CreateInstance(CTRL_FLOAT_CLASS_ID,Class_ID(LININTERP_FLOAT_CLASS_ID,0)); 
	pblock->SetController(PB_TIME, c, TRUE);
	
	// Set the initial values at time 0.
	GetLocalTime(&theTime);
	tvalid = NEVER;

	//TIME_ZONE_INFORMATION tzi;
	//DWORD result = GetTimeZoneInformation(&tzi);

    CityList::GetInstance().init();

    CityList::Entry * pEntry = CityList::GetInstance().GetDefaultCity();

    char *  name    = defcityname;
    int     zone    = SF_ZONE;
    float   lati    = SF_LAT;
    float   longi   = SF_LONG;
    if (pEntry)
    {
        name = pEntry->name;
        if ((*name == '+') || (*name == '*'))
            ++name;
        zone  = getTimeZone(pEntry->longitude);
        lati  = pEntry->latitude;
        longi = pEntry->longitude;
    }

	SetLat ( TimeValue(0), lati );
	SetLong( TimeValue(0), longi);

	SetCity(name);
	SetNorth(TimeValue(0), 0.0f);
	SetZone(TimeValue(0), zone);

	// Since we don't use the time zone of the computer anymore then default the daylight saving time to false.
	//SetDst(TimeValue(0), result == TIME_ZONE_ID_DAYLIGHT ? TRUE : FALSE);
	SetDst(TimeValue(0), FALSE);

	SetManual(0, false);
	refset=FALSE;
    mbOldAnimation = FALSE;
	timeref=0.0f;
	dateref=0;

	// Get time notification so we can update the UI
	GetCOREInterface()->RegisterTimeChangeCallback(this);


	//we need to make sure the natLight ptr is NULL before we reset. 1020318
	RegisterNotification(&NotifyMaxReset,				(void *) this, NOTIFY_FILE_PRE_OPEN);
	RegisterNotification(&NotifyMaxReset,				(void *) this, NOTIFY_SYSTEM_PRE_RESET);
	RegisterNotification(&NotifyMaxReset,				(void *) this, NOTIFY_SYSTEM_PRE_NEW);


// Since the users typically render an image per solstice/equinox, create
// the daylight system to a time that approximatly represents the summer solstice
// of the current year (i.e. June 12th of the current year, 12:00).
#ifdef DAYLIGHT_DEFAULT_TIME_SUMMER_SOLSTICE
	theTime.wMonth = 6;
	theTime.wDay = 21;

	theTime.wHour = 12;
	theTime.wMinute = 0;
	theTime.wSecond = 0;
#endif // DAYLIGHT_DEFAULT_TIME_SUMMER_SOLSTICE

	// since we know that theTime is valid we can do this
	interpJulianStruct jd = fusetime(theTime);
	SetDate( TimeValue(0),  jd );
	SetTime( TimeValue(0),  jd );
	// now that the parmeter block is up to date we can calculate
	// the dependent vars
	calculate(TimeValue(0),FOREVER);

	}

static void NodesCloned(void* param, NotifyInfo*)
{
	static_cast<SunMaster*>(param)->RefThePointClone();
	UnRegisterNotification(NodesCloned, param, NOTIFY_POST_NODES_CLONED);
}

SunMaster::~SunMaster()
{
	// Just in case this gets deleted after Clone is called
	// but before the NOTIFY_POST_NODES_CLONED notification is sent.
	UnRegisterNotification(NodesCloned, this, NOTIFY_POST_NODES_CLONED);
	// Stop time notifications
	GetCOREInterface()->UnRegisterTimeChangeCallback(this);

	//notifications need to get unregistered.
	UnRegisterNotification(&NotifyMaxReset,				(void *) this, NOTIFY_FILE_PRE_OPEN);
	UnRegisterNotification(&NotifyMaxReset,				(void *) this, NOTIFY_SYSTEM_PRE_RESET);
	UnRegisterNotification(&NotifyMaxReset,				(void *) this, NOTIFY_SYSTEM_PRE_NEW);

}

void SunMaster::RefThePointClone()
{
	if (thePoint != NULL) {
		INode* p = thePoint;
		thePoint = NULL;
		ReplaceReference(REF_POINT, p);
	}
}

// This method is called to return a copy of the ring master.
RefTargetHandle SunMaster::Clone(RemapDir& remap) {
    SunMaster* newm = new SunMaster(daylightSystem);	
	newm->ReplaceReference(REF_PARAM,remap.CloneRef(this->pblock));
	newm->ReplaceReference(REF_POINT,NULL);
	newm->ReplaceReference(REF_LOOKAT,remap.CloneRef(this->theLookat));
	newm->ReplaceReference(REF_MULTIPLIER,remap.CloneRef(this->theMultiplier));
	 newm->ReplaceReference(REF_OBJECT_MONITOR, remap.CloneRef(this->theObjectMon));
	newm->thePoint = NULL;
	newm->theLegacyObject = NULL;
	newm->dateref = dateref;
	newm->SetCity(GetCity());
	remap.PatchPointer((RefTargetHandle*)&newm->thePoint,(RefTargetHandle)thePoint);
	if (newm->thePoint == NULL)
		RegisterNotification(NodesCloned, newm, NOTIFY_POST_NODES_CLONED);
	else
		newm->RefThePointClone();

	BaseClone(this, newm, remap);
	return(newm);
	}


// This method is called to update the UI parameters to reflect the
// correct values at the time passed.  Note that FALSE is passed as
// the notify parameter.  This ensure that notification message are
// not sent when the values are updated.
void SunMaster::UpdateUI(TimeValue t)
	{
	if ( hMasterParams ) {


		int manual = GetManual(t); //0 is location, 1 is manual and 2 is weather data file
		bool dateTime = (manual== NatLightAssembly::MANUAL_MODE||manual== NatLightAssembly::WEATHER_MODE) ? false: true;


		EnableWindow(GetDlgItem(hMasterParams,IDC_MANUAL_POSITION), !inCreate);
		EnableWindow(GetDlgItem(hMasterParams,IDC_CONTROLLER_POS), !inCreate);
		EnableWindow(GetDlgItem(hMasterParams,IDC_WEATHER_DATA_FILE), !inCreate);

		TimeValue t = GetCOREInterface()->GetTime();
		CheckDlgButton(hMasterParams, IDC_MANUAL_POSITION,
			manual== NatLightAssembly::MANUAL_MODE);
		CheckDlgButton(hMasterParams, IDC_CONTROLLER_POS,
			manual == NatLightAssembly::DATETIME_MODE);
		CheckDlgButton(hMasterParams, IDC_WEATHER_DATA_FILE,
			manual == NatLightAssembly::WEATHER_MODE);

		if(manual==NatLightAssembly::DATETIME_MODE||manual==NatLightAssembly::WEATHER_MODE) //orb scale for date time and weather file
		{
			radSpin->Enable(true);
			northSpin->Enable(true);
		}
		else
		{
			radSpin->Enable(false);
			northSpin->Enable(false);

		}
		latSpin->Enable(dateTime);
		longSpin->Enable(dateTime);
		hourSpin->Enable(dateTime);
		minSpin->Enable(dateTime);
		secSpin->Enable(dateTime);
		monthSpin->Enable(dateTime);
		daySpin->Enable(dateTime);
		yearSpin->Enable(dateTime);
		zoneSpin->Enable(dateTime);


		if(manual==NatLightAssembly::WEATHER_MODE) //only enable setup with weather file.
			EnableWindow(GetDlgItem(hMasterParams,IDC_SETUP),TRUE);
		else
			EnableWindow(GetDlgItem(hMasterParams,IDC_SETUP),FALSE);

		if (daylightSystem)
		{
#ifndef NO_DAYLIGHT_SKY_COVERAGE_SLIDER
			skySlider->Enable(dateTime);
#endif // NO_DAYLIGHT_SKY_COVERAGE_SLIDER
		}

		EnableWindow(GetDlgItem(hMasterParams,IDC_DST), dateTime);
		EnableWindow(GetDlgItem(hMasterParams,IDC_GETLOC), dateTime);

		radSpin->SetValue( GetRad(t), FALSE );
		latSpin->SetValue( GetLat(t), FALSE );
		longSpin->SetValue( GetLong(t), FALSE );
		if (daylightSystem)
		{
#ifndef NO_DAYLIGHT_SKY_COVERAGE_SLIDER
			skySlider->SetValue( GetSkyCondition(t), FALSE);
#endif // NO_DAYLIGHT_SKY_COVERAGE_SLIDER
		}

		radSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_RAD,t));
		latSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_LAT,t));
		longSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_LONG,t));
		if (daylightSystem)
		{
#ifndef NO_DAYLIGHT_SKY_COVERAGE_SLIDER
			skySlider->SetKeyBrackets(pblock->KeyFrameAtTime(PB_SKY,t));
#endif // NO_DAYLIGHT_SKY_COVERAGE_SLIDER
		}

		BOOL timekey = pblock->KeyFrameAtTime(PB_TIME,t);
		BOOL datekey = pblock->KeyFrameAtTime(PB_DATE,t);

		hourSpin->SetValue( GetHour(t), FALSE );
		minSpin->SetValue( GetMin(t), FALSE );
		secSpin->SetValue( GetSec(t),FALSE);
		monthSpin->SetValue( GetMon(t), FALSE );
		daySpin->SetValue( GetDay(t), FALSE );
		yearSpin->SetValue( GetYr(t), FALSE );

		hourSpin->SetKeyBrackets(timekey);
		minSpin->SetKeyBrackets(timekey);
		secSpin->SetKeyBrackets(timekey);
		monthSpin->SetKeyBrackets(datekey);
		daySpin->SetKeyBrackets(datekey);
		yearSpin->SetKeyBrackets(datekey);

		northSpin->SetValue( GetNorth(t), FALSE);
		zoneSpin->SetValue( GetZone(t), FALSE);
		CheckDlgButton(hMasterParams,IDC_DST, GetDst(t));
		TCHAR buf[10];
	  _itot_s(static_cast<int>(rtd(az)), buf, (sizeof(buf) / sizeof(buf[0])), 10);
		if(azEdit) azEdit->SetText(buf);
	  _itot_s(static_cast<int>(rtd(alt)), buf, (sizeof(buf) / sizeof(buf[0])), 10);
		if(altEdit) altEdit->SetText(buf);
		if(cityDisplay) cityDisplay->SetText(city);
		}
	}


// The master controller of a system plug-in should implement this 
// method to give MAX a list of nodes that are part of the system.   
// The master controller should fill in the given table with the 
// INode pointers of the nodes that are part of the system. This 
// will ensure that operations like cloning and deleting affect 
// the whole system.  MAX will use GetInterface() in the 
// tmController of each selected node to retrieve the master 
// controller and then call GetSystemNodes() on the master 
// controller to get the list of nodes.
void SunMaster::GetSystemNodes(INodeTab &nodes, SysNodeContext c)
	{
	if (thePoint) {
		nodes.Append(1,&thePoint);
	}

	if (theObjectMon) 
	{
		INodeMonitor* nodeMon = static_cast<INodeMonitor*>(theObjectMon->GetInterface(IID_NODEMONITOR));
		DbgAssert(nodeMon != NULL);
		INode* n = nodeMon->GetNode();
		if (n != NULL) 
		{
			nodes.Append(1, &n);

			if (daylightSystem) {
				NatLightAssembly::AppendAssemblyNodes(n, nodes, c);
			}
		}
	}
	}

int SunMaster::NumRefs()
{
	return NUM_REF;
};

// This methods returns the ith reference - there are two: the 
// parameter block and the light (helper, actually).
RefTargetHandle SunMaster::GetReference(int i)  
{ 
	switch (i) 
	{
	case REF_PARAM:
		return pblock;
	case REF_POINT:
		return thePoint;
	case REF_LOOKAT:
		return theLookat;
	case REF_MULTIPLIER:
		return theMultiplier;
	 case REF_OBJECT_MONITOR:
		 return theObjectMon;
		}
	return NULL;
	}

// This methods sets the ith reference - there are two.
void SunMaster::SetReference(int i, RefTargetHandle rtarg) 
{
	switch (i) 
	{
	case REF_PARAM:
		pblock = static_cast<IParamBlock*>(rtarg);
		break;
	case REF_POINT:
		thePoint = static_cast<INode*>(rtarg);
		break;
	case REF_LOOKAT:
		theLookat = static_cast<Control*>(rtarg);
		break;
	case REF_MULTIPLIER:
		theMultiplier = static_cast<Control*>(rtarg);
		break;
		case REF_OBJECT_MONITOR:
			theObjectMon = rtarg;
			break;
		}
	}		

BOOL SunMaster::GetNextKeyTime(TimeValue t,DWORD flags,TimeValue &nt)
	{
	TimeValue at,tnear = 0;
	BOOL tnearInit = FALSE;
	Control *ct, *cd;
	ct = cd = NULL;
	ct = pblock->GetController(PB_TIME);
	cd = pblock->GetController(PB_DATE);

	
	if (cd && cd->GetNextKeyTime(t,flags,at)) {
		if (!tnearInit) {
			tnear = at;
			tnearInit = TRUE;
		} else 
		if (ABS(at-t) < ABS(tnear-t)) tnear = at;
		}

	if (ct && ct->GetNextKeyTime(t,flags,at)) {
		if (!tnearInit) {
			tnear = at;
			tnearInit = TRUE;
		} else 
		if (ABS(at-t) < ABS(tnear-t)) tnear = at;
		}

	
	if (tnearInit) {
		nt = tnear;
		return TRUE;
	} else {
		return FALSE;
		}
	}

// This function converts a position on the unit sphere,
// given in azimuth altitude, to xyz coordinates
Point3 az_to_xyz(float az, float alt){
	double x,y,z;
	x = cos(alt)*sin(az);
	y = cos(alt)*cos(az);
	z =	sin(alt);
	Point3 xyzp(x,y,z);
	return Normalize(xyzp);
}


// ======= This method is the crux of the system plug-in ==========
// This method gets called by each slave controller and based on the slaves
// ID, it is free to do whatever it wants. In the current system there is only 
// the slave controller of the light.
void SunMaster::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method,
 int id) {
	Matrix3 tmat;
	Matrix3 *mat;
	tmat.IdentityMatrix();
	Point3 unitdir;
	float* mult;

	switch(id){
		case LIGHT_TM:

			if (theLookat != NULL && GetManual(t)==NatLightAssembly::MANUAL_MODE) {
				theLookat->GetValue(t, val, valid, method);
			} else {
				float radius = GetRad(t,valid);

				if(controlledByWeatherFile&&natLight!=NULL)
				{
					static int notGetting = true; //just to make sure we hit no infinite loops
												  //we shouldn't
					if(notGetting)
					{
						notGetting = false;
						natLight->SetSunLocationBasedOnWeatherFile(this,t,valid);
						notGetting = true;
					};
				}
				// calculate the controllers dependent variables: the azimuth and altitude
				calculate(t,valid);

				// Calculate the translation and rotation of the node 
				//CAUTION slop data types
				mat = (Matrix3*)val;
				tmat.RotateX(PI/2.0f - float(alt));
				tmat.RotateZ(PI - float(az));
				unitdir = az_to_xyz(float(az), float(alt));
				tmat.SetTrans(radius*unitdir);
				if (theLookat != NULL) {
					SuspendAnimate();
					AnimateOff();
					SetXFormPacket xform(tmat);
					theLookat->SetValue(t, &xform, 0);
					ResumeAnimate();
				}

				Matrix3 tm = *mat;
				(*mat) = (method==CTRL_RELATIVE) ? tmat*(*mat) : tmat;

				if (theLookat != NULL) {
					// Lookat controllers setup some of their state in
					// the GetValue method, so it needs to be called.
					Interval v;
					theLookat->GetValue(t, &tm, v, method);
				}

				// Make sure spinners track when animating and in Motion Panel
				// the limits on the day spinner may be wrong
				if ( hMasterParams ) {
					int year, month, day ,leap, modays;
					month = GetMon(t);
					year = GetYr(t);
					day = GetDay(t);
					leap = isleap(year);
					if (month == 12) modays = 31;
					else modays = mdays[leap][month]-mdays[leap][month-1];
					daySpin->SetLimits(1,modays,FALSE);
				}

//				UpdateUI(t);
			}
			break;

		case LIGHT_INTENSE:
		case LIGHT_MULT:
			mult=(float*)val;
			if (theMultiplier != NULL && GetManual(t)==NatLightAssembly::MANUAL_MODE) {
				theMultiplier->GetValue(t, val, valid, method);
			} else {
				// calculate the controllers dependent variables: the azimuth and altitude
				calculate(t,valid);
				*mult = calcIntensity(t, valid);
				if (theMultiplier != NULL) {
					SuspendAnimate();
					AnimateOff();
					theMultiplier->SetValue(t, mult, 0);
					ResumeAnimate();
				}
			}
			if (id == LIGHT_MULT) {
				float factor = 1500.0f;
				ToneOperatorInterface* pI = static_cast<ToneOperatorInterface*>(
					GetCOREInterface(TONE_OPERATOR_INTERFACE));
				if (pI != NULL) {
					ToneOperator* p = pI->GetToneOperator();
					if (p != NULL) {
						factor = p->GetPhysicalUnit(t, valid);
					}
				}
				*mult /= factor;
			}
			break;

		case LIGHT_SKY:
			mult=(float*)val;
			*mult = GetSkyCondition(t, valid);
			break;
	}
}

static inline int getJulian(int mon, int day)
{
	static int days[] = {
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};

	return days[mon - 1] + day;
}

float SunMaster::calcIntensity(TimeValue t, Interval& valid)
{
	const float Esc = 127500;	// Solar Illumination Constant in lx
	const float kPi = 3.14159265358979323846f;
	const float k2Pi365 = (2.0f * kPi) / 365.0f;
	float cloudy;

	cloudy = GetSkyCondition(t, valid);

	if (cloudy > 0.75f || alt <= 0.0f) {
		return 0.0f;
	}

	int julian = getJulian(GetMon(t, valid), GetDay(t, valid));

	float Ext = Esc * (1.0f + .034f * cosf(k2Pi365 * float(julian - 2)));
	float opticalAirMass = 1.0f / sin(alt);
	if (cloudy < 0.35f)
		opticalAirMass *= -0.21f;
	else
		opticalAirMass *= -0.80f;

	return Ext * exp(opticalAirMass);
}

void SunMaster::SetValue(TimeValue t, void *val, int commit, GetSetMethod method, int id)
{
	switch (id) {
	case LIGHT_TM:
		// Always set the lookat controller. To keep
		// the lookat and time somewhat in sync.
		if (GetManual(t)==NatLightAssembly::MANUAL_MODE && theLookat != NULL)
			theLookat->SetValue(t, val, commit, method);
		break;

	case LIGHT_INTENSE:
		// Always set the lookat controller. To keep
		// the lookat and time somewhat in sync.
		if (GetManual(t)==NatLightAssembly::MANUAL_MODE && theMultiplier != NULL)
			theMultiplier->SetValue(t, val, commit, method);
		break;

	case LIGHT_MULT:
		// Always set the lookat controller. To keep
		// the lookat and time somewhat in sync.
		if (GetManual(t)==NatLightAssembly::MANUAL_MODE && theMultiplier != NULL) {
			float factor = 1500.0f;
			ToneOperatorInterface* pI = static_cast<ToneOperatorInterface*>(
				GetCOREInterface(TONE_OPERATOR_INTERFACE));
			if (pI != NULL) {
				ToneOperator* p = pI->GetToneOperator();
				if (p != NULL) {
					factor = p->GetPhysicalUnit(t);
				}
			}
			factor *= *static_cast<float*>(val);
			theMultiplier->SetValue(t, &factor, commit, method);
		}
		break;

	case LIGHT_SKY:
		SetSkyCondition(t, *static_cast<float*>(val));
		break;
	}
}


class TimeDimension : public ParamDimension {
	public:
		DimType DimensionType() {return DIM_CUSTOM;}
		// Enforce range limits. Out-of-range values are reset to valid limits.
		float Convert(float value)
		{
			// Convert seconds to hours.
			if (value < 0.0f)
				return 0.0f;
			else if (value >= SECS_PER_DAY)
				value = SECS_PER_DAY - 1;
			return value/3600.0f;
		}
		float UnConvert(float value)
		{
			// Convert hours to seconds.
			if (value < 0.0f)
				return 0.0f;
			else if (value >= 24.0f)
				return SECS_PER_DAY - 1;
			return value*3600.0f;
		}
	};
static TimeDimension theTimeDim;


// This is the method that recieves change notification messages
RefResult SunMaster::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
     PartID& partID, RefMessage message ) 
    {
	switch (message) {
		case REFMSG_GET_PARAM_DIM: { 
			// The ParamBlock needs info to display in the track view.
			GetParamDim *gpd = (GetParamDim*)partID;
			switch (gpd->index) {
				case PB_RAD: gpd->dim = stdWorldDim;break;
				case PB_TIME: gpd->dim = &theTimeDim;break;
				}
			return REF_HALT; 
			}

		case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			switch (gpn->index) {
				case PB_DATE: gpn->name = GetString(IDS_SOL_DATE);break;
				case PB_TIME: gpn->name = GetString(IDS_SOL_TIME);break;
				case PB_RAD: gpn->name = GetString(IDS_RAD);break;
				case PB_LAT: gpn->name = GetString(IDS_LAT);break;
				case PB_LONG: gpn->name = GetString(IDS_LONG);break;
				case PB_SKY: gpn->name = GetString(IDS_SKY_COVER);break;
//				case PB_ZONE: gpn->name = GetString(IDS_ZONE);break;
				}
			return REF_HALT; 
			}

		case REFMSG_TARGET_DELETED:
			if (hTarget==thePoint) {
				thePoint = NULL;
				break;
				}
			return REF_STOP;

		case REFMSG_CHANGE:
			if ( hTarget==thePoint && !ignore_point_msgs ) {
				if (hMasterParams) 
					UpdateUI(iObjParams->GetTime());
				break;
				}
			else if (hTarget == pblock){
				if (hMasterParams) 
					UpdateUI(iObjParams->GetTime());
				break;
				}
			else if (hTarget == theLookat && GetManual(GetCOREInterface()->GetTime())==NatLightAssembly::MANUAL_MODE)
				break;
			else if (hTarget == theMultiplier && GetManual(GetCOREInterface()->GetTime())==NatLightAssembly::MANUAL_MODE)
				break;
			return REF_STOP;
		}
	return(REF_SUCCEED);
	}

// Sets the city name.
void SunMaster::SetCity(const char* name)
{
	const char* nm = name;
	if (nm != NULL)
	{
		if ((*nm == '+') || ((*nm == '*')))
			++nm;
		strcpy(city, nm);
	}
	else
	{
		*city = '\0';
		lastCity = -1;
	}
}

void SunMaster::calculateCity(TimeValue t)
{
	if(controlledByWeatherFile==false)
	{
		//Search through the city list to find a match if we can.
		CityList& citylist = CityList::GetInstance();
		citylist.init();
		char* cityName = NULL;
		//There may be minor differences in the latitude and longitude.
		const float eps = 0.01f;
		for (int i = 0; i < citylist.Count(); ++i)
		{
			if (fabs(GetLat(t) - citylist[i].latitude) < eps && 
				fabs(GetLong(t) - citylist[i].longitude) < eps)
			{
				cityName = citylist[i].name;
				if (cityName[0] == '+' || cityName[0] == '*')
				{
					cityName = &cityName[1];
				}
				break;
			}
		}
		SetCity(cityName);
	}
}

// The following methods set and get the radius value in the parameter block.
void SunMaster::SetRad(TimeValue t, float r) { 
	pblock->SetValue( PB_RAD, t, r );
	}

float SunMaster::GetRad(TimeValue t, Interval& valid ) { 	
	float f;
	pblock->GetValue( PB_RAD, t, f, valid );
	return f;
	}

// The following methods set and get the lat/long in the parameter block.
void SunMaster::SetLat(TimeValue t, float r) { 
	pblock->SetValue( PB_LAT, t, r );
	}

float SunMaster::GetLat(TimeValue t, Interval& valid ) { 	
	float f;
	pblock->GetValue( PB_LAT, t, f, valid );
	return f;
	}

void SunMaster::SetLong(TimeValue t, float r) { 
	pblock->SetValue( PB_LONG, t, r );
	}

float SunMaster::GetLong(TimeValue t, Interval& valid ) { 	
	float f;
	pblock->GetValue( PB_LONG, t, f, valid );
	return f;
	}

// Other pblock access methods

void SunMaster::SetZone(TimeValue t, int h) { 
	pblock->SetValue( PB_ZONE, t, h );
	}

int SunMaster::GetZone(TimeValue t, Interval& valid ) { 	
	int i;
	pblock->GetValue( PB_ZONE, t, i, valid );
	return i;
	}

void SunMaster::SetDst(TimeValue t, BOOL h) { 
	pblock->SetValue( PB_DST, t, h );
	}

BOOL  SunMaster::GetDst(TimeValue t, Interval& valid ) { 	
	BOOL b;
	pblock->GetValue( PB_DST, t, b, valid );
	return b;
	}

void SunMaster::SetManual(TimeValue t, int h) { 
	if (daylightSystem) {
		int oldManual; 
		Interval valid = FOREVER;
		pblock->GetValue( PB_MANUAL, t, oldManual, valid );
		if(oldManual!=h)
		{
			pblock->SetValue( PB_MANUAL, t, h ); //we could not call this
			if(theHold.Holding())
				theHold.Put(new ManualRestore(this, oldManual));	
			NotifyNatLightManualChanged((IDaylightSystem2::DaylightControlType)(h));
		}
	}
}

int  SunMaster::GetManual(TimeValue t, Interval& valid ) {
	if (daylightSystem && pblock != NULL) {
		int b;
		pblock->GetValue( PB_MANUAL, t, b, valid );
		return b;
		}
	return false;
	}

void SunMaster::SetSkyCondition(TimeValue t, float sky)
{
	if (daylightSystem)
		pblock->SetValue( PB_SKY, t, sky );
}

float SunMaster::GetSkyCondition(TimeValue t, Interval& valid)
{
	if (daylightSystem) {
		float f;
		pblock->GetValue( PB_SKY, t, f, valid );
		return f;
	}
	return 1.0f;
}

void SunMaster::SetNorth(TimeValue t, float r) { 
	if (thePoint) align_north(t,r);
	}

float SunMaster::GetNorth(TimeValue t, Interval& valid ) { 	
	return getZRot(t, thePoint);
	}


// This method actually places a representation of the current Julian date
// into the parameter block where it is interpolated. Because pblocks
// can only handle floats, two values are interpolated separately.
// The number of days goes into PB_DATE and is interpolated as a float.
//
// Furthermore because the number of JulianDays can be such a large number,
// prior to sticking it in the pblock we save a local epoch and measure date
// changes relative to this benchmark
//
void SunMaster::SetDate(TimeValue t, const interpJulianStruct& jd) { 
	double t1;

	// if were not animating we save a local epoch and measure relative to it
//	if(!Animating()){
	if(!refset){
		dateref=(long)jd.days;
		refset = TRUE;
		t1=0.0;
	}
	else {
		tvalid.SetEmpty();
		t1 = jd.days - dateref;
	}

	pblock->SetValue( PB_DATE, t, (float) t1 ); //caution possible data loss
	}

// This method puts the time of day into the pblock.
// The number of seconds in the current day goes into PB_TIME and
// is interpolated as an int.
void SunMaster::SetTime(TimeValue t, const interpJulianStruct& jd) { 
	pblock->SetValue( PB_TIME, t, jd.subday );
	}

// GetTime pulls the interpolated parameter block values out and passes them through
// fracturetime which adds the local epoch back in and explodes the value into a gregorian
// representation in the static var theTime.

// We implement a validity mechanism for the local time representation

void SunMaster::GetTime(TimeValue t, Interval& valid ) {
	if(0){
//	if(tvalid.InInterval(t)){
		valid &= tvalid;
		return;
	}
	else{
		tvalid = FOREVER;
		interpJulianStruct jd;
		float date;
		int time;
		pblock->GetValue( PB_DATE, t, date, tvalid );
		pblock->GetValue( PB_TIME, t, time, tvalid );
		valid &= tvalid;
		if (mbOldAnimation) {
			jd.days=date;
			jd.subday=time;
		}
		else {
			jd.days=floor(date + 0.5);
			jd.subday=time % (24 * 60 * 60);	// Truncate time to a day
			if (jd.subday < 0)					// No negative times.
				jd.subday += 24 * 60 * 60;
		}
		jd.epoch=dateref;
		fracturetime(jd,theTime);
	}
	}

//////////////////////////////////////////////////////////////
// The Pseudo pblock methods for getting time and setting time
//////////////////////////////////////////////////////////////

void SunMaster::SetHour(TimeValue t, int h) { 
	theTime.wHour = h;
	SetTime(t,fusetime(theTime));
	}

int SunMaster::GetHour(TimeValue t, Interval& valid ) { 	
	GetTime(t,valid);//throw away return, we want side-effect
	return theTime.wHour;
	}

void SunMaster::SetMin(TimeValue t, int h) { 
	theTime.wMinute = h;
	SetTime(t,fusetime(theTime));
	}

int SunMaster::GetMin(TimeValue t, Interval& valid ) { 	
	GetTime(t,valid);//throw away return, we want side-effect
	return theTime.wMinute;
	}

void SunMaster::SetSec(TimeValue t, int h) { 
	theTime.wSecond = h;
	SetTime(t,fusetime(theTime));
	}

int SunMaster::GetSec(TimeValue t, Interval& valid ) { 	
	GetTime(t,valid);//throw away return, we want side-effect
	return theTime.wSecond;
	}

void SunMaster::SetMon(TimeValue t, int h) { 
	theTime.wMonth = h;
	SetDate(t,fusetime(theTime));
	}

int SunMaster::GetMon(TimeValue t, Interval& valid ) { 	
	GetTime(t,valid);//throw away return, we want side-effect
	return theTime.wMonth;
	}

void SunMaster::SetDay(TimeValue t, int h) { 
	theTime.wDay = h;
	SetDate(t,fusetime(theTime));
	}

int SunMaster::GetDay(TimeValue t, Interval& valid ) { 	
	GetTime(t,valid);//throw away return, we want side-effect
	return theTime.wDay;
	}

void SunMaster::SetYr(TimeValue t, int h) { 
	theTime.wYear = h;
	SetDate(t,fusetime(theTime));
	}

int SunMaster::GetYr(TimeValue t, Interval& valid ) { 	
	GetTime(t,valid);//throw away return, we want side-effect
	return theTime.wYear;
	}

void SunMaster::SetWeatherMDYHMS(TimeValue t,int month,int day, int year, int hour, int minute, int second,BOOL dst)
{
	theTime.wMonth = month;
	theTime.wDay = day;
	theTime.wYear = year;
	theTime.wHour = hour;
	theTime.wMinute = minute;
	theTime.wSecond = second;
	interpJulianStruct jd = fusetime(theTime);
	SetDst(t, dst);
	SetDate(t,  jd );
	SetTime(t,  jd );
}



/////////////////////////////////////////////////////////////
//Methods for getting the private, dependent variables
///////////////////////////////////////////////////////////////

Point2 SunMaster::GetAzAlt(TimeValue t, Interval& valid ) {
	Point2 result;
	GetTime(t,valid);
	calculate(t,FOREVER);
	result.x = (float) az;
	result.y = (float) alt;
	return result;
}

// calculate the dependent variables at the given time
void SunMaster::calculate(TimeValue t, Interval& valid){
	double latitude,longitude;
	long hour,min,sec,month,day,year,zone;
	BOOL dst;

	// Retrieve the values of the UI parameters at the time passed in.
	zone = GetZone(t,valid);
	dst = GetDst(t,valid);
	latitude = GetLat(t,valid);
	longitude = GetLong(t,valid);
	hour = GetHour(t,valid);
	min = GetMin(t,valid);
	sec = GetSec(t,valid);
	month = GetMon(t,valid);
	day = GetDay(t,valid);
	year = GetYr(t,valid);


	double st;
	long zonedst = zone;
	if(dst) zonedst++;
	sunLocator(dtr(latitude), dtr(-longitude), month, day,  //mz new daylight fixes autovis functions expect longtitude to be wrong
		year, hour - zonedst, min,sec, 0,
		   &alt, &az, &st);

	

#ifdef _DEBUG
	FILE *stream;
	stream=fopen("round.log","a");
	float date;
	pblock->GetValue( PB_DATE, t, date, valid );
	float time;
	pblock->GetValue( PB_TIME, t, time, valid );
	fprintf(stream, "Yr: %d\tMon: %d\tDay: %d\tHr: %d\tMin: %d\tSec: %d\n", year, month, day, hour, min, sec );
/*	printf( "UYr: %d\tUMon: %d\tUDay: %d\tUHr: %d\tUMin: %d\tUSec: %d\n", hourSpin->GetIVal(),\
		monthSpin->GetIVal(), daySpin->GetIVal(), hourSpin->GetIVal(),\
		minSpin->GetIVal(), secSpin->GetIVal() );*/
	fprintf(stream, "Date: %f\tOffset: %d\tTime: %f\tOffset: %f\n", date, dateref, time, timeref );
  	fprintf(stream, "Lat: %f\tLong: %f\tZone: %ld\n\n", latitude, longitude, zone );
	fprintf(stream, "Az: %f\tAlt: %f\tSt: %f\n\n\n", az, alt, st );
	fclose(stream);
#endif

}


//method to rotate the helper object from the UI of the master controller
void SunMaster::align_north(TimeValue now, float north){
	assert(thePoint);

	// animate off and suspend msg processing
	ignore_point_msgs = TRUE;
	SuspendAnimate();
	AnimateOff();

	AffineParts lparts;
	float ang[3];
	INode *parent;
	Matrix3 nodeTM, parentTM, localTM, newTM;
  	parent = thePoint->GetParentNode();
	nodeTM = thePoint->GetNodeTM(now);
	parentTM = parent->GetNodeTM(now);
	localTM = nodeTM * Inverse(parentTM);
	decomp_affine(localTM, &lparts);
	QuatToEuler(lparts.q, ang);

	int turns = (int) (ang[2]/FTWO_PI);
	float subturn = ang[2] - turns;//we only affect the scrap
	ang[2]= turns + fdtr(compass_to_stdang(north));
	
	// build a new matrix
	EulerToQuat(ang, lparts.q);
	lparts.q.MakeMatrix(newTM);
	newTM.SetTrans(lparts.t);

	// animate back on and msg processing back to normal
	thePoint->SetNodeTM(now, newTM);
	ResumeAnimate();
	ignore_point_msgs = FALSE;
}

void SunMaster::NotifyNatLightManualChanged(IDaylightSystem2::DaylightControlType type)
{
	TimeValue t = GetCOREInterface()->GetTime();
	Interface* ip = GetCOREInterface();
	INodeTab nodes;
	GetSystemNodes(nodes,kSNCFileSave);
	for(int i=0;i<nodes.Count();++i)
	{
		if(nodes[i])
		{
			Object* daylightAssemblyObj = nodes[i]->GetObjectRef();
			BaseInterface* bi = daylightAssemblyObj->GetInterface(IID_DAYLIGHT_SYSTEM2);
			if(bi)
			{
				NatLightAssembly *nLA = dynamic_cast<NatLightAssembly*>(daylightAssemblyObj);
				nLA->SunMasterManualChanged(type);
			}
		}
	}
	
}
void SunMaster::ValuesUpdated()
{

	 INodeMonitor* nodeMon = NULL;
	 if (NULL != theObjectMon && 
			 NULL != (nodeMon = static_cast<INodeMonitor*>(theObjectMon->GetInterface(IID_NODEMONITOR))) &&
			 NULL != nodeMon->GetNode()) 
	 {
		 NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	 }
}

// The Dialog Proc

INT_PTR CALLBACK MasterParamDialogProc( HWND hDlg, UINT message, 
	WPARAM wParam, LPARAM lParam )
	{
	TCHAR buf[10];
	SunMaster *mc = DLGetWindowLongPtr<SunMaster *>( hDlg);
	if ( !mc && message != WM_INITDIALOG ) return FALSE;
	TimeValue now = mc->iObjParams->GetTime();

	assert(mc->iObjParams);
	switch ( message ) {
		int year, month,day,leap,modays;// locals for handling date changes
		case WM_INITDIALOG: {
			mc = (SunMaster *)lParam;
			DLSetWindowLongPtr( hDlg, mc);
			SetDlgFont( hDlg, mc->iObjParams->GetAppHFont() );

			// set up the date locals based on the current UI
			year = mc->GetYr(now);
			month = mc->GetMon(now);
			day = mc->GetDay(now);
			leap = isleap(year);
			if (month == 12) modays = 31;
			else modays = mdays[leap][month]-mdays[leap][month-1];

			// reset these copies
			holdLastCity = -1;
			strncpy(holdLastName, mc->GetCity(), sizeof(holdLastName)/ sizeof(holdLastName[0]));
			
			mc->radSpin  = GetISpinner(GetDlgItem(hDlg,IDC_RADSPINNER));
			mc->latSpin  = GetISpinner(GetDlgItem(hDlg,IDC_LATSPINNER));
			mc->longSpin  = GetISpinner(GetDlgItem(hDlg,IDC_LONGSPINNER));
			mc->yearSpin  = GetISpinner(GetDlgItem(hDlg,IDC_YEARSPINNER));
			mc->monthSpin  = GetISpinner(GetDlgItem(hDlg,IDC_MONTHSPINNER));
			mc->daySpin  = GetISpinner(GetDlgItem(hDlg,IDC_DAYSPINNER));
			mc->secSpin  = GetISpinner(GetDlgItem(hDlg,IDC_SECSPINNER));
			mc->hourSpin  = GetISpinner(GetDlgItem(hDlg,IDC_HOURSPINNER));
			mc->minSpin  = GetISpinner(GetDlgItem(hDlg,IDC_MINSPINNER));
			mc->northSpin  = GetISpinner(GetDlgItem(hDlg,IDC_NORTHSPINNER));
			mc->zoneSpin  = GetISpinner(GetDlgItem(hDlg,IDC_ZONESPINNER));
#ifndef NO_DAYLIGHT_SKY_COVERAGE_SLIDER
			mc->skySlider = mc->daylightSystem ? GetISlider(GetDlgItem(hDlg,IDC_SKY_COVER_SLIDER)) : NULL;
#endif // NO_DAYLIGHT_SKY_COVERAGE_SLIDER
			mc->azEdit  = GetICustStatus(GetDlgItem(hDlg,IDC_AZ));
			mc->altEdit  = GetICustStatus(GetDlgItem(hDlg,IDC_ALT));
			mc->cityDisplay  = GetICustStatus(GetDlgItem(hDlg,IDC_CITY));

			mc->radSpin->SetLimits( MINRADIUS,MAXRADIUS, FALSE );
			mc->latSpin->SetLimits( -90.0f, 90.0f, FALSE );
			mc->longSpin->SetLimits( -180.0f, 180.0f, FALSE );
			mc->yearSpin->SetLimits(MINYEAR,MAXYEAR, FALSE );
			mc->zoneSpin->SetLimits(-12,12, FALSE );
			mc->monthSpin->SetLimits(1,12, FALSE );
			mc->daySpin->SetLimits( 1, modays, FALSE );
			mc->hourSpin->SetLimits(0,23, FALSE );
			mc->minSpin->SetLimits( 0, 59, FALSE );
			mc->secSpin->SetLimits( 0, 59, FALSE );
			mc->northSpin->SetLimits(0.0f, 359.99f, FALSE );

			mc->radSpin->SetAutoScale( TRUE );
			mc->latSpin->SetScale(float(0.1) );
			mc->longSpin->SetScale(float(0.1) );
			mc->yearSpin->SetScale(float(1.0) );
			mc->monthSpin->SetScale(float(1.0) );
			mc->daySpin->SetScale(float(1.0) );
			mc->hourSpin->SetScale(float(1.0) );
			mc->minSpin->SetScale(float(1.0) );
			mc->secSpin->SetScale(float(1.0) );
			mc->northSpin->SetScale(float(1.0) );
			mc->zoneSpin->SetScale(float(1.0) );

			SetupFloatSlider(hDlg, IDC_SKY_COVER_SLIDER, 0, 0, 1,
				mc->GetSkyCondition(now), 2);

			mc->latSpin->SetValue( mc->GetLat(now), FALSE );
			mc->longSpin->SetValue( mc->GetLong(now), FALSE );
			mc->radSpin->SetValue( mc->GetRad(now), FALSE );
			mc->hourSpin->SetValue( mc->GetHour(now), FALSE );
			mc->minSpin->SetValue( mc->GetMin(now), FALSE );
			mc->secSpin->SetValue( mc->GetSec(now), FALSE );
			mc->monthSpin->SetValue( month, FALSE );
			mc->daySpin->SetValue( mc->GetDay(now), FALSE );
			mc->yearSpin->SetValue( year , FALSE );
			mc->northSpin->SetValue( mc->GetNorth(now), FALSE );
			mc->zoneSpin->SetValue( mc->GetZone(now), FALSE );
			CheckDlgButton(hDlg,IDC_DST, mc->GetDst(now));
		 _itot_s(static_cast<int>(rtd(mc->Getaz())), buf, (sizeof(buf) / sizeof(buf[0])), 10);
			mc->azEdit->SetText(buf);
		 _itot_s(static_cast<int>(rtd(mc->Getalt())), buf, (sizeof(buf) / sizeof(buf[0])), 10);
			mc->altEdit->SetText(buf);
			mc->cityDisplay->SetText(mc->GetCity());

			mc->radSpin->LinkToEdit( GetDlgItem(hDlg,IDC_RADIUS), EDITTYPE_POS_UNIVERSE );			
			mc->latSpin->LinkToEdit( GetDlgItem(hDlg,IDC_LAT), EDITTYPE_FLOAT );			
			mc->longSpin->LinkToEdit( GetDlgItem(hDlg,IDC_LONG), EDITTYPE_FLOAT );			
			mc->yearSpin->LinkToEdit( GetDlgItem(hDlg,IDC_YEAR), EDITTYPE_INT );			
			mc->monthSpin->LinkToEdit( GetDlgItem(hDlg,IDC_MONTH), EDITTYPE_INT );			
			mc->daySpin->LinkToEdit( GetDlgItem(hDlg,IDC_DAY), EDITTYPE_INT );			
			mc->hourSpin->LinkToEdit( GetDlgItem(hDlg,IDC_HOUR), EDITTYPE_INT );			
			mc->minSpin->LinkToEdit( GetDlgItem(hDlg,IDC_MIN), EDITTYPE_INT );			
			mc->secSpin->LinkToEdit( GetDlgItem(hDlg,IDC_SEC), EDITTYPE_INT );			
			mc->northSpin->LinkToEdit( GetDlgItem(hDlg,IDC_NORTH), EDITTYPE_FLOAT );			
 			mc->zoneSpin->LinkToEdit( GetDlgItem(hDlg,IDC_ZONE), EDITTYPE_INT );			

			// Set up icon	
			ICustButton *pBut = GetICustButton(GetDlgItem(hDlg,IDC_SETUP));
			if (pBut)
			{
				if (!mc->hIconImages)
				{
					mc->hIconImages  = ImageList_Create(16, 15, ILC_COLOR24 | ILC_MASK, 6, 0);
					LoadMAXFileIcon("daylight", mc->hIconImages, kBackground, FALSE);
				}	
				if(mc->hIconImages)
				{
					pBut->SetImage(mc->hIconImages, 4,4,5,5,12,11);
					ReleaseICustButton(pBut);
				}
				//pBut->SetTooltip(TRUE, GetString(IDS_SETUP));
			}

			mc->iObjParams->RedrawViews(now, REDRAW_INTERACTIVE, mc);
			}

			return FALSE;	// DB 2/27

		case WM_DESTROY:
			ReleaseISpinner( mc->radSpin );
			ReleaseISpinner( mc->latSpin );
			ReleaseISpinner( mc->longSpin );
			ReleaseISpinner( mc->daySpin );
			ReleaseISpinner( mc->monthSpin );
			ReleaseISpinner( mc->yearSpin );
			ReleaseISpinner( mc->hourSpin );
			ReleaseISpinner( mc->minSpin );
			ReleaseISpinner( mc->secSpin );
			ReleaseISpinner( mc->northSpin );
			ReleaseISpinner( mc->zoneSpin );
			ReleaseICustStatus( mc->azEdit);
			ReleaseICustStatus( mc->altEdit);
			ReleaseICustStatus( mc->cityDisplay);
			mc->radSpin  = NULL;
			mc->latSpin  = NULL;
			mc->longSpin  = NULL;
			mc->yearSpin  = NULL;
			mc->monthSpin  = NULL;
			mc->daySpin  = NULL;
			mc->hourSpin  = NULL;
			mc->minSpin  = NULL;
			mc->secSpin  = NULL;
			mc->azEdit  = NULL;
			mc->altEdit  =NULL;
			mc->cityDisplay  = NULL;
			mc->northSpin  =NULL;
			mc->zoneSpin  =NULL;


			return FALSE;

		case CC_SPINNER_BUTTONDOWN:
			theHold.Begin();
			return TRUE;


		case CC_SPINNER_CHANGE:
			if (!theHold.Holding())
				theHold.Begin();
			year = mc->GetYr(now);
			month = mc->GetMon(now);
			day = mc->GetDay(now);
			leap = isleap(year);
			if (month == 12) modays = 31;
			else modays = mdays[leap][month]-mdays[leap][month-1];


			switch ( LOWORD(wParam) ) {
				case IDC_RADSPINNER: mc->SetRad(now,  mc->radSpin->GetFVal() );  break;
				case IDC_LATSPINNER:
					mc->SetLat(now,  mc->latSpin->GetFVal() );
					mc->SetCity(NULL);
					break;
				case IDC_LONGSPINNER:
					mc->SetLong(now,  mc->longSpin->GetFVal() );
					mc->SetCity(NULL);
					break;
				case IDC_HOURSPINNER: mc->SetHour(now,  mc->hourSpin->GetIVal() );  break;
				case IDC_MINSPINNER: mc->SetMin(now,  mc->minSpin->GetIVal() );  break;
				case IDC_SECSPINNER: mc->SetSec(now,  mc->secSpin->GetIVal() );  break;
				case IDC_MONTHSPINNER:
					month =  mc->monthSpin->GetIVal();
					if (month == 12) modays = 31;
					else modays = mdays[leap][month]-mdays[leap][month-1];
					if (day > modays){
						day=modays;
						mc->SetDay(now,  day );}
					mc->daySpin->SetLimits(1,modays,FALSE);
					mc->SetMon(now,  month );
					break;
				case IDC_DAYSPINNER: mc->SetDay(now,  mc->daySpin->GetIVal() );  break;
				case IDC_YEARSPINNER: 
					year =  mc->yearSpin->GetIVal();
					leap = isleap(year);
					if (month == 12) modays = 31;
					else modays = mdays[leap][month]-mdays[leap][month-1];
					if (day > modays){
						day=modays;
						mc->SetDay(now,  day );}
					mc->daySpin->SetLimits(1,modays,FALSE);
					mc->SetYr(now,  year );
					break;
				case IDC_NORTHSPINNER:mc->SetNorth(now,  mc->northSpin->GetFVal() );  break;
				case IDC_ZONESPINNER: mc->SetZone(now,  mc->zoneSpin->GetIVal() );  break;
				}
			//Notify dependents: they will call getvalue which will update UI
			//else : calculate and Update UI directly
				 {
					 INodeMonitor* nodeMon = NULL;
					 if (NULL != mc->theObjectMon && 
							 NULL != (nodeMon = static_cast<INodeMonitor*>(mc->theObjectMon->GetInterface(IID_NODEMONITOR))) &&
							 NULL != nodeMon->GetNode()) 
					 {
						 mc->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
					 }
			else {
				mc->calculate(now,FOREVER);
				mc->UpdateUI(now);
			}
				 }
			assert(mc->iObjParams);
			mc->iObjParams->RedrawViews(now, REDRAW_INTERACTIVE, mc);
			return TRUE;

		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP:
			if (!HIWORD(wParam) && message != WM_CUSTEDIT_ENTER)
			{
				theHold.Cancel();
				switch (LOWORD(wParam))
				{
					case IDC_LATSPINNER:
					case IDC_LONGSPINNER:
						// reset if spinner operation is cancelled
						lastCity = holdLastCity;
						mc->SetCity(holdLastName);
						mc->calculate(now,FOREVER);
						mc->UpdateUI(now);
						break;
				}
			} else {
				if ((holdLastCity != -1 || *holdLastName != '\0') && theHold.Holding())
					theHold.Put(new CityRestore(mc, holdLastName));
				strncpy(holdLastName, mc->GetCity(), sizeof(holdLastName)/ sizeof(holdLastName[0]));
				holdLastCity = -1;
//				mc->iObjParams->RedrawViews(now, REDRAW_END, mc);
				theHold.Accept(GetString(IDS_UNDO_PARAM));
			}
			return TRUE;
			 break;

		case CC_SLIDER_CHANGE:
			{
			if (!theHold.Holding())
				theHold.Begin();
#ifndef NO_DAYLIGHT_SKY_COVERAGE_SLIDER
			switch ( LOWORD(wParam) ) {
				case IDC_SKY_COVER_SLIDER:
					if (mc->daylightSystem)
						mc->SetSkyCondition(now, mc->skySlider->GetFVal());
					break;
			}
#endif // NO_DAYLIGHT_SKY_COVERAGE_SLIDER
			//Notify dependents: they will call getvalue which will update UI
			//else : calculate and Update UI directly
				 INodeMonitor* nodeMon = NULL;
				 if (NULL != mc->theObjectMon && 
						 NULL != (nodeMon = static_cast<INodeMonitor*>(mc->theObjectMon->GetInterface(IID_NODEMONITOR))) &&
						 NULL != nodeMon->GetNode()) 
				 {
					mc->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
				 }
			else {
				mc->UpdateUI(now);
			}

			assert(mc->iObjParams);
			mc->iObjParams->RedrawViews(now, REDRAW_INTERACTIVE, mc);
			return TRUE;
			}
			break;
		case CC_SLIDER_BUTTONDOWN:
			theHold.Begin();
			return TRUE;

		case CC_SLIDER_BUTTONUP:
			if (! HIWORD(wParam)) {
				theHold.Cancel();
			} else {
				theHold.Accept(GetString(IDS_UNDO_PARAM));
			}
			return TRUE;

		case WM_MOUSEACTIVATE:
			mc->iObjParams->RealizeParamPanel();
			return FALSE;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			mc->iObjParams->RollupMouseMessage(hDlg,message,wParam,lParam);
			return FALSE;

		case WM_COMMAND:
			{
				switch(LOWORD(wParam)) { // Switch on ID
				default:
					return FALSE;
				case IDC_MANUAL_POSITION: {
					theHold.Begin();
					mc->SetManual(now,NatLightAssembly::MANUAL_MODE);
					theHold.Accept(GetString(IDS_UNDO_PARAM));
				} break;

				case IDC_CONTROLLER_POS: {
					theHold.Begin();
					mc->SetManual(now,NatLightAssembly::DATETIME_MODE);
					theHold.Accept(GetString(IDS_UNDO_PARAM));
				} break;

				case IDC_WEATHER_DATA_FILE:{
					theHold.Begin();
					mc->SetManual(now,NatLightAssembly::WEATHER_MODE);
					theHold.Accept(GetString(IDS_UNDO_PARAM));
				} break;
				case IDC_SETUP:
					{
						TimeValue t = GetCOREInterface()->GetTime();
						if (HIWORD(wParam) == BN_CLICKED&&mc->GetManual(t)==NatLightAssembly::WEATHER_MODE)
						{
							Interface* ip = GetCOREInterface();
							INodeTab nodes;
							mc->GetSystemNodes(nodes,kSNCFileSave);
							for(int i=0;i<nodes.Count();++i)
							{
								if(nodes[i])
								{
									Object* daylightAssemblyObj = nodes[i]->GetObjectRef();
									BaseInterface* bi = daylightAssemblyObj->GetInterface(IID_DAYLIGHT_SYSTEM2);
									if(bi)
									{
										IDaylightSystem2* ds = static_cast<IDaylightSystem2*>(bi);
										ds->OpenWeatherFileDlg();
										break;
									}
								}
							}
						}
					}
					break;
				// The user clicked the daylight savings checkbox.
				case IDC_DST:
					theHold.Begin();
					mc->SetDst(now,IsDlgButtonChecked(mc->hMasterParams,IDC_DST));
					theHold.Accept(GetString(IDS_UNDO_PARAM));
					break;
				case IDC_GETLOC: 
					float lat = mc->GetLat(now);
					float longi = mc->GetLong(now);
					char city[256] = "";
					if (doLocationDialog(hDlg, mc->iObjParams, &lat, &longi, city)) {
						// the "holdLastCity" string may be out of sync with the master
						// since it only shows the currently selected master regardless
						// of which one the undo action will modify
						std::basic_string<TCHAR> oldCity;
						if(NULL != mc->GetCity())
						{
							oldCity = mc->GetCity();
						}
						int tz = getTimeZone(longi);
						theHold.Begin();
						mc->SetCity(city);		// Set this first so updates triggered by
						mc->SetLat(now,lat);	// setting other values will update the UI.
						mc->SetLong(now,longi);
						mc->SetZone(now,tz);
						theHold.Put(new CityRestore(mc, oldCity.c_str()));
						theHold.Accept(GetString(IDS_UNDO_PARAM));
						// set these copies
						holdLastCity = lastCity;
						strcpy(holdLastName, city);
					}
					break;
				}
				 INodeMonitor* nodeMon = NULL;
				 if (NULL != mc->theObjectMon && 
						 NULL != (nodeMon = static_cast<INodeMonitor*>(mc->theObjectMon->GetInterface(IID_NODEMONITOR))) &&
						 NULL != nodeMon->GetNode()) 
				 {
					 mc->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
				 }
			else {
				mc->calculate(now,FOREVER);
				mc->UpdateUI(now);
			}
			mc->iObjParams->RedrawViews(now, REDRAW_INTERACTIVE, mc);
			return TRUE;
			}
			break;
		default:
			return FALSE;
		}
	}



// This method is called when the sun masters parameters may be edited
// in the motion branch of the command panel.  
void SunMaster::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
	{
	// Save the interface pointer passed in.  This pointer is only valid
	// between BeginEditParams() and EndEditParams().
	iObjParams = ip;
	inCreate = (flags & BEGIN_EDIT_CREATE) != 0;

	if ( !hMasterParams  ) {
		// Add the rollup page to the command panel. This method sets the
		// dialog proc used to manage the user interaction with the dialog
		// controls.


		hMasterParams = ip->AddRollupPage( 
				hInstance, 
				MAKEINTRESOURCE(daylightSystem ? IDD_DAYPARAM : IDD_SUNPARAM),
				MasterParamDialogProc,
				GetString(IDS_SUN_DLG_NAM), 
				(LPARAM)this
#if defined(NO_MOTION_PANEL) || defined(NO_DAYLIGHT_MOTION_PANEL)
				, 0,
				(ROLLUP_CAT_STANDARD + 2 * ROLLUP_CAT_CUSTATTRIB) / 3
#endif	// defined(NO_MOTION_PANEL) || defined(NO_DAYLIGHT_MOTION_PANEL)
				);
		
	} else {
		DLSetWindowLongPtr( hMasterParams, this);    
/*
		// Init the dialog to our values.
		radSpin->SetValue(GetRad(ip->GetTime()),FALSE);
		latSpin->SetValue(GetLat(ip->GetTime()),FALSE);
		longSpin->SetValue(GetLong(ip->GetTime()),FALSE);
		hourSpin->SetValue(GetHour(ip->GetTime()),FALSE);
		minSpin->SetValue(GetMin(ip->GetTime()),FALSE);
		secSpin->SetValue(GetSec(ip->GetTime()),FALSE);
		monthSpin->SetValue(GetMon(ip->GetTime()),FALSE);
		daySpin->SetValue(GetDay(ip->GetTime()),FALSE);
		yearSpin->SetValue(GetYr(ip->GetTime()),FALSE);
		BOOL temp = GetDst(ip->GetTime());
		CheckDlgButton(hMasterParams,IDC_DST,temp );
		*/
		}
	//JH 11/20/00
	//FIx for 136634 Commented out the manual updates to the UI and just call update Ui in both cases.
	UpdateUI(ip->GetTime());
	
	}
		
// This method is called when the user is finished editing the sun masters
// parameters in the command panel.		
void SunMaster::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{
	if (hMasterParams==NULL) {
		return;
		}

	if ( flags&END_EDIT_REMOVEUI ) 
	 {    
		ip->UnRegisterDlgWnd(hMasterParams);
		ip->DeleteRollupPage(hMasterParams);
		hMasterParams = NULL;
		}
	else 
	 {      
		DLSetWindowLongPtr( hMasterParams, 0);
		}


	hMasterParams = NULL;
	iObjParams = NULL;
}

#define VERSION_CHUNK 0x130
#define CITY_CHUNK		0x120
#define NODE_ID_CHUNK 0x110 // obsolete data
#define EPOCH_CHUNK 0x100
#define NEW_ANIM_CHUNK 0xf0

IOResult SunMaster::Save(ISave *isave) 
{
	ULONG nb;
	IOResult res;

	isave->BeginChunk(CITY_CHUNK);
		res = isave->WriteCString(city);
		assert(res == IO_OK);
	isave->EndChunk();

	isave->BeginChunk(EPOCH_CHUNK);
		isave->Write(&dateref,sizeof(ULONG), &nb);
	isave->EndChunk();

	if (!mbOldAnimation) 
	{
	    isave->BeginChunk(NEW_ANIM_CHUNK);
	    isave->EndChunk();
    }

	return IO_OK;
	}

class SunMasterPostLoad : public PostLoadCallback {
public:
    SunMaster *bo;
    SunMasterPostLoad(SunMaster *b) {bo=b;}
	void replaceSunlightWithDaylight();
    void proc(ILoad *iload) {
		int sunMasterVersion = bo->daylightSystem
			? SUNMASTER_VERSION_DAYLIGHT : SUNMASTER_VERSION_SUNLIGHT;
		int nParams = bo->daylightSystem ? 9 : 7;
		ParamBlockDescID* pDesc = bo->daylightSystem ? desc4Daylight : desc1;
        if (bo->pblock->GetVersion() != sunMasterVersion) {
            switch (bo->pblock->GetVersion()) {
            case 0:
                bo->ReplaceReference(REF_PARAM,
                                     UpdateParameterBlock(
                                         desc, 7, bo->pblock,
                                         pDesc, nParams, sunMasterVersion));
				bo->SetZone(TimeValue(0),-1 * bo->GetZone(TimeValue(0)));
				bo->SetLong(TimeValue(0),-1 * bo->GetLong(TimeValue(0)));
                iload->SetObsolete();
                break;

            case 1:
				bo->ReplaceReference(REF_PARAM,
										 UpdateParameterBlock(
											 desc1, 7, bo->pblock,
											 pDesc, nParams, sunMasterVersion));
				//if sun no change to any reference, just reset long still
				bo->SetLong(TimeValue(0),-1 * bo->GetLong(TimeValue(0)));
                iload->SetObsolete();
                break;

            case 2:
                bo->ReplaceReference(REF_PARAM,
                                     UpdateParameterBlock(
                                         desc2Daylight, 8, bo->pblock,
                                         desc3Daylight, 9, SUNMASTER_VERSION_DAYLIGHT));
				bo->daylightSystem = true;
				bo->SetLong(TimeValue(0),-1 * bo->GetLong(TimeValue(0)));
                iload->SetObsolete();
                break;
			   case 3:
                bo->ReplaceReference(REF_PARAM,
                                     UpdateParameterBlock(
                                         desc3Daylight, 9, bo->pblock,
                                         desc4Daylight, 9, SUNMASTER_VERSION_DAYLIGHT));
				bo->daylightSystem = true;
				bo->SetLong(TimeValue(0),-1 * bo->GetLong(TimeValue(0)));
                iload->SetObsolete();
                break;

            case 4:
				 bo->ReplaceReference(REF_PARAM,
                                     UpdateParameterBlock(
                                         desc3Daylight, 9, bo->pblock,
                                         desc4Daylight, 9, SUNMASTER_VERSION_DAYLIGHT));
				bo->daylightSystem = true;
				bo->SetLong(TimeValue(0),-1 * bo->GetLong(TimeValue(0)));
                iload->SetObsolete();
                break;
            default:
                assert(0);
                break;
            }
        }

				// Transform the persisted pointer to the Sun\Daylight node, into a node monitor
				if (bo->theLegacyObject != NULL) 
				{
					INodeMonitor* nodeMon = static_cast<INodeMonitor*>(bo->theObjectMon->GetInterface(IID_NODEMONITOR));
					DbgAssert(nodeMon != NULL);
					nodeMon->SetNode(bo->theLegacyObject);
					bo->theLegacyObject = NULL;
				}
        delete this;
    }
};

IOResult SunMaster::Load(ILoad *iload) 
{
	// Legacy max files do not contain a RingMaster version number
	IOResult res = IO_OK;
	USHORT next = iload->PeekNextChunkID();
	if (next != VERSION_CHUNK) {
		res = DoLoadLegacy(iload);
	}
	else {
		res = DoLoad(iload);
	}
	return res;
}


IOResult SunMaster::DoLoad(ILoad *iload)
{
	IOResult res;

	while (IO_OK == (res = iload->OpenChunk())) 
	{
		switch (iload->CurChunkID()) 
		{
			case VERSION_CHUNK:
			{
				ULONG 	nb;
				iload->Read(&mVerLoaded, sizeof(unsigned short), &nb);
				if (kVERSION_CURRENT > mVerLoaded) {
					iload->SetObsolete();
				}
			}
			break;
		}
		iload->CloseChunk();
	}
	return IO_OK;
}

IOResult SunMaster::DoLoadLegacy(ILoad *iload) 
{
	ULONG nb;
	TCHAR	*cp;
	IOResult res;

    mbOldAnimation = TRUE;
	iload->RegisterPostLoadCallback(new SunMasterPostLoad(this));
	while (IO_OK == (res = iload->OpenChunk())) 
	{
		switch (iload->CurChunkID())  
		{
			case CITY_CHUNK:
					res = iload->ReadCStringChunk(&cp);
				if (res == IO_OK) {
						SetCity(cp);
				}
				break;
			case NODE_ID_CHUNK:
					ULONG id;
					iload->Read(&id,sizeof(ULONG), &nb);
				if (id != 0xffffffff) {
					iload->RecordBackpatch(id, (void**)&theLegacyObject);
				}
				break;
			case EPOCH_CHUNK:
					iload->Read(&dateref,sizeof(LONG), &nb);
				if (dateref != 0) {
						refset = TRUE;
				}
				break;
			case NEW_ANIM_CHUNK:
					mbOldAnimation = FALSE;
				break;
			}

		iload->CloseChunk();
		if (res != IO_OK) {
			return res;
		}
	} // while

	tvalid = NEVER;

	return IO_OK;
	}

void SunMaster::TimeChanged(TimeValue t)
{
	if(controlledByWeatherFile&&natLight!=NULL)
	{
		Interval valid;
		natLight->SetEnergyValuesBasedOnWeatherFile(t,valid);
	}
	UpdateUI(t);
}

/*===========================================================================*\
 | Sun Slave Controller  Methods:
\*===========================================================================*/


// This method returns a new instance of the slave controller.
Control* GetNewSlaveControl(SunMaster *master, int i) {
	return new SlaveControl(master,i);
	}

// Constructor.  
SlaveControl::SlaveControl(const SlaveControl& ctrl) {
	daylightSystem = ctrl.daylightSystem;
	master = ctrl.master;
	id = ctrl.id;
	}

void SlaveControl::GetClassName(TSTR& s)
{
   s = GetString(id == LIGHT_TM ? IDS_SLAVE_POS_CLASS : IDS_SLAVE_FLOAT_CLASS);
}

// This constructor creates a reference from the slave controller to 
// the sun master object.
SlaveControl::SlaveControl(const SunMaster* m, int i) {
	daylightSystem = m->daylightSystem;
	id = i;
	master = NULL;
	ReplaceReference( 0, (ReferenceTarget *)m);
	}

SClass_ID SlaveControl::SuperClassID()
{
	switch (id) {
	case LIGHT_TM:
		return CTRL_POSITION_CLASS_ID;
	}
	return CTRL_FLOAT_CLASS_ID;
}

// This method is called to create a copy of the slave controller.
RefTargetHandle SlaveControl::Clone(RemapDir& remap) {
	SlaveControl *sl = new SlaveControl(daylightSystem);
	sl->id = id;
	sl->ReplaceReference(0, remap.CloneRef(master));
	BaseClone(this, sl, remap);
	return sl;
	}

SlaveControl& SlaveControl::operator=(const SlaveControl& ctrl) {
	daylightSystem = ctrl.daylightSystem;
	master = ctrl.master;
	id = ctrl.id;
	return (*this);
	}

// ========= This method is used to retrieve the value of the 
// controller at the specified time. =========

// This is a important aspect of the system plug-in - this method 
// calls the master object to get the value.

void SlaveControl::GetValue(TimeValue t, void *val, Interval &valid, 
	GetSetMethod method) {
	// Ensure the ring master exists.
	assert(master);
	master->GetValue(t,val,valid,method,id);	
	}

void SlaveControl::SetValue(TimeValue t, void *val, int commit, 
	GetSetMethod method)
{
	master->SetValue(t, val, commit, method, id);
	}

void* SlaveControl::GetInterface(ULONG id) {
	if (id==I_MASTER) 
		return (ReferenceTarget*)master;
	else 
		return Control::GetInterface(id);
	}

const int kSlaveIDChunk = 0x4444;
IOResult SlaveControl::Save(ISave *isave)
{
	isave->BeginChunk(kSlaveIDChunk);
	ULONG nb;
	IOResult res = isave->Write(&id, sizeof(id), &nb);
	if (res == IO_OK && nb != sizeof(id))
		res = IO_ERROR;
	isave->EndChunk();
	return res;
}

class SlaveControlPostLoad : public PostLoadCallback {
public:
	SlaveControlPostLoad(SlaveControl* slave) : mpSlave(slave) {}

	void proc(ILoad* iload) {
		if (mpSlave->master != NULL) {
			if (mpSlave->daylightSystem != mpSlave->master->daylightSystem) {
				mpSlave->daylightSystem = mpSlave->master->daylightSystem;
				iload->SetObsolete();
			}
		}
		delete this;
	}

private:
	SlaveControl*	mpSlave;
};

IOResult SlaveControl::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res;

	iload->RegisterPostLoadCallback(new SlaveControlPostLoad(this));

	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
		case kSlaveIDChunk:
			res = iload->Read(&id, sizeof(id), &nb);
			if (res != IO_OK && nb != sizeof(id))
				res = IO_ERROR;
			break;
		}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
	}

	return IO_OK;
}

// These are the API methods
void SlaveControl::GetSunTime( TimeValue t, SYSTEMTIME&	sunt ){
	master->GetTime(t,FOREVER);
	int hours = master->GetZone(t,FOREVER);
	if(master->GetDst(t,FOREVER)) hours--;

	
	sunt.wYear = master->theTime.wYear;
	sunt.wMonth = master->theTime.wMonth;
	sunt.wDay = master->theTime.wDay;

	sunt.wHour = (master->theTime.wHour + hours)%24;
	sunt.wMinute = master->theTime.wMinute;
	sunt.wSecond = master->theTime.wSecond;
}


 void SlaveControl::GetSunLoc(TimeValue t, Point2& origin, Point2& orient){

//	master.GetTime(t, FOREVER);
//	master.Calculate(t,FOREVER);
	
	origin.x = master->GetLat(t, FOREVER);
	origin.y = master->GetLong(t, FOREVER);
	Point2 temp =master->GetAzAlt(t,FOREVER); 
	orient.x = temp.x;
	orient.y = temp.y;
}


/*===========================================================================*\
 | SunMasterCreationManager Class:
\*===========================================================================*/
// This is the class used to manage the creation process of the ring array
// in the 3D viewports.
class SunMasterCreationManager : public MouseCallBack, ReferenceMaker {
	private:
		INode *node0;
		// This holds a pointer to the SunMaster object.  This is used
		// to call methods on the sun  master such as BeginEditParams() and
		// EndEditParams() which bracket the editing of the UI parameters.
		SunMaster *theMaster;
		// This holds the interface pointer for calling methods provided
		// by MAX.
		IObjCreate *createInterface;

		ClassDesc *cDesc;
		// This holds the nodes TM relative to the CP
		Matrix3 mat;
		// This holds the initial mouse point the user clicked when
		// creating the ring array.
		IPoint2 pt0;
		// This holds the center point of the ring array
		Point3 center;
		// This flag indicates the dummy nodes have been hooked up to
		// the master node and the entire system has been created.
		BOOL attachedToNode;
		bool daylightSystem;

		// This method is called to create a new SunMaster object (theMaster)
		// and begin the editing of the systems user interface parameters.
		void CreateNewMaster(HWND rollup);
			
		// This flag is used to catch the reference message that is sent
		// when the system plug-in itself selects a node in the scene.
		// When the user does this, the plug-in recieves a reference message
		// that it needs to respond to.
		int ignoreSelectionChange;

		// --- Inherited virtual methods of ReferenceMaker ---
		// use named constants to keep track of references
		static const int NODE_0_REFERENCE_INDEX = 0;
		static const int SUN_MASTER_REFERENCE_INDEX = 1;
		static const int REFERENCE_COUNT = 2;

		// This returns the number of references this item has.
		virtual int NumRefs();
		
		// This methods retrieves the ith reference.
		virtual RefTargetHandle GetReference(int i);
		
		// This methods stores the ith reference.
		virtual void SetReference(int i, RefTargetHandle rtarg);

		// This method recieves the change notification messages sent
		// when the the reference target changes.
	    RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
	    	PartID& partID,  RefMessage message);

	public:
		// This method is called just before the creation command mode is
		// pushed on the command stack.
		void Begin( IObjCreate *ioc, ClassDesc *desc, bool daylight );
		// This method is called after the creation command mode is finished 
		// and is about to be popped from the command stack.
		void End();
		
		// Constructor.
		SunMasterCreationManager()
			{
			ignoreSelectionChange = FALSE;
			}
		// --- Inherited virtual methods from MouseCallBack
		// This is the method that handles the user / mouse interaction
		// when the system plug-in is being created in the viewports.
		int proc( HWND hwnd, int msg, int point, int flag, IPoint2 m );

#if defined(NO_MOTION_PANEL) || defined(NO_DAYLIGHT_MOTION_PANEL)
		bool IsActive() const { return theMaster != NULL; }
#endif	// defined(NO_MOTION_PANEL) || defined(NO_DAYLIGHT_MOTION_PANEL)
	};

/*===========================================================================*\
 | SunMasterCreateMode Class:
\*===========================================================================*/
#define CID_RINGCREATE	CID_USER + 0x509C2DF4
// This is the command mode that manages the overall process when 
// the system is created.  
// See the Advanced Topics section on Command Modes and Mouse Procs for 
// more details on these methods.
class SunMasterCreateMode : public CommandMode {
	// This instance of SunMasterCreationMangaer handles the user/mouse
	// interaction as the sun array is created.
	IObjCreate *ip;
	SunMasterCreationManager proc;
	public:
		// These two methods just call the creation proc method of the same
		// name. 
		// This creates a new sun master object and starts the editing
		// of the objects parameters.  This is called just before the 
		// command mode is pushed on the stack to begin the creation
		// process.
		void Begin( IObjCreate *ioc, ClassDesc *desc, bool daylight ) 
			{ 
			ip=ioc;
			proc.Begin( ioc, desc, daylight ); }
		// This terminates the editing of the sun masters parameters.
		// This is called just before the command mode is removed from
		// the command stack.
		void End() { proc.End(); }
		// This returns the type of command mode this is.  See the online
		// help under this method for a list of the available choices.
		// In this case we are a creation command mode.
		int Class() { return CREATE_COMMAND; }
		// Returns the ID of the command mode. This value should be the 
		// constant CID_USER plus some random value chosen by the developer.
		int ID() { return CID_RINGCREATE; }
		// This method returns a pointer to the mouse proc that will
		// handle the user/mouse interaction.  It also establishes the number 
		// of points that may be accepted by the mouse proc.  In this case
		// we set the number of points to 100000.  The user process will 
		// terminate before this (!) after the mouse proc returns FALSE.
		// The mouse proc returned from this method is an instance of
		// SunMasterCreationManager.  Note that that class is derived
		// from MouseCallBack.
		MouseCallBack *MouseProc(int *numPoints) 
			{ *numPoints = 100000; return &proc; }
		// This method is called to flag nodes in the foreground plane.
		// We just return the standard CHANGE_FG_SELECTED value to indicate
		// that selected nodes will go into the foreground.  This allows
		// the system to speed up screen redraws.  See the Advanced Topics
		// section on Foreground / Background planes for more details.
		ChangeForegroundCallback *ChangeFGProc() 
			{ return CHANGE_FG_SELECTED; }
		// This method returns TRUE if the command mode needs to change the
		// foreground proc (using ChangeFGProc()) and FALSE if it does not. 
		BOOL ChangeFG( CommandMode *oldMode ) 
			{ return (oldMode->ChangeFGProc() != CHANGE_FG_SELECTED); }
		// This method is called when a command mode becomes active.  We
		// don't need to do anything at this time so our implementation is NULL
		void EnterMode() {
			SetCursor(LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CROSS_HAIR)));
			ip->PushPrompt( GetString(IDS_SUN_CREATE_PROMPT));
		}
		// This method is called when the command mode is replaced by 
		// another mode.  Again, we don't need to do anything.
		void ExitMode() {
			ip->PopPrompt();
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
		BOOL IsSticky() { return FALSE; }
		
#if defined(NO_MOTION_PANEL) || defined(NO_DAYLIGHT_MOTION_PANEL)
		bool IsActive() const { return proc.IsActive(); }
#endif	// defined(NO_MOTION_PANEL) || defined(NO_DAYLIGHT_MOTION_PANEL)
		};

// A static instance of the command mode.
static SunMasterCreateMode theSunMasterCreateMode;

#if defined(NO_MOTION_PANEL) || defined(NO_DAYLIGHT_MOTION_PANEL)
bool SunMaster::IsCreateModeActive()
{
	return theSunMasterCreateMode.IsActive();
}
#endif	// defined(NO_MOTION_PANEL) || defined(NO_DAYLIGHT_MOTION_PANEL)

// This initializes a few variables, creates a new sun master object and 
// starts the editing of the objects parameters.
void SunMasterCreationManager::Begin( IObjCreate *ioc, ClassDesc *desc, bool daylight )
	{
	// This just initializes the variables.
	createInterface = ioc;
	cDesc           = desc;	 //class descriptor of the master controller
	DeleteAllRefsFromMe();
	attachedToNode = FALSE;
	daylightSystem  = daylight;
	// This creates a new sun master object and starts the editing
	// of the objects parameters.
	CreateNewMaster(NULL);
	}

int SunMasterCreationManager::NumRefs()
{
	return REFERENCE_COUNT;
}

// This method sets the ith reference.  
void SunMasterCreationManager::SetReference(int i, RefTargetHandle rtarg) 
{ 
	switch(i) 
	{
		case NODE_0_REFERENCE_INDEX: 
			node0 = static_cast<INode *>(rtarg); 
			break;
		
		case SUN_MASTER_REFERENCE_INDEX:
			theMaster = static_cast<SunMaster *>(rtarg);
			break;

		default: 
			assert(0); 
		}
	}
// This method returns the ith node.  
RefTargetHandle SunMasterCreationManager::GetReference(int i) 
{ 
	switch(i) 
	{
		case NODE_0_REFERENCE_INDEX: 
			return node0;

		case SUN_MASTER_REFERENCE_INDEX:
			return theMaster;

		default: 
			return NULL;
		}
	}

//SunMasterCreationManager::~SunMasterCreationManager
void SunMasterCreationManager::End()
	{
	if (NULL != theMaster) {
#ifndef NO_CREATE_TASK
		theMaster->EndEditParams( (IObjParam*)createInterface, 
	                    	          TRUE/*destroy*/, NULL );
#endif
			DeleteAllRefsFromMe();
			}
		}

// This method is used to receive change notification messages from the 
// item that is referenced - the first created node in the system.
RefResult SunMasterCreationManager::NotifyRefChanged(
	Interval changeInt, 
	RefTargetHandle hTarget, 
	PartID& partID,  
	RefMessage message) 
	{
	bool targetIsNode0 = (hTarget == static_cast<RefTargetHandle>(node0));
	bool observedSelectionChange = 
		(REFMSG_TARGET_SELECTIONCHANGE == message && !ignoreSelectionChange);
	bool isDeleteMessage = (REFMSG_TARGET_DELETED == message);

	if( targetIsNode0 && (observedSelectionChange || isDeleteMessage) )
	{ 
			// This method is sent if the reference target (node0) is
			// selected or deselected.
			// This is used so that if the user deselects this slave node
			// the creation process will begin with a new item (and not 
		// continue to edit the existing item if the UI controls are 
			// adjusted)

		if(NULL != node0 && !node0->Selected())
		{
			// This will set node0 to NULL
			DeleteReference(NODE_0_REFERENCE_INDEX);
				}

		if( NULL != theMaster )
		{
			// refresh the panel
				HWND temp = theMaster->hMasterParams;
#ifndef NO_CREATE_TASK
				theMaster->EndEditParams( (IObjParam*)createInterface, FALSE/*destroy*/,NULL );
#endif
				// This creates a new SunMaster object and starts
				// the parameter editing process.
				CreateNewMaster(temp);	
				// Indicate that no nodes have been attached yet - 
				// no system created...
				attachedToNode = FALSE;
				}
		}
	return REF_SUCCEED;
	}

// This method is called to create a new sun master object and 
// begin the parameter editing process.
void SunMasterCreationManager::CreateNewMaster(HWND rollup)
	{
	theHold.Suspend();
	ReplaceReference(SUN_MASTER_REFERENCE_INDEX, 
		new SunMaster(daylightSystem), 
		true);
	theHold.Resume();
#ifndef NO_CREATE_TASK
	//Possibly point to an existing dialog rollup
	theMaster->hMasterParams = rollup;
#endif

#ifdef GLOBAL_COORD_SYS
	APPLICATION_ID appid = GetAppID();
	if (appid == kAPP_VIZ || appid == kAPP_VIZR)
	{
		IGcsSession * pSession = NULL;
		HRESULT hr = ::CoCreateInstance(CLSID_GcsSession,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IGcsSession,
			reinterpret_cast<void **>(&pSession));
		DbgAssert(SUCCEEDED(hr));
		if (SUCCEEDED(hr))
		{
			ICsCoordsysDef * pSys = NULL;
			pSession->GetGlobalCoordSystem(&pSys);
			if (pSys)
			{
#ifndef NO_CREATE_TASK
				theMaster->iObjParams = (IObjParam*)createInterface;
#endif	// NO_CREATE_TASK
				double lat, log;
				pSys->GetOriginLatitude(&lat);
				pSys->GetOriginLongitude(&log);

				//SS 2/12/2002: Sometimes we aren't getting a lat & long; see if we
				// get any other data out of the coordsys def.
				#ifdef _DEBUG
				BSTR loc, country, datum;
				pSys->GetLocation(&loc);
				pSys->GetCntrySt(&country);
				BOOL isgeo;
				pSys->IsGeodetic(&isgeo);
				pSys->GetDatumName(&datum);
				double xoff, yoff;
				pSys->GetOffsets(&xoff, &yoff);
				double lonmin, latmin, lonmax, latmax;
				pSys->GetLatLongBounds(&lonmin, &latmin, &lonmax, &latmax);
				short quad;
				pSys->GetQuadrant(&quad);
				#endif

				if (!(lat == 0.0 && log == 0.0))
				{
					theMaster->SetLat(TimeValue(0), lat);
					theMaster->SetLong(TimeValue(0), -log);
				}
				if (theMaster->GetLat(0) != SF_LAT || theMaster->GetLong(0) != SF_LONG)
				{
					theMaster->SetCity("");
					theMaster->SetZone(TimeValue(0), -(theMaster->GetLong(0) / 15));
				}
#ifndef NO_CREATE_TASK
				theMaster->iObjParams = NULL;
#endif	// NO_CREATE_TASK
				pSys->Release();
			}
			pSession->Release();
		}
	}
#endif
	
#ifndef NO_CREATE_TASK
	// Start the edit parameters process
	theMaster->BeginEditParams( (IObjParam*)createInterface, BEGIN_EDIT_CREATE,NULL );
#endif
	}


// This is the method of MouseCallBack that is used to handle the user/mouse
// interaction during the creation process of the system plug-in.
int SunMasterCreationManager::proc( 
				HWND hwnd,
				int msg,
				int point,
				int flag,
				IPoint2 m )
	{
	// This is set to TRUE if we should keep processing and FALSE
	// if the message processing should end.
	 int res = TRUE;
	// This is the helper object at the center of the system.
	static INode *dummyNode = NULL;
	// This is the radius of the sun system.
	float r;

	// The two objects we create here.
	//static HelperObject* compassObj = NULL;
	static CompassRoseObject* compassObj = NULL;
	static GenLight* lightObj = NULL;
	static NatLightAssembly* natLightObj = NULL;

	createInterface->GetMacroRecorder()->Disable();  // JBW, disable for now; systems not creatable in MAXScript

	// Attempt to get the viewport interface from the window
	// handle passed
	ViewExp *vpx = createInterface->GetViewport(hwnd); 
	assert( vpx );
	TimeValue t = createInterface->GetTime();
	// Set the cursor
	SetCursor(LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CROSS_HAIR)));


	// Process the mouse message sent...
	switch ( msg ) {
		case MOUSE_POINT:
		case MOUSE_MOVE: {
			switch (point) {
				case 0: {
					// Mouse Down: This point defines the center of the sun system.
					pt0 = m;
					// Make sure the master object exists
					assert(theMaster);

					// Reset our pointers.
					compassObj = NULL;
					lightObj = NULL;
					natLightObj = NULL;
					dummyNode = NULL;

					mat.IdentityMatrix();
					if ( createInterface->SetActiveViewport(hwnd) ) {
						createInterface->GetMacroRecorder()->Enable();  
						return FALSE;
						}

					// Make sure the view in the viewport is not parallel
					// to the creation plane - if it is the mouse points
					// clicked by the user cannot be converted to 3D points.
					if (createInterface->IsCPEdgeOnInView()) {
						createInterface->GetMacroRecorder()->Enable();  
						return FALSE;
						}

					if ( attachedToNode) {
						// As an experiment we won't allow any other to be created.
						//createInterface->PushPrompt( GetString(IDS_SUN_COMPLETE_PROMPT));
						//return FALSE;

						// A previous sun system exists - terminate the editing
						// of it's parameters and send it on it's way...
						// Hang on to the last one's handle to the rollup
						HWND temp = theMaster->hMasterParams;

#ifndef NO_CREATE_TASK
				   		theMaster->EndEditParams( (IObjParam*)createInterface,0,NULL );
#endif
						// Get rid of the references.  This sets node0 = NULL.
						DeleteAllRefsFromMe();

						// This creates a new SunMaster object (theMaster)
						// and starts the parameter editing process.
						CreateNewMaster(temp);
						}

					// Begin hold for undo
				   	theHold.Begin();

	 				createInterface->RedrawViews(t, REDRAW_BEGIN);

					// Snap the inital mouse point
					center = vpx->SnapPoint(m,m,NULL,SNAP_IN_PLANE);
					mat.SetTrans(center);

					// Create the compass (a helper).
					compassObj = (CompassRoseObject*)createInterface->
							CreateInstance(HELPER_CLASS_ID, COMPASS_CLASS_ID); 			
					assert(compassObj);
					dummyNode = createInterface->CreateObjectNode(compassObj);
					createInterface->SetNodeTMRelConstPlane(dummyNode, mat);
					if (theMaster->daylightSystem) {
						// Force the compass to be created in the XY Plane with
						// up in the +Z direction.
						TimeValue t = createInterface->GetTime();
						Matrix3 tm(1);
						tm.SetTrans(dummyNode->GetNodeTM(t).GetTrans());
						dummyNode->SetNodeTM(t, tm);
					}
					createInterface->RedrawViews(t, REDRAW_INTERACTIVE);

					res = TRUE;
					break;
					}

				case 1: {
					// Mouse Drag: Set on-screen size of the compass
					Point3 p1 = vpx->SnapPoint(m,m,NULL,SNAP_IN_PLANE);
					compassObj->axisLength = Length(p1 - center);
					compassObj->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);

							 if (msg == MOUSE_POINT) 
							 {
						INode* newNode;
									if (theMaster->daylightSystem) 
									{
							// Create the NaturalLightAssembly
										 newNode = NatLightAssembly::CreateAssembly(natLightObj, createInterface);
							assert(natLightObj);
							assert (newNode);

							natLightObj->SetMultController(new SlaveControl(theMaster,LIGHT_MULT));
							natLightObj->SetIntenseController(new SlaveControl(theMaster,LIGHT_INTENSE));
							natLightObj->SetSkyCondController(new SlaveControl(theMaster,LIGHT_SKY));
									} 
									else 
									{
							// Create the Light
										 lightObj = 
											 (GenLight *)createInterface->CreateInstance(LIGHT_CLASS_ID,Class_ID(DIR_LIGHT_CLASS_ID,0));
							assert(lightObj);
										 SetUpSun(SYSTEM_CLASS_ID, SUNLIGHT_CLASS_ID, createInterface, lightObj, SUN_RGB);

							// Create a new node given the instance.
							newNode = createInterface->CreateObjectNode(lightObj);
							assert (newNode);
							createInterface->AddLightToScene(newNode);
							TSTR nodename = GetString(IDS_LIGHT_NAME);
							createInterface->MakeNameUnique(nodename);
							newNode->SetName(nodename);
						}

						// Create a new slave controller of the master control.
						SlaveControl* slave = new SlaveControl(theMaster,LIGHT_TM);
						// Set the transform controller used by the node.
						newNode->SetTMController(slave);
						// Attach the new node as a child of the central node.
						dummyNode->AttachChild(newNode);
									INodeMonitor* nodeMon = static_cast<INodeMonitor*>(theMaster->theObjectMon->GetInterface(IID_NODEMONITOR));
									nodeMon->SetNode(newNode);
									//theMaster->thePoint=dummyNode;
						//the master references the point so it can get
						//notified when the	user rotates it
									theMaster->ReplaceReference(REF_POINT, dummyNode );
									DbgAssert(theMaster->thePoint == dummyNode);
						// Indicate that the system is attached
						attachedToNode = TRUE;

						if (daylightSystem) {
										 theMaster->ReplaceReference(REF_LOOKAT, static_cast<Control*>(
								CreateInstance(CTRL_MATRIX3_CLASS_ID, Class_ID(PRS_CONTROL_CLASS_ID,0))));
							Control* lookat = static_cast<Control*>(
								CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(LOOKAT_CONSTRAINT_CLASS_ID, 0)));
							assert(lookat != NULL);
							Animatable* a = lookat;
							ILookAtConstRotation* ilookat = GetILookAtConstInterface(a);
							assert(ilookat != NULL);
							ilookat->AppendTarget(theMaster->thePoint);
							ilookat->SetTargetAxis(2);
							ilookat->SetTargetAxisFlip(true);
							ilookat->SetVLisAbs(false);
							theMaster->theLookat->SetRotationController(lookat);

										 theMaster->ReplaceReference(REF_MULTIPLIER, static_cast<Control*>(
								CreateInstance(CTRL_FLOAT_CLASS_ID, Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))));
						}

						// Reference the node so we'll get notifications.
						// This is done so when the node is selected or 
						// deselected the parameter editing process can 
						// be started and stopped.
									ReplaceReference( 0, newNode, FALSE );
						// Set the initial radius of the system to one
						theMaster->SetRad(TimeValue(0),1.0f);
						// construction plane.  This is used during creating
						// so you can set the position of the node in terms of 
						// the construction plane and not in world units.
						createInterface->SetNodeTMRelConstPlane(dummyNode, mat);
									if (theMaster->daylightSystem) 
									{
							// Force the compass to be created in the XY Plane with
							// up in the +Z direction.
							TimeValue t = createInterface->GetTime();
							Matrix3 tm(1);
							tm.SetTrans(dummyNode->GetNodeTM(t).GetTrans());
							dummyNode->SetNodeTM(t, tm);
						}
					}

	 				createInterface->RedrawViews(t, REDRAW_INTERACTIVE);
							 // This indicates the mouse proc should continue to recieve messages.
					res = TRUE;
					break;
					}

				case 2: {
					// Mouse Pick and release: 
					// The user is dragging the mouse to set the radius
					// (i.e. the distance of the light from the compass center)
					if (node0) {
						// Calculate the radius as the distance from the initial
						// point to the current point
						r = (float)fabs(vpx->SnapLength(vpx->GetCPDisp(center,Point3(0,1,0),pt0,m)));
						// Set the radius at time 0.
						theMaster->SetRad(0, r);
						if (daylightSystem) {
							LightObject* l = natLightObj->GetSun();
							if (l != NULL) 
							{
								l->SetTDist(t, r);
							}
						} else {
							lightObj->SetTDist(t, r);
						}
						theMaster->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
#ifndef NO_CREATE_TASK
						// Update the UI spinner to reflect the current value.
						theMaster->radSpin->SetValue(r, FALSE );
#endif
					}

					res = TRUE;
					if (msg == MOUSE_POINT) {
						// The mouse has been released - finish the creation 
						// process.  Select the first node so if we go into 
						// the motion branch we'll see it's parameters.

						// We set the flag to ignore the selection change
						// because the NotifyRefChanged() method recieves
						// a message when the node is selected or deselected
						// and terminates the parameter editing process.  This
						// flag causes it to ignore the selection change.
						ignoreSelectionChange = TRUE;
									INodeMonitor* nodeMon = 
										static_cast<INodeMonitor*>(theMaster->theObjectMon->GetInterface(IID_NODEMONITOR));
									createInterface->SelectNode(nodeMon->GetNode());
						if (daylightSystem) {
							inCreate = false;
							theMaster->UpdateUI(t);
						}
						ignoreSelectionChange = FALSE;
						// This registers the undo object with the system so
						// the user may undo the creation.
						theHold.Accept(GetString(IDS_UNDO));
						// Set the return value to FALSE to indicate the 
						// creation process is finished.  This mouse proc will
						// no longer be called until the user creates another
						// system.
						res = FALSE;
						}
	 				createInterface->RedrawViews(t, res ? REDRAW_INTERACTIVE : REDRAW_END);
#ifdef RENDER_VER
                    if (FALSE == res) {
                        createInterface->RemoveMode(NULL);
                    }
#endif
					break;
					}
				}
			}
			break;

	    case MOUSE_PROPCLICK:
			// right click while between creations
			createInterface->RemoveMode(NULL);
			break;
		
		case MOUSE_ABORT:
		{
				 // The user has right-clicked the mouse to abort the  creation process.
			assert(theMaster);
			HWND temp = theMaster->hMasterParams;
#ifndef NO_CREATE_TASK
			theMaster->EndEditParams( (IObjParam*)createInterface, 0,NULL );
#endif
				 theHold.Cancel();  
				 DeleteReference(NODE_0_REFERENCE_INDEX);

				 // This creates a new SunMaster object and starts the parameter editing process.
				 CreateNewMaster(temp);  

				 // the viewports are now dirty, schedule a repaint to avoid leaving
				 // compass artifacts behind.  (I've tried a few different methods
				 // to get the screen to refresh correctly upon exit from this 
				 // message processing.  This way seems to be the most reliable.)
				 for(int i=0; i < GetCOREInterface9()->getNumViewports(); ++i)
				 {
					 GetCOREInterface9()->ViewportInvalidate(i);
				 }
			createInterface->RedrawViews(t, REDRAW_NORMAL);

				 // Indicate the nodes have not been hooked up to the system yet.
			attachedToNode = FALSE;
			res = FALSE;						
		}
		break;

		case MOUSE_FREEMOVE:
		{
			vpx->SnapPreview(m,m);
		}
		break;
	} // switch
	
	createInterface->ReleaseViewport(vpx); 
	// Returns TRUE if processing should continue and false if it
	// has been aborted.
	createInterface->GetMacroRecorder()->Enable();  
	return res;
	}

/*===========================================================================*\
 | The Class Descriptors
\*===========================================================================*/
class SunMasterClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new SunMaster(false); }
	// This method returns the name of the class.  This name appears 
	// in the button for the plug-in in the MAX user interface.
	const TCHAR *	ClassName() {
		return GetString(IDS_SUN_CLASS);
	}
	int 			BeginCreate(Interface *i);
	int 			EndCreate(Interface *i);
	SClass_ID		SuperClassID() { return SYSTEM_CLASS_ID; } 
	Class_ID    ClassID() { return SUNLIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return _T("");  }
	};
// A single instance of the class descriptor.
static SunMasterClassDesc mcDesc;
// This returns a pointer to the instance.
ClassDesc* GetSunMasterDesc() { return &mcDesc; }

class DayMasterClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new SunMaster(true); }
	// This method returns the name of the class.  This name appears 
	// in the button for the plug-in in the MAX user interface.
	const TCHAR *	ClassName() {
		return GetString(IDS_DAY_CLASS);
	}
	int 			BeginCreate(Interface *i);
	int 			EndCreate(Interface *i);
	SClass_ID		SuperClassID() { return SYSTEM_CLASS_ID; } 
	Class_ID    ClassID() { return DAYLIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return _T("");  }
	};
// A single instance of the class descriptor.
static DayMasterClassDesc mcDayDesc;
// This returns a pointer to the instance.
ClassDesc* GetDayMasterDesc() { return &mcDayDesc; }

class SlaveControlPosClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading = FALSE) { return new SlaveControl(false); }
	const TCHAR *	ClassName() {
		return GetString(IDS_SLAVE_POS_CLASS);
	}
	SClass_ID		SuperClassID() { return CTRL_POSITION_CLASS_ID; }
	Class_ID		ClassID() 
		{ return Class_ID(SLAVE_CONTROL_CID1,SLAVE_CONTROL_CID2); }
	// The slave controllers don't appear in any of the drop down lists, 
	// so they just return a null string.
	const TCHAR* 	Category() { return _T("");  }
	};


class SlaveControlFloatClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading = FALSE) { return new SlaveControl(false); }
	const TCHAR *	ClassName() {
		return GetString(IDS_SLAVE_FLOAT_CLASS);
	}
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() 
		{ return Class_ID(SLAVE_CONTROL_CID1,SLAVE_CONTROL_CID2); }
	// The slave controllers don't appear in any of the drop down lists, 
	// so they just return a null string.
	const TCHAR* 	Category() { return _T("");  }
	};


class DaySlaveControlPosClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading = FALSE) { return new SlaveControl(true); }
	const TCHAR *	ClassName() {
		return GetString(IDS_SLAVE_POS_CLASS);
	}
	SClass_ID		SuperClassID() { return CTRL_POSITION_CLASS_ID; }
	Class_ID		ClassID() 
		{ return Class_ID(DAYLIGHT_SLAVE_CONTROL_CID1,DAYLIGHT_SLAVE_CONTROL_CID2); }
	// The slave controllers don't appear in any of the drop down lists, 
	// so they just return a null string.
	const TCHAR* 	Category() { return _T("");  }
	};


class DaySlaveControlFloatClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading = FALSE) { return new SlaveControl(true); }
	const TCHAR *	ClassName() {
		return GetString(IDS_SLAVE_FLOAT_CLASS);
	}
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() 
		{ return Class_ID(DAYLIGHT_SLAVE_CONTROL_CID1,DAYLIGHT_SLAVE_CONTROL_CID2); }
	// The slave controllers don't appear in any of the drop down lists, 
	// so they just return a null string.
	const TCHAR* 	Category() { return _T("");  }
	};

// A single instance of the class descriptor.
static SlaveControlPosClassDesc slvPosDesc;
// This returns a pointer to the instance.
ClassDesc* GetSlavePosControlDesc() { return &slvPosDesc; }

// A single instance of the class descriptor.
static SlaveControlFloatClassDesc slvFloatDesc;
// This returns a pointer to the instance.
ClassDesc* GetSlaveFloatControlDesc() { return &slvFloatDesc; }

// A single instance of the class descriptor.
static DaySlaveControlPosClassDesc daySlvPosDesc;
// This returns a pointer to the instance.
ClassDesc* GetDaySlavePosControlDesc() { return &daySlvPosDesc; }

// A single instance of the class descriptor.
static DaySlaveControlFloatClassDesc daySlvFloatDesc;
// This returns a pointer to the instance.
ClassDesc* GetDaySlaveFloatControlDesc() { return &daySlvFloatDesc; }

// This is the method of the class descriptor that actually begins the 
// creation process in the viewports.
int SunMasterClassDesc::BeginCreate(Interface *i)
	{
	// Save the interface pointer passed in.  This is used to call 
	// methods provided by MAX itself.
	IObjCreate *iob = i->GetIObjCreate();
	
	theSunMasterCreateMode.Begin( iob, this, false );
	// Set the current command mode to the SunMasterCreateMode.
	iob->PushCommandMode( &theSunMasterCreateMode );
	
	return TRUE;
	}

// This is the method of the class descriptor that terminates the 
// creation process.
int SunMasterClassDesc::EndCreate(Interface *i)
	{
	theSunMasterCreateMode.End();
	// Remove the command mode from the command stack.
	i->RemoveMode( &theSunMasterCreateMode );
	return TRUE;
	}

void ReplaceChar( TSTR& str, TCHAR searchChar, TCHAR replaceChar )
	{
	for( int i=0; i<str.Length(); i++ )
		{
		if( str[i]==searchChar ) str[i] = replaceChar;
		}
	}

// Checks if an appropriate exposure control is currently applied, for this light type.
// If not, the user is prompted with the option to automatically apply an approriate exposure control.
// This function calls the underlying function implemented in MaxScript.
// TO DO: Remove duplicated function in photometric Free Lights and Target Lights.
void ValidateExposureControl(ClassDesc* classDesc)
	{
	TSTR className = classDesc->ClassName();
	ReplaceChar( className, _T(' '), _T('_') ); // Repalce spaces with underscores

	// Set up the command to execute in MaxScript
	TSTR execString;
	execString.printf( _T("ValidateExposureControlForLight %s"), className );

	// Execute the command in MaxScript
	FPValue retval;
	BOOL quietErrors = TRUE;
	ExecuteMAXScriptScript( execString, quietErrors, &retval );
	}

// This is the method of the class descriptor that actually begins the 
// creation process in the viewports.
int DayMasterClassDesc::BeginCreate(Interface *i)
	{
	ValidateExposureControl(this);

	// Save the interface pointer passed in.  This is used to call 
	// methods provided by MAX itself.
	IObjCreate *iob = i->GetIObjCreate();

	theSunMasterCreateMode.Begin( iob, this, true );
	// Set the current command mode to the SunMasterCreateMode.
	iob->PushCommandMode( &theSunMasterCreateMode );
	
	return TRUE;
	}

// This is the method of the class descriptor that terminates the 
// creation process.
int DayMasterClassDesc::EndCreate(Interface *i)
	{
	theSunMasterCreateMode.End();
	// Remove the command mode from the command stack.
	i->RemoveMode( &theSunMasterCreateMode );
	return TRUE;
	}

#pragma unmanaged

// This function is called by Windows when the DLL is loaded.  This 
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
	{	
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
	// Hang on to this DLL's instance handle.
	hInstance = hinstDLL;
		DisableThreadLibraryCalls(hInstance);
	}
	
	return(TRUE);
	}

__declspec(dllexport) const TCHAR *
LibDescription() { return GetString(IDS_LIB_DESC); 
	}

__declspec( dllexport ) int 
LibNumberClasses()
{
if (Get3DSMAXVersion() < 120)
return 0;

return 8;
}



__declspec(dllexport) ClassDesc* 
LibClassDesc(int i) { 
	switch(i) {
		case 0:	return GetSunMasterDesc();
		case 1:	return GetDayMasterDesc();
		case 2:	return GetSlavePosControlDesc();
		case 3:	return GetSlaveFloatControlDesc();
		case 4:	return GetDaySlavePosControlDesc();
		case 5:	return GetDaySlaveFloatControlDesc();
		case 6:	return GetCompassRoseDesc();
		case 7: return GetNatLightAssemblyDesc();
		default: return 0;
	};
}

__declspec( dllexport ) ULONG
LibVersion() {  return VERSION_3DSMAX; }


TCHAR *GetString(int id)
	{
		static TCHAR buf[256] = {0};

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	return NULL;
	}

__declspec(dllexport) BOOL LibShutdown()
{
	// clean up all singletons
	CityList::Destroy();

	return TRUE;
}
