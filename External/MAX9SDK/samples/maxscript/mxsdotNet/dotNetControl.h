//**************************************************************************/
// Copyright (c) 1998-2006 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: dotNetControl.h  : MAXScript dotNetControl header
// AUTHOR: Larry.Minton - created Jan.1.2006
//***************************************************************************/
//

#pragma once

#include "stdafx.h"

// 
#pragma unmanaged
#include "MAXScrpt.h"
#include "Numbers.h"
#include "parser.h"
#include "funcs.h"
#include "thunks.h"
#include "hashtab.h"
#include "macrorec.h"

#ifdef ScripterExport
	#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

namespace MXS_dotNet
{
class dotNetControl;
class dotNetObject;
class dotNetMethod;
class dotNetClass;
class dotNetMXSValue;

#pragma managed
bool ShowMethodInfo(System::Type ^type, MCHAR* pattern, System::Text::StringBuilder ^sb, 
				    bool showStaticOnly, bool showSpecial, bool showAttributes, bool declaredOnTypeOnly);
bool ShowPropertyInfo(System::Type ^type, MCHAR* pattern, System::Text::StringBuilder ^sb, 
					  bool showStaticOnly, bool showMethods, bool showAttributes, bool declaredOnTypeOnly);
bool ShowEventInfo(System::Type ^type, MCHAR* pattern, System::Text::StringBuilder ^sb, 
				   bool showStaticOnly, bool declaredOnTypeOnly);
bool ShowFieldInfo(System::Type ^type, MCHAR* pattern, System::Text::StringBuilder ^sb, 
				   bool showStaticOnly, bool showAttributes, bool declaredOnTypeOnly);
/*
// see comments in utils.cpp
bool ShowInterfaceInfo(System::Type ^type, MCHAR* pattern, System::Text::StringBuilder ^sb);
void GetInterfaces(System::Type ^type, Array* result);
bool ShowAttributeInfo(System::Type ^type, MCHAR* pattern, System::Text::StringBuilder ^sb, bool declaredOnTypeOnly);
void GetAttributes(System::Type ^type, Array* result);
*/
bool ShowConstructorInfo(System::Type ^type, System::Text::StringBuilder ^sb);
void GetPropertyAndFieldNames(System::Type ^type, Array* propNames, 
							  bool showStaticOnly, bool declaredOnTypeOnly);
System::Object^ MXS_Value_To_Object(Value* val, System::Type ^ type);
Value* Object_To_MXS_Value(System::Object^ val);
System::Type^ ResolveTypeFromName(MCHAR* pTypeString);
void PrepArgList(array<System::Reflection::ParameterInfo^> ^candidateParamList, 
				 Value** arg_list, array<System::Object^> ^argArray, int count);
int FindMethodAndPrepArgList(System::Collections::Generic::List<array<System::Reflection::ParameterInfo^>^> ^candidateParamLists, 
					   Value** arg_list, array<System::Object^> ^argArray, int count, Array* userSpecifiedParamTypes);
int CalcParamScore(Value *val, System::Type^ type);
Value* CombineEnums(Value** arg_list, int count);
bool CompareEnums(Value** arg_list, int count);
Value* LoadAssembly(MCHAR* assyDll);

// Note that this functionality should probably be added to CStr/WStr at some point
struct MNETStr
{
    static MSTR ToMSTR(System::String^);
    static MSTR ToMSTR(System::Exception^, bool InnerException = true);
};

/* -------------------- event_delegate_pair  ------------------- */
// 
#pragma managed
public ref class event_delegate_pair
{
public:
	System::String ^m_pEventName;
	System::Delegate ^m_pEventDelegate;
	event_delegate_pair(System::String ^pEventName, System::Delegate ^pEventDelegate)
	{
		m_pEventName = pEventName;
		m_pEventDelegate = pEventDelegate;
	}
};

/* -------------------- DotNetObjectWrapper  ------------------- */
// 
#pragma managed
public class DotNetObjectWrapper
{
public:
	static Value* intern(System::Object ^ object); // wraps existing System::Object, returns dotNetObject or dotNetControl
	static Value* WrapType(System::Type ^ pType); // wraps type, returns dotNetObject
	static Value* WrapType(MCHAR* pTypeString); // Finds and wraps type, returns dotNetObject
	static Value* WrapType(DotNetObjectWrapper* pDotNetObjectWrapper); // Wraps type of DotNetObjectWrapper, returns dotNetObject
	virtual System::Object ^ GetObject() = 0;
	virtual System::Type ^ GetType();
	virtual Value* GetMXSContainer() = 0; //  returns dotNetObject, dotNetControl, or dotNetClass
	// derived classes must implement:
	// returns dotNetObject or dotNetControl if object already wrapped
	// static Value* GetMXSContainerForObject(System::Object ^ object);
	virtual void StoreEventDelegatePair(event_delegate_pair ^ pEventDelegatePair) = 0;
	virtual MSTR GetTypeName();
	virtual Value* GetEventHandlers(Value* eventName) { return &undefined; }
	Value* get_property(Value** arg_list, int count);
	Value* set_property(Value** arg_list, int count);
	void get_property_names(Array* propNames, bool showStaticOnly, bool declaredOnTypeOnly);
	bool show_properties(MCHAR* pattern, CharStream* out, bool showStaticOnly, bool showMethods, bool showAttributes, bool declaredOnTypeOnly);
	bool show_methods(MCHAR* pattern, CharStream* out, bool showStaticOnly, bool showSpecial, bool showAttributes, bool declaredOnTypeOnly);
	bool show_events(MCHAR* pattern, CharStream* out, bool showStaticOnly, bool declaredOnTypeOnly);
	bool show_fields(MCHAR* pattern, CharStream* out, bool showStaticOnly, bool showAttributes, bool declaredOnTypeOnly);
/*
// see comments in utils.cpp
	bool show_interfaces(MCHAR* pattern, CharStream* out);
	void get_interfaces(Array* result);
	bool show_attributes(MCHAR* pattern, CharStream* out, bool declaredOnTypeOnly);
	void get_attributes(Array* result);
*/
	bool show_constructors(CharStream* out);
	void CreateEventHandlerDelegates(System::Object ^ targetWrapper, System::Reflection::MethodInfo ^mi);
	void RunEventHandlers(Value* target, Value* eventName, Array* handlers, array<System::Object^> ^delegateArgsArray);
	bool HasEvent(MCHAR *eventName);
	virtual void ProcessEvent(System::String ^eventName, System::Object ^delegateArgs);
};

/* -------------------- CDotNetHostWnd  ------------------- */
// 
#pragma managed

public class CDotNetHostWnd : public CWnd, public DotNetObjectWrapper
{
	typedef CWnd inherited;

public:
	ref class delegate_proxy_type;
	msclr::delegate_map::internal::delegate_proxy_factory<CDotNetHostWnd> m_delegate_map_proxy;

