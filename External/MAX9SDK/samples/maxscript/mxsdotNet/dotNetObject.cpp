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
// DESCRIPTION: dotNetObject.cpp  : MAXScript dotNetObject code
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

/* -------------------- DotNetObject  ------------------- */
// 
#pragma managed

DotNetObject::DotNetObject(dotNetObject* p_dotNetObject)
{
	m_delegate_map_proxy.get_proxy(this)->set_dotNetObject(p_dotNetObject);
}

DotNetObject::DotNetObject(System::Object ^ pObject, dotNetObject* p_dotNetObject)
{
	m_delegate_map_proxy.get_proxy(this)->set_object(pObject);
	m_delegate_map_proxy.get_proxy(this)->set_dotNetObject(p_dotNetObject);
	if (pObject)
		CreateEventHandlerDelegates();
}

DotNetObject::~DotNetObject()
{
}

Value* DotNetObject::Create(MCHAR* pTypeString, dotNetClass* p_dotNetClass, Value** arg_list, int count)
{
	using namespace System::Reflection;
	using namespace System::Collections::Generic;

	try
	{

		Value**	evald_args;
		Value**	thunk_args;
		Value	**ap, **eap, **tap;
		int		i;

		// stack allocate space for temp eval'd args, run through evaling them all 
		value_local_array(evald_args, count);
		value_local_array(thunk_args, count);
		for (i = count, ap = arg_list, eap = evald_args, tap = thunk_args; i > 0 ; eap++, ap++, tap++, i--)
		{
			*eap = (*ap)->eval();
			if (is_thunk(*eap))
			{
				*tap = *eap;
				*eap = (*eap)->eval();
			}
		}

		// Create the user object.
		System::Type ^l_pType;
		if (p_dotNetClass)
			l_pType = p_dotNetClass->GetDotNetObjectWrapper()->GetType();
		else if (pTypeString)
			l_pType = ResolveTypeFromName(pTypeString);
		if (l_pType)
		{
			ConstructorInfo ^ l_pConstructorInfo;
			array<System::Object^> ^argArray;
			int arg_count = count_with_keys();
			if (count != 0)
			{
				array<ConstructorInfo^> ^cInfos = l_pType->GetConstructors();

				// if no constructors, we may have a basic data type. Try going through 
				// MXS_Value_To_Object
				if (cInfos->Length == 0 && count == 1)
				{
					System::Object ^l_pObject = MXS_dotNet::MXS_Value_To_Object(evald_args[0],l_pType);
					pop_value_local_array(thunk_args);
					pop_value_local_array(evald_args);
					return intern(l_pObject);
				}

				// collect the constructors that have the right number of parameters
				List<ConstructorInfo^> ^candidateCInfos = gcnew List<ConstructorInfo^>();
				List<array<ParameterInfo^>^> ^candidateParamLists = gcnew List<array<ParameterInfo^>^>;

				for(int c=0; c<cInfos->Length; c++)
				{
					ConstructorInfo ^ cInfo = cInfos[c];
					array<ParameterInfo^> ^params = cInfo->GetParameters();
					if (params->Length == arg_count)
					{
						candidateCInfos->Add(cInfo);
						candidateParamLists->Add(params);
					}
				}
				// Find the best match between supplied args and the parameters, convert args to parameter types
				Value* userSpecifiedParamTypes = _get_key_arg(evald_args, count, n_paramTypes);
				Array* userSpecifiedParamTypeArray = NULL;
				if (userSpecifiedParamTypes != &unsupplied)
				{
					if (is_array(userSpecifiedParamTypes))
					{
						userSpecifiedParamTypeArray = (Array*)userSpecifiedParamTypes;
						// make sure correct size. Save check to see if elements are Type wrappers for later
						if (userSpecifiedParamTypeArray->size != arg_count)
							throw RuntimeError(_M("Incorrect paramTypes array size"));
					}
					else
						throw ConversionError(userSpecifiedParamTypes, _M("Array"));

				}

				argArray = gcnew array<System::Object^>(arg_count);
				int bestMatch = MXS_dotNet::FindMethodAndPrepArgList(candidateParamLists, evald_args, 
					argArray, arg_count, userSpecifiedParamTypeArray);
				if (bestMatch >= 0)
					l_pConstructorInfo = candidateCInfos[bestMatch];
			}
			else
			{
				l_pConstructorInfo = l_pType->GetConstructor(System::Type::EmptyTypes);
				argArray = System::Type::EmptyTypes;
			}

			System::Object ^ l_pObject;
			if (l_pConstructorInfo)
				l_pObject = l_pConstructorInfo->Invoke(argArray);
			else
				throw RuntimeError(_M("No constructor found which matched argument list: "), pTypeString);
			if (l_pObject)
			{
				// write back to byRef args
				array<ParameterInfo^> ^params = l_pConstructorInfo->GetParameters();
				for (int i = 0; i < params->Length; i++)
				{
					ParameterInfo ^ l_pParam = params[i];
					if (l_pParam->ParameterType->IsByRef && !l_pParam->IsIn && thunk_args[i])
					{
						Value *origValue =  thunk_args[i]->eval();
						if (is_dotNetObject(origValue) || is_dotNetControl(origValue))
							((Thunk*)(thunk_args[i]))->assign(DotNetObjectWrapper::intern(argArray[i]));
						else
							((Thunk*)(thunk_args[i]))->assign(Object_To_MXS_Value(argArray[i]));
					}
				}
				pop_value_local_array(thunk_args);
				pop_value_local_array(evald_args);
				return intern(l_pObject);
			}
			else
				throw RuntimeError(_M("Construction of object failed: "), pTypeString);
		}
		else
			throw RuntimeError(_M("Cannot resolve type: "), pTypeString);
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

void DotNetObject::CreateEventHandlerDelegates()
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

void DotNetObject::delegate_proxy_type::init()
{
	m_pWrappedObjects = gcnew System::Collections::Hashtable(53);
}

void DotNetObject::Init()
{
	delegate_proxy_type::init();
}

System::Object ^ DotNetObject::GetObject()
{
	return m_delegate_map_proxy.get_proxy(this)->get_object();
}

Value* DotNetObject::GetMXSContainer()
{
	return m_delegate_map_proxy.get_proxy(this)->get_dotNetObject();
}

void DotNetObject::delegate_proxy_type::store_EventDelegatePair(event_delegate_pair ^ pEventDelegatePair)
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

void DotNetObject::StoreEventDelegatePair(event_delegate_pair ^ pEventDelegatePair)
{
	return m_delegate_map_proxy.get_proxy(this)->store_EventDelegatePair(pEventDelegatePair);
}

Value* DotNetObject::GetEventHandlers(Value* eventName) 
{ 
	dotNetObject *l_p_dotNetObject = m_delegate_map_proxy.get_proxy(this)->get_dotNetObject();
	return l_p_dotNetObject->get_event_handlers(eventName);
}

void DotNetObject::delegate_proxy_type::set_object(System::Object ^ object)
{
	try
	{
		m_pObject = object;
		// we can't cache structure instances. Hashcode is same for 2 different
		// instances with the same values. 
		if (object)
		{
			System::Type ^ type = object->GetType();
			if (!type->IsValueType || type->IsPrimitive || type->IsEnum)
				m_pWrappedObjects->Add(object, this);
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

Value* DotNetObject::delegate_proxy_type::get_MXS_container_for_object(System::Object ^ object)
{
	delegate_proxy_type ^ theProxy = safe_cast<delegate_proxy_type ^>(m_pWrappedObjects[object]);
	if (theProxy)
		return theProxy->get_dotNetObject();
	return NULL;
}

void DotNetObject::delegate_proxy_type::detach()
{
	using namespace System;
	using namespace System::Reflection;

	try
	{
		m_p_native_target = NULL; 
		if (m_pObject)
		{
			// unhook all the event handlers
			if (m_pEventDelegatePairs)
			{
				Type ^ targetType = m_pObject->GetType();
				System::Collections::IEnumerator^ myEnum = m_pEventDelegatePairs->GetEnumerator();
				while ( myEnum->MoveNext() )
				{
					event_delegate_pair^ pEventDelegatePair = safe_cast<event_delegate_pair^>(myEnum->Current);
					EventInfo ^pEventInfo = targetType->GetEvent(pEventDelegatePair->m_pEventName);
					if (pEventInfo)
						pEventInfo->RemoveEventHandler(m_pObject, pEventDelegatePair->m_pEventDelegate);
				}
				m_pEventDelegatePairs = nullptr;
			}
			m_pWrappedObjects->Remove(m_pObject);
			m_pObject = nullptr;
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

void DotNetObject::delegate_proxy_type::ProcessEvent(System::String ^eventName, System::Object ^delegateArgs)
{
	DbgAssert(m_p_native_target);
	if(m_p_native_target)
		m_p_native_target->ProcessEvent(eventName, delegateArgs);
}

//  returns dotNetObject if object already wrapped
Value* DotNetObject::GetMXSContainerForObject(System::Object ^ object)
{
	return delegate_proxy_type::get_MXS_container_for_object(object);
}

//  returns dotNetObject if object already wrapped
bool DotNetObject::Compare(DotNetObject * pCompareVal)
{
	System::Object ^ l_pObject = GetObject();
	System::Object ^ l_pCompareObject;
	if (pCompareVal)
		l_pCompareObject = pCompareVal->GetObject();

	if (!l_pObject || !l_pCompareObject)
		return false;
	if (l_pObject == l_pCompareObject)
		return true;
	return l_pObject->Equals(l_pCompareObject);
}

//  returns dotNetObject containing a copy of the wrapped object 
Value* DotNetObject::Copy()
{
	using namespace System;
	using namespace System::Reflection;

	try
	{
		System::Object ^ l_pObject = GetObject();
		System::ICloneable ^ l_pICloneable = dynamic_cast<System::ICloneable^>(l_pObject);
		if (l_pICloneable) 
			return intern(l_pICloneable->Clone());
		// see if a structure. 
		System::Type ^ l_pType = GetType();
		if (l_pType->IsValueType && !l_pType->IsPrimitive)
		{
			System::Object ^ l_pNewObject;
			try
			{
				TypeDelegator ^ td = gcnew TypeDelegator(l_pType);
				BindingFlags flags = (BindingFlags)( BindingFlags::FlattenHierarchy | BindingFlags::Public | BindingFlags::Instance | BindingFlags::InvokeMethod);
				l_pNewObject = td->InvokeMember( "MemberwiseClone", flags, nullptr, l_pObject, nullptr );
			}
			catch (System::Exception ^) { }
			if (l_pNewObject)
				return intern(l_pNewObject);
		}
		return GetMXSContainer();
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

/* -------------------- dotNetObject  ------------------- */
// 
#pragma unmanaged

#pragma warning(disable:4835)
visible_class_instance(MXS_dotNet::dotNetObject, _M("dotNetObject"));
#pragma warning(default:4835)

Value*
dotNetObjectClass::apply(Value** arg_list, int count, CallContext* cc)
{
	// dotNetObject { <dotNetClass> | <class_type_string> } ... additional args as needed ...
	if (count == 0)
		check_arg_count(dotNetObject, 1, count);
	two_value_locals(arg, result);
	vl.arg = arg_list[0]->eval();
	if (is_dotNetClass(vl.arg))
	{
		vl.result = DotNetObject::Create(NULL, (dotNetClass*)vl.arg, &arg_list[1], count-1);
	}
	else
	{
		MCHAR *type = vl.arg->to_string();
		vl.result = DotNetObject::Create(type, NULL, &arg_list[1], count-1);
	}
	return_value(vl.result);
}

dotNetObject::dotNetObject() : m_pDotNetObject(NULL)
{
	tag = class_tag(dotNetObject); 
}

dotNetObject::~dotNetObject()
{
	if (m_pDotNetObject)
		delete m_pDotNetObject;
}

void
dotNetObject::sprin1(CharStream* s)
{
	if (m_typeName.isNull() && m_pDotNetObject)
		m_typeName = m_pDotNetObject->GetTypeName();
	s->printf(_M("dotNetObject:%s"), m_typeName);
}

Value*
dotNetObject::eq_vf(Value** arg_list, int count)
{
	// dotNetObject operator == 
	Value* cmpnd = arg_list[0];
	if (cmpnd == this)
		return &true_value;
	if (m_pDotNetObject && comparable(cmpnd))
	{
		dotNetObject *l_p_dotNetObject = (dotNetObject*)cmpnd;
		return bool_result(m_pDotNetObject->Compare(l_p_dotNetObject->m_pDotNetObject));
	}
	else
		return &false_value;
}

Value*
dotNetObject::ne_vf(Value** arg_list, int count)
{
	// dotNetObject operator != 
	Value* cmpnd = arg_list[0];
	if (cmpnd == this)
		return &false_value;
	if (m_pDotNetObject && comparable(cmpnd))
	{
		dotNetObject *l_p_dotNetObject = (dotNetObject*)cmpnd;
		return bool_result(!m_pDotNetObject->Compare(l_p_dotNetObject->m_pDotNetObject));
	}
	else
		return &true_value;
}

Value*
dotNetObject::copy_vf(Value** arg_list, int count) 
{ 
	if (m_pDotNetObject)
		return m_pDotNetObject->Copy();
	return this;
}
