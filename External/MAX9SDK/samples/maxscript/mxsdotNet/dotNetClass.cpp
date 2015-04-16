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
// DESCRIPTION: dotNetClass.cpp  : MAXScript dotNetClass code
// AUTHOR: Larry.Minton - created Jan.1.2006
//***************************************************************************/
//

#include "dotNetControl.h"
#include "resource.h"

#include "defextfn.h"
#	include "namedefs.h"

#ifdef ScripterExport
	#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

using namespace MXS_dotNet;

/* -------------------- DotNetClass  ------------------- */
// 
#pragma managed

DotNetClass::DotNetClass(System::Type ^ p_type, dotNetClass* p_dotNetClass)
{
	m_delegate_map_proxy.get_proxy(this)->set_type(p_type);
	m_delegate_map_proxy.get_proxy(this)->set_dotNetClass(p_dotNetClass);
	if (p_dotNetClass)
		CreateEventHandlerDelegates();
}

DotNetClass::~DotNetClass()
{
}

void DotNetClass::delegate_proxy_type::init()
{
	m_pWrappedClasses = gcnew System::Collections::Hashtable(53);
}

void DotNetClass::Init()
{
	delegate_proxy_type::init();
}

Value* DotNetClass::Create(MCHAR* pTypeString)
{
	try
	{
		// Create the user object.
		System::Type ^l_pType = ResolveTypeFromName(pTypeString);
		if (l_pType)
		{
			// see if already have a wrapper
			one_value_local(result);
			vl.result = GetMXSContainerForObject(l_pType);
			if (!vl.result)
			{
				dotNetClass *l_p_dotNetClass;
				vl.result = l_p_dotNetClass = new dotNetClass();
				l_p_dotNetClass->m_pDotNetClass = new DotNetClass(l_pType, l_p_dotNetClass);
			}
			return_value(vl.result);
		}
		return &undefined;
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(_M("dotNet runtime exception: "), MNETStr::ToMSTR(e));
	}
}

Value* DotNetClass::Create(DotNetObjectWrapper* wrapper)
{
	try
	{
		System::Object ^ pObject = wrapper->GetObject();
		System::Type ^ l_pType = wrapper->GetType();

		// if object is a type, create DotNetClass for the type. Otherwise get the type of the object
		// and create DotNetClass for that type
		if (pObject)
		{
			System::Type ^ pType = dynamic_cast<System::Type ^>(pObject);
			if (pType)
				l_pType = pType;
		}

		if (l_pType)
		{
			// see if already have a wrapper
			one_value_local(result);
			vl.result = GetMXSContainerForObject(l_pType);
			if (!vl.result)
			{
				dotNetClass *l_p_dotNetClass;
				vl.result = l_p_dotNetClass = new dotNetClass();
				l_p_dotNetClass->m_pDotNetClass = new DotNetClass(l_pType, l_p_dotNetClass);
			}
			return_value(vl.result);
		}
		return &undefined;
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(_M("dotNet runtime exception: "), MNETStr::ToMSTR(e));
	}
}

Value* DotNetClass::Create(System::Type ^ type)
{
	try
	{
		// see if already have a wrapper
		one_value_local(result);
		vl.result = GetMXSContainerForObject(type);
		if (!vl.result)
		{
			dotNetClass *l_p_dotNetClass;
			vl.result = l_p_dotNetClass = new dotNetClass();
			l_p_dotNetClass->m_pDotNetClass = new DotNetClass(type, l_p_dotNetClass);
		}
		return_value(vl.result);
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(_M("dotNet runtime exception: "), MNETStr::ToMSTR(e));
	}
}

void DotNetClass::CreateEventHandlerDelegates()
{
	using namespace System;
	using namespace System::Reflection;
	using namespace System::Reflection::Emit;
	// Event handler methods are generated at run time using lightweight dynamic methods and Reflection.Emit. 
	// get the method that the delegate's dynamic method will be calling

	try
	{
		Type ^ proxyType = m_delegate_map_proxy.get_proxy(this)->GetType();
		MethodInfo ^mi = proxyType->GetMethod("ProcessEvent");
		DotNetObjectWrapper::CreateEventHandlerDelegates(m_delegate_map_proxy.get_proxy(this), mi);
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(_M("dotNet runtime exception: "), MNETStr::ToMSTR(e));
	}
}

void DotNetClass::delegate_proxy_type::ProcessEvent(System::String ^eventName, System::Object ^delegateArgs)
{
	DbgAssert(m_p_native_target);
	if(m_p_native_target)
		m_p_native_target->ProcessEvent(eventName, delegateArgs);
}

System::Object ^ DotNetClass::GetObject()
{
	return nullptr;
}

System::Type ^ DotNetClass::GetType()
{
	return m_delegate_map_proxy.get_proxy(this)->get_type();
}

Value* DotNetClass::GetMXSContainer()
{
	return m_delegate_map_proxy.get_proxy(this)->get_dotNetClass();
}