	ref class delegate_proxy_type
	{
		CDotNetHostWnd* m_p_native_target;
		// the control 
		System::Windows::Forms::Control ^ m_pControl;
		// the owning MXS dotNetControl
		dotNetControl *m_p_dotNetControl;
		// existing wrapped controls
		static System::Collections::Hashtable ^ m_pWrappedControls;
		// event/delegate pairs for removing the delegates from the events when done with the object
		System::Collections::Generic::List<event_delegate_pair^> ^ m_pEventDelegatePairs;
		// flag that we are in the process of running the ControlResizeMethod method
		bool m_in_ResizeMethod;

	public:
		delegate_proxy_type(CDotNetHostWnd* pNativeTarget) : 
		  m_p_native_target(pNativeTarget), m_p_dotNetControl(NULL), m_in_ResizeMethod(false) {}
		void detach();

		// this method is called by a dynamic method attached to a delegate, which is then attached to an event
		// eventName is the event name, delegateArgs is an array containing the parameters passed to the delegate method
		// (sender, EventArgs)
		void ProcessEvent(System::String ^eventName, System::Object ^delegateArgs);

		void ControlSetFocusMethod(System::Object ^ sender, System::EventArgs ^e);
		System::EventHandler^ m_pSetFocusHandler;
		void ControlResizeMethod(System::Object ^ sender, System::EventArgs ^e);
		System::EventHandler^ m_pResizeHandler;

		System::Windows::Forms::Control ^ get_control() { return m_pControl; }
		void set_control(System::Windows::Forms::Control ^ control);
		dotNetControl* get_dotNetControl() { return m_p_dotNetControl; }
		void set_dotNetControl(dotNetControl *_dotNetControl) { m_p_dotNetControl = _dotNetControl; }
		void store_EventDelegatePair(event_delegate_pair ^ pEventDelegatePair);
		static void init();
		static Value* get_MXS_container_for_object(System::Object ^ object);
	};

public:
	CWinFormsControl<System::Windows::Forms::Control> m_CWinFormsControl;

public:
	CDotNetHostWnd(dotNetControl *p_dotNetControl);

	~CDotNetHostWnd();

	BOOL Create(HWND hParent, int id, int x, int y, int w, int h, MCHAR* pControlTypeString, Value **keyparms, int keyparm_count);
	void CreateEventHandlerDelegates();
	static void Init();
	static Value* GetMXSContainerForObject(System::Object ^ object); //  returns dotNetControl if object already wrapped
	void CreateSetFocusEventHandler();
	void CreateResizeEventHandler();
	void FitToChildWindow();

	// DotNetObjectWrapper
	System::Object ^ GetObject();
	Value* GetMXSContainer();
	void StoreEventDelegatePair(event_delegate_pair ^ pEventDelegatePair);

	afx_msg void OnDestroy();

	// This method will be called when any event is raised on the user control
	void ProcessEvent(System::String ^eventName, System::Object ^delegateArgs);

	BOOL set_focus();

private:
	DECLARE_MESSAGE_MAP()
};


/* -------------------- dotNetControl  ------------------- */
// 
#pragma unmanaged

visible_class (dotNetControl)

class dotNetControl : public RolloutControl
{
public:
	CDotNetHostWnd	*m_pDotNetHostWnd;   // To host .NET control
	MSTR			m_typeName;

	dotNetControl(Value* name, Value* caption, Value** keyparms, int keyparm_count);

	~dotNetControl();

	static	RolloutControl* create(Value* name, Value* caption, Value** keyparms, int keyparm_count)
	{ return new dotNetControl (name, caption, keyparms, keyparm_count); }

	void		gc_trace();
#	define		is_dotNetControl(p) ((p)->tag == class_tag(dotNetControl))
	classof_methods (dotNetControl, RolloutControl);
	void		collect() { delete this; }
	void		sprin1(CharStream* s);

	DotNetObjectWrapper* GetDotNetObjectWrapper() { return m_pDotNetHostWnd; }

	void		add_control(Rollout *ro, HWND parent, HINSTANCE hInstance, int& current_y);
	LPCMSTR		get_control_class() { return _M("STATIC"); }
	BOOL		handle_message(Rollout *ro, UINT message, WPARAM wParam, LPARAM lParam);
	void		set_enable();
	BOOL		set_focus();

	Value*		get_property(Value** arg_list, int count);
	Value*		set_property(Value** arg_list, int count);

	// operations
	def_generic_debug_ok	( show_props,			"showProperties");
	def_generic_debug_ok	( get_props,			"getPropNames");
	def_generic_debug_ok	( show_methods,			"showMethods");
	def_generic_debug_ok	( show_events,			"showEvents");
/*
// see comments in utils.cpp
	def_generic_debug_ok	( show_interfaces,		"showInterfaces");
	def_generic_debug_ok	( get_interfaces,		"getInterfaces");
*/
};

/* -------------------- DotNetObject  ------------------- */
// 
#pragma managed

public class DotNetObject : public DotNetObjectWrapper
{
public:
	DotNetObject(dotNetObject* p_dotNetObject);
	DotNetObject(System::Object ^ object, dotNetObject* p_dotNetObject);
	~DotNetObject();

	ref class delegate_proxy_type;
	msclr::delegate_map::internal::delegate_proxy_factory<DotNetObject> m_delegate_map_proxy;

	ref class delegate_proxy_type
	{
		DotNetObject* m_p_native_target;
		// the object 
		System::Object ^ m_pObject;
		// the owning MXS dotNetObject
		dotNetObject *m_p_dotNetObject;
		// existing wrapped objects
		static System::Collections::Hashtable ^ m_pWrappedObjects;
		// event/delegate pairs for removing the delegates from the events when done with the object
		System::Collections::Generic::List<event_delegate_pair^> ^ m_pEventDelegatePairs;

	public:
		delegate_proxy_type(DotNetObject* pNativeTarget) : m_p_native_target(pNativeTarget), m_p_dotNetObject(NULL) {}
		void detach();

		// this method is called by a dynamic method attached to a delegate, which is then attached to an event
		// eventName is the event name, delegateArgs is an array containing the parameters passed to the delegate method
		// (sender, EventArgs)
		void ProcessEvent(System::String ^eventName, System::Object ^delegateArgs);

		System::Object ^ get_object() { return m_pObject; }
		void set_object(System::Object ^ object);
		dotNetObject* get_dotNetObject() { return m_p_dotNetObject; }
		void set_dotNetObject(dotNetObject *_dotNetObject) { m_p_dotNetObject = _dotNetObject; }
		void store_EventDelegatePair(event_delegate_pair ^ pEventDelegatePair);
		static void init();
		static Value* get_MXS_container_for_object(System::Object ^ object);
	};