void DotNetClass::delegate_proxy_type::store_EventDelegatePair(event_delegate_pair ^ pEventDelegatePair)
{
	try
	{
		if (!m_pEventDelegatePairs)
			m_pEventDelegatePairs = gcnew System::Collections::Generic::List<event_delegate_pair^>();
		m_pEventDelegatePairs->Add(pEventDelegatePair);
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(_M("dotNet runtime exception: "), MNETStr::ToMSTR(e));
	}
}

void DotNetClass::StoreEventDelegatePair(event_delegate_pair ^ pEventDelegatePair)
{
	return m_delegate_map_proxy.get_proxy(this)->store_EventDelegatePair(pEventDelegatePair);
}

Value* DotNetClass::GetEventHandlers(Value* eventName) 
{ 
	dotNetClass *l_p_dotNetClass = m_delegate_map_proxy.get_proxy(this)->get_dotNetClass();
	return l_p_dotNetClass->get_event_handlers(eventName);
}

void DotNetClass::delegate_proxy_type::set_type(System::Type ^ type)
{
	try
	{
		m_pType = type;
		m_pWrappedClasses->Add(m_pType, this);
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(_M("dotNet runtime exception: "), MNETStr::ToMSTR(e));
	}
}

MSTR DotNetClass::GetTypeName() 
{
	try
	{
		MSTR name;
		System::Type ^ pType = GetType();
		if (pType) 
		{
			System::String ^ typeName = pType->FullName;
			if (!typeName)
				typeName = pType->Name;
			name = MNETStr::ToMSTR(typeName);
		}
		return name;
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(_M("dotNet runtime exception: "), MNETStr::ToMSTR(e));
	}
}

Value* DotNetClass::delegate_proxy_type::get_MXS_container_for_type(System::Type ^ type)
{
	delegate_proxy_type ^ theProxy = safe_cast<delegate_proxy_type ^>(m_pWrappedClasses[type]);
	if (theProxy)
		return theProxy->get_dotNetClass();
	return NULL;
}

void DotNetClass::delegate_proxy_type::detach()
{
	using namespace System;
	using namespace System::Reflection;

	try
	{
		m_p_native_target = NULL; 
		if (m_pType)
		{
			// unhook all the event handlers
			if (m_pEventDelegatePairs)
			{
				System::Collections::IEnumerator^ myEnum = m_pEventDelegatePairs->GetEnumerator();
				while ( myEnum->MoveNext() )
				{
					event_delegate_pair^ pEventDelegatePair = safe_cast<event_delegate_pair^>(myEnum->Current);
					EventInfo ^pEventInfo = m_pType->GetEvent(pEventDelegatePair->m_pEventName);
					if (pEventInfo)
						pEventInfo->RemoveEventHandler(nullptr, pEventDelegatePair->m_pEventDelegate);
				}
				m_pEventDelegatePairs = nullptr;
			}
			m_pWrappedClasses->Remove(m_pType);
			m_pType = nullptr;
		}
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(_M("dotNet runtime exception: "), MNETStr::ToMSTR(e));
	}
}

//  returns dotNetClass if object already wrapped
Value* DotNetClass::GetMXSContainerForObject(System::Object ^ object)
{
	System::Type ^ asType = dynamic_cast<System::Type^>(object);
	if (asType)
		return delegate_proxy_type::get_MXS_container_for_type(asType);
	return NULL;
}

/* -------------------- dotNetClass  ------------------- */
// 
#pragma unmanaged

#pragma warning(disable:4835)
visible_class_instance(MXS_dotNet::dotNetClass, _M("dotNetClass"));
#pragma warning(default:4835)

Value*
dotNetClassClass::apply(Value** arg_list, int count, CallContext* cc)
{
	// dotNetClass {<dotNetObject> | <dotNetControl> | <class_type_string>}
	check_arg_count(dotNetClass, 1, count);
	two_value_locals(val, result);
	vl.val = arg_list[0]->eval();
	if (is_dotNetObject(vl.val) || is_dotNetControl(vl.val))
	{
		DotNetObjectWrapper *wrapper = NULL;
		if (is_dotNetObject(vl.val)) wrapper = ((dotNetObject*)vl.val)->GetDotNetObjectWrapper();
		else wrapper = ((dotNetControl*)vl.val)->GetDotNetObjectWrapper();
		if (wrapper)
			vl.result = DotNetClass::Create(wrapper);
	}
	else
	{
		MCHAR *type = vl.val->to_string();
		vl.result = DotNetClass::Create(type);
	}
	return_value(vl.result);
}

dotNetClass::dotNetClass() : m_pDotNetClass(NULL)
{
	tag = class_tag(dotNetClass); 
}

dotNetClass::~dotNetClass()
{
	if (m_pDotNetClass)
		delete m_pDotNetClass;
}

void
dotNetClass::sprin1(CharStream* s)
{
	if (m_typeName.isNull() && m_pDotNetClass)
		m_typeName = m_pDotNetClass->GetTypeName();
	s->printf(_M("dotNetClass:%s"), (const MCHAR*) m_typeName);
}