	static Value* Create(MCHAR* pTypeString, dotNetClass* p_dotNetClass, Value** arg_list, int count);
	void CreateEventHandlerDelegates();
	static void Init();
	static Value* GetMXSContainerForObject(System::Object ^ object); //  returns dotNetObject if object already wrapped
	bool Compare(DotNetObject * pCompareVal);
	Value* Copy();

	// DotNetObjectWrapper
	System::Object ^ GetObject();
	Value* GetMXSContainer();
	void StoreEventDelegatePair(event_delegate_pair ^ pEventDelegatePair);
	Value* GetEventHandlers(Value* eventName);
};

/* -------------------- dotNetBase  ------------------- */
// 
#pragma unmanaged

// define base class to be used by dotNetObject and dotNetClass
class dotNetBase : public Value
{
protected:
	dotNetBase() : m_pEventHandlers(NULL) {}
public:
	MSTR		m_typeName;
	HashTable	*m_pEventHandlers;
	virtual DotNetObjectWrapper* GetDotNetObjectWrapper() = 0;
#	define		is_dotNetBase(p) ((p)->tag == class_tag(dotNetObject) || (p)->tag == class_tag(dotNetClass))
	virtual	Value* DefaultShowStaticOnly() { return &false_value; }

	void		gc_trace();

	Value*		get_property(Value** arg_list, int count);
	Value*		set_property(Value** arg_list, int count);

	// operations
	def_generic_debug_ok	( show_props,			"showProperties");
	def_generic_debug_ok	( get_props,			"getPropNames");
	def_generic_debug_ok	( show_methods,			"showMethods");
	def_generic_debug_ok	( show_events,			"showEvents");
/*
// see comments in utils.cpp
	def_generic_debug_ok	( show_interfaces,		"showInterfaces");
	def_generic_debug_ok	( get_interfaces,		"getInterfaces");
*/

	Value*		get_event_handlers(Value* eventName);
	void		add_event_handler(Value* eventName, Value* handler);
	void		remove_event_handler(Value* eventName, Value* handler);
	void		remove_all_event_handlers(Value* eventName);
	void		remove_all_event_handlers();
};

/* -------------------- dotNetObject  ------------------- */
// 
#pragma unmanaged

applyable_class_debug_ok (dotNetObject);

class dotNetObject : public dotNetBase
{
public:
	DotNetObject    *m_pDotNetObject;   // To host .NET object

	// use DotNetObject::intern to wrap existing System::Object vals
	// use DotNetObject::create to create and wrap new System::Object vals
	dotNetObject(); 
	~dotNetObject();

#	define		is_dotNetObject(p) ((p)->tag == class_tag(dotNetObject))
	classof_methods (dotNetObject, Value);
	void		collect() { delete this; }

	void		sprin1(CharStream* s);

	use_generic( eq,		"=");
	use_generic( ne,		"!=");

	Value*  copy_vf(Value** arg_list, int count);

	DotNetObjectWrapper* GetDotNetObjectWrapper() { return m_pDotNetObject; }
};


/* -------------------- DotNetClass  ------------------- */
// 
#pragma managed


public class DotNetClass : public DotNetObjectWrapper
{
public:
	DotNetClass(System::Type ^ p_type, dotNetClass* p_dotNetClass);
	~DotNetClass();

	ref class delegate_proxy_type;
	msclr::delegate_map::internal::delegate_proxy_factory<DotNetClass> m_delegate_map_proxy;

	ref class delegate_proxy_type
	{
		DotNetClass* m_p_native_target;
		// the object 
		System::Type ^ m_pType;
		// the owning MXS dotNetClass
		dotNetClass *m_p_dotNetClass;
		// existing wrapped objects
		static System::Collections::Hashtable ^ m_pWrappedClasses;
		// event/delegate pairs for removing the delegates from the events when done with the object
		System::Collections::Generic::List<event_delegate_pair^> ^ m_pEventDelegatePairs;

	public:
		delegate_proxy_type(DotNetClass* pNativeTarget) : m_p_native_target(pNativeTarget), m_p_dotNetClass(NULL) {}
		void detach();

		// this method is called by a dynamic method attached to a delegate, which is then attached to an event
		// eventName is the event name, delegateArgs is an array containing the parameters passed to the delegate method
		// (sender, EventArgs)
		void ProcessEvent(System::String ^eventName, System::Object ^delegateArgs);

		System::Type ^ get_type() { return m_pType; }
		void set_type(System::Type ^ type);
		dotNetClass* get_dotNetClass() { return m_p_dotNetClass; }
		void set_dotNetClass(dotNetClass *_dotNetClass) { m_p_dotNetClass = _dotNetClass; }
		void store_EventDelegatePair(event_delegate_pair ^ pEventDelegatePair);
		static void init();
		static Value* get_MXS_container_for_type(System::Type ^ type);
	};

	static void Init();
	static Value* Create(MCHAR* pTypeString);
	static Value* Create(DotNetObjectWrapper* wrapper);
	static Value* Create(System::Type ^ type);
	void CreateEventHandlerDelegates();
	static Value* GetMXSContainerForObject(System::Object ^ object); //  returns dotNetClass if object already wrapped

	// DotNetObjectWrapper
	System::Object ^ GetObject();
	System::Type ^ GetType();
	MSTR GetTypeName();
	Value* GetMXSContainer();
	void StoreEventDelegatePair(event_delegate_pair ^ pEventDelegatePair);
	Value* GetEventHandlers(Value* eventName);
};


/* -------------------- dotNetClass  ------------------- */
// 
#pragma unmanaged

applyable_class_debug_ok (dotNetClass);

class dotNetClass : public dotNetBase
{
public:
	DotNetClass		*m_pDotNetClass;   // To host .NET object

	// use DotNetObject::intern to wrap existing System::Object vals
	// use DotNetObject::create to create and wrap new System::Object vals
	dotNetClass(); 
	~dotNetClass();

#	define		is_dotNetClass(p) ((p)->tag == class_tag(dotNetClass))
	classof_methods (dotNetClass, Value);
	void		collect() { delete this; }

	void		sprin1(CharStream* s);

	DotNetObjectWrapper* GetDotNetObjectWrapper() { return m_pDotNetClass; }
	Value* DefaultShowStaticOnly() { return &true_value; }
};


/* -------------------- DotNetMethod  ------------------- */
// 
#pragma managed

// TODO: need a cache of methods. Doing this will require the wrapper based on the System::Type
// (see comments on DotNetObjectWrapper::get_property
public class DotNetMethod
{
public:

	DotNetMethod::DotNetMethod(System::String ^ pMethodName, System::Type ^ pWrapperType);
	static dotNetMethod* intern(System::String ^ pMethodName, DotNetObjectWrapper *pWrapper, System::Type ^ pObjectType);
	Value*		apply(DotNetObjectWrapper *pWrapper, Value** arglist, int count, bool asDotNetObject);

	ref class delegate_proxy_type;
	msclr::delegate_map::internal::delegate_proxy_factory<DotNetMethod> m_delegate_map_proxy;

	ref class delegate_proxy_type
	{
		DotNetMethod* m_p_native_target;
		// the method name 
		System::String ^ m_p_MethodName;
		// the type the method is associated with
		System::Type ^ m_p_ObjectType;
		// the owning MXS dotNetMethod
		dotNetMethod *m_p_dotNetMethod;

	public:
		delegate_proxy_type(DotNetMethod* pNativeTarget) : m_p_native_target(pNativeTarget), m_p_dotNetMethod(NULL) {}
		void detach();

		System::String ^ get_MethodName() { return m_p_MethodName; }
		void set_MethodName(System::String ^ name) { m_p_MethodName = name; }
		System::Type ^ get_type() { return m_p_ObjectType; }
		void set_type(System::Type ^ type) { m_p_ObjectType = type; }
		dotNetMethod* get_dotNetMethod() { return m_p_dotNetMethod; }
		void set_dotNetMethod(dotNetMethod *_dotNetMethod) { m_p_dotNetMethod = _dotNetMethod; }
	};

};

/* -------------------- dotNetMethod  ------------------- */
// 
#pragma unmanaged

visible_class (dotNetMethod)

class dotNetMethod : public Function
{
public:
	DotNetObjectWrapper	*m_pWrapper;
	DotNetMethod		*m_pMethod;

	dotNetMethod(MCHAR* pMethodName, DotNetObjectWrapper* pWrapper);
	~dotNetMethod();

	void		gc_trace();
#	define		is_dotNetMethod(p) ((p)->tag == class_tag(dotNetMethod))
	classof_methods (dotNetMethod, Function);
	void		collect() { delete this; }

	Value*		apply(Value** arglist, int count, CallContext* cc);
	Value*		get_vf(Value** arg_list, int count);
	Value*		put_vf(Value** arg_list, int count);
};


/* -------------------- DotNetMXSValue  ------------------- */
// 
#pragma managed

public class DotNetMXSValue
{
public:

	DotNetMXSValue::DotNetMXSValue(dotNetMXSValue *p_dotNetMXSValue);

	ref class delegate_proxy_type;
	msclr::delegate_map::internal::delegate_proxy_factory<DotNetMXSValue> m_delegate_map_proxy;

	ref class DotNetMXSValue_proxy
	{
		delegate_proxy_type ^ m_p_delegate_proxy_type;
	public:
		DotNetMXSValue_proxy(delegate_proxy_type ^ p_delegate_proxy_type) : m_p_delegate_proxy_type(p_delegate_proxy_type) {}
		delegate_proxy_type ^ get_proxy() { return m_p_delegate_proxy_type; }
		Value* get_value();
	};

	ref class delegate_proxy_type
	{
		DotNetMXSValue* m_p_native_target;
		// the owning MXS dotNetMXSValue
		dotNetMXSValue *m_p_dotNetMXSValue;
		// the weak reference that points at the object can be used by .net objects
		System::WeakReference^ m_pDotNetMXSValue_proxy_weakRef;

	public:
		delegate_proxy_type(DotNetMXSValue* pNativeTarget) : m_p_native_target(pNativeTarget), m_p_dotNetMXSValue(NULL) {}
		void detach();

		dotNetMXSValue* get_dotNetMXSValue() { return m_p_dotNetMXSValue; }
		void set_dotNetMXSValue(dotNetMXSValue *_dotNetMXSValue) { m_p_dotNetMXSValue = _dotNetMXSValue; }
		DotNetMXSValue_proxy ^ get_weak_proxy();
		bool is_alive();
	};
	dotNetMXSValue* Get_dotNetMXSValue();
	DotNetMXSValue_proxy ^ GetWeakProxy();
	bool IsAlive();

};

/* -------------------- dotNetMXSValueHashTable  ------------------- */
// 
#pragma unmanaged

invisible_class (dotNetMXSValueHashTable)
class dotNetMXSValueHashTable : public Value
{
public:
	HashTable	*m_pExisting_dotNetMXSValues;
	dotNetMXSValueHashTable();
	~dotNetMXSValueHashTable();
	void	gc_trace();
	void	collect() { delete this; }
	Value*	get(void* key);
	Value*	put(void* key, void* val);
	void	remove(void* key);
};

/* -------------------- dotNetMXSValue  ------------------- */
// 
#pragma unmanaged

applyable_class_debug_ok (dotNetMXSValue)

class dotNetMXSValue : public Value
{
	dotNetMXSValue(Value *pMXSValue); // use intern

public:
	DotNetMXSValue		*m_pDotNetMXSValue;
	Value				*m_pMXSValue;
	static dotNetMXSValueHashTable	*m_pExisting_dotNetMXSValues;

	~dotNetMXSValue();
	static dotNetMXSValue* intern(Value *pMXSValue);
	Value*		get_value() { return m_pMXSValue; }

	void		gc_trace();
#	define		is_dotNetMXSValue(p) ((p)->tag == class_tag(dotNetMXSValue))
	classof_methods (dotNetMXSValue, Value);
	void		collect() { delete this; }
	void		sprin1(CharStream* s);

	Value*		get_property(Value** arg_list, int count);
	Value*		set_property(Value** arg_list, int count);

	// operations
	def_generic_debug_ok	( show_props,			"showProperties");
	def_generic_debug_ok	( get_props,			"getPropNames");

};

/* -------------------- ListViewItemComparer  ------------------- */
// 
#pragma managed

// Implements the manual sorting of items by columns.
public ref class ListViewItemComparer: public System::Collections::IComparer
{
private:
	int col;

public:
	ListViewItemComparer()
	{
		col = 0;
	}

	ListViewItemComparer( int column )
	{
		col = column;
	}

	virtual int Compare( Object^ x, Object^ y )
	{
		return System::String::Compare( (dynamic_cast<System::Windows::Forms::ListViewItem^>(x))->SubItems[ col ]->Text,
			(dynamic_cast<System::Windows::Forms::ListViewItem^>(y))->SubItems[ col ]->Text );
	}
};


}  // end of namespace MXS_dotNet

