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
// DESCRIPTION: dotNetControl.cpp  : MAXScript dotNetControl code
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

/* -------------------- CDotNetHostWnd  ------------------- */
// 
#pragma managed

// first some MFC stuff...
BEGIN_MESSAGE_MAP(CDotNetHostWnd, CWnd)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


CDotNetHostWnd::CDotNetHostWnd(dotNetControl *p_dotNetControl) : inherited()
{
	m_delegate_map_proxy.get_proxy(this)->set_dotNetControl(p_dotNetControl);
}

CDotNetHostWnd::~CDotNetHostWnd()
{
	// Safety net -- if we delete the C++ object and there's still an HWND on it,
	// we need to call this so our own OnDestroy() is called.
	try
	{
		DestroyWindow(); 
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

BOOL CDotNetHostWnd::Create(HWND hParent, int id, int x, int y, int w, int h, 
						  MCHAR* pControlTypeString, Value **keyparms, int keyparm_count)
{
	// Create and add the control to the user control.
	using namespace System;
	using namespace System::Reflection;
	using namespace System::Reflection::Emit;

	try
	{
		// Create a dummy AFX class name (doesn't really have to be static here, MFC will
		// return the same class name for the same parameters, but it does make it explicit
		// this way)
		static LPCTSTR lpcszClassName = AfxRegisterWndClass(NULL);

		CWnd* pCWnd = CWnd::FromHandle(hParent);

		if( !inherited::Create(lpcszClassName, _T(""), WS_CHILD|WS_VISIBLE, CRect(x, y, x+w, y+h), 
			pCWnd, id) )
		{
			return FALSE;
		}

		System::Windows::Forms::Control ^l_pControl;

		// look for control type using fully qualified class name (includes assembly)
		Type ^controlType = ResolveTypeFromName(pControlTypeString);
		if (controlType)
		{
			ConstructorInfo ^ l_pConstructorInfo = controlType->GetConstructor(Type::EmptyTypes);
			if (l_pConstructorInfo)
				l_pControl = (System::Windows::Forms::Control^)l_pConstructorInfo->Invoke(Type::EmptyTypes);
		}

		if (l_pControl)
		{
			// hook up to the max message pump. Really needed?
			MAXScript_interface->RegisterDlgWnd(GetSafeHwnd());
			m_delegate_map_proxy.get_proxy(this)->set_control(l_pControl);
			CreateEventHandlerDelegates();
			CreateSetFocusEventHandler();

			Value* args[2];
			for (int i = 0; i < keyparm_count; i+=2)
			{
				args[0] = keyparms[i+1];
				args[1] = keyparms[i];
				set_property(args,2);
			}
			RECT rect; 
			rect.left = 0; rect.top = 0; rect.right = w; rect.bottom = h;
			BOOL res = m_CWinFormsControl.CreateManagedControl(
				l_pControl, 
				WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
				rect, 
				this, 
				id);

			if (res)
				CreateResizeEventHandler();
			return res;
		}

		return FALSE;
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

afx_msg void CDotNetHostWnd::OnDestroy()
{
	// Do any cleanup we need to here, if any.
	try
	{
		MAXScript_interface->UnRegisterDlgWnd(GetSafeHwnd());
		m_CWinFormsControl.DestroyWindow();
		CWnd::OnDestroy();
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

void CDotNetHostWnd::CreateEventHandlerDelegates()
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

// This method will be called when any event is raised on the user control
void CDotNetHostWnd::ProcessEvent(System::String ^eventName, System::Object ^delegateArgs)
{
	try
	{
		array<System::Object^> ^ delegateArgsArray = safe_cast<array<System::Object^> ^>(delegateArgs);
		System::Windows::Forms::Control ^ sender = safe_cast<System::Windows::Forms::Control ^>(delegateArgsArray[0]);
		System::EventArgs ^ eventArgs = safe_cast<System::EventArgs ^>(delegateArgsArray[1]);

		init_thread_locals();
		push_alloc_frame();
		Value**		arg_list;
		three_value_locals(eventName, handler, ehandler);
		vl.eventName = Name::intern(MNETStr::ToMSTR(eventName));
		dotNetControl *l_p_dotNetControl = m_delegate_map_proxy.get_proxy(this)->get_dotNetControl();
		vl.handler = l_p_dotNetControl->get_event_handler(vl.eventName);
		// see if an handler exists for this event
		if ( vl.handler != &undefined)
		{
			// make the event handler argument list
			int cArgs = 0;
			vl.ehandler = vl.handler->eval_no_wrapper();
			if (vl.ehandler->tag == class_tag(MAXScriptFunction))
				cArgs = ((MAXScriptFunction*)vl.ehandler)->parameter_count;
			value_local_array(arg_list, cArgs);
			for (int i = 0; i < cArgs; i++)
				arg_list[i] = &undefined;
			if (cArgs == 1)
				arg_list[0] = DotNetObjectWrapper::intern(eventArgs);
			if (cArgs > 1)
			{
				arg_list[0] = l_p_dotNetControl;
				arg_list[1] = DotNetObjectWrapper::intern(eventArgs);
			}
			// call the event handler
			l_p_dotNetControl->run_event_handler(l_p_dotNetControl->parent_rollout, vl.eventName, arg_list, cArgs);
			pop_value_local_array(arg_list);
		}
		pop_value_locals();
		pop_alloc_frame();
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

void CDotNetHostWnd::delegate_proxy_type::ControlSetFocusMethod(System::Object ^ sender, System::EventArgs ^e)
{
//	System::Diagnostics::Debug::WriteLine( "ControlSetFocusMethod" );
	DisableAccelerators();
}

void CDotNetHostWnd::delegate_proxy_type::ControlResizeMethod(System::Object ^ sender, System::EventArgs ^e)
{
	//	System::Diagnostics::Debug::WriteLine( "ControlResizeMethod" );
	if (!m_in_ResizeMethod && m_p_native_target && m_p_native_target->GetParent())
	{
		m_in_ResizeMethod = true;
		CWnd *hCWndParent = m_p_native_target->GetParent();
		CRect rect_host;
		m_p_native_target->GetWindowRect(&rect_host);
		hCWndParent->ScreenToClient(&rect_host);
		System::Drawing::Size control_size = m_pControl->Size;
		rect_host.bottom = rect_host.top + control_size.Height;
		rect_host.right = rect_host.left + control_size.Width;
		m_p_native_target->MoveWindow(rect_host);

		// hack alert! The HWND of the control appears take its size when the size of
		// the control changes. The HWNDs size is a combination of the control's size and
		// the parent's window's size. But we are changing the parent's window size here,
		// and we need to tell the control to update it's HWND size. The only way I've found
		// so far is to change and reset the size. We use the m_in_ResizeMethod variable as an
		// interlock so we don't go recursive. We also check m_in_ResizeMethod before calling 
		// scripted event handlers so that events fired from these resizings don't show.
		int width_orig = m_pControl->Width;
		m_pControl->Width = width_orig + 1;
		m_pControl->Width = width_orig;
		m_in_ResizeMethod = false;
	}
}

void CDotNetHostWnd::CreateSetFocusEventHandler()
{
	try
	{
		delegate_proxy_type ^l_pProxy = m_delegate_map_proxy.get_proxy(this);
		System::Windows::Forms::Control ^ l_pControl = l_pProxy->get_control();
		System::Type ^ l_pControlType = l_pControl->GetType();
		System::Reflection::EventInfo ^l_pGotFocusEvent = l_pControlType->GetEvent("GotFocus");
		if (l_pGotFocusEvent)
		{
			l_pProxy->m_pSetFocusHandler = gcnew System::EventHandler( l_pProxy, &delegate_proxy_type::ControlSetFocusMethod );
			l_pGotFocusEvent->AddEventHandler(l_pControl, l_pProxy->m_pSetFocusHandler);
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

void CDotNetHostWnd::CreateResizeEventHandler()
{
	try
	{
		delegate_proxy_type ^l_pProxy = m_delegate_map_proxy.get_proxy(this);
		System::Windows::Forms::Control ^ l_pControl = l_pProxy->get_control();
		System::Type ^ l_pControlType = l_pControl->GetType();
		System::Reflection::EventInfo ^l_pResizeEvent = l_pControlType->GetEvent("Resize");
		if (l_pResizeEvent)
		{
			l_pProxy->m_pResizeHandler = gcnew System::EventHandler( l_pProxy, &delegate_proxy_type::ControlResizeMethod );
			l_pResizeEvent->AddEventHandler(l_pControl, l_pProxy->m_pResizeHandler);
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

void CDotNetHostWnd::delegate_proxy_type::init()
{
	m_pWrappedControls = gcnew System::Collections::Hashtable(53);
}

void CDotNetHostWnd::Init()
{
	delegate_proxy_type::init();
	// force a load of the Windows::Form and System::Drawing Assemblies
	System::Reflection::Assembly ^windowsFormAssembly = System::Windows::Forms::MonthCalendar::typeid->Assembly;
	System::Reflection::Assembly ^drawingAssembly = System::Drawing::Color::typeid->Assembly;
}

System::Object ^ CDotNetHostWnd::GetObject()
{
	return m_delegate_map_proxy.get_proxy(this)->get_control();
}

Value* CDotNetHostWnd::GetMXSContainer()
{
	return m_delegate_map_proxy.get_proxy(this)->get_dotNetControl();
}

void CDotNetHostWnd::delegate_proxy_type::store_EventDelegatePair(event_delegate_pair ^ pEventDelegatePair)
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

void CDotNetHostWnd::StoreEventDelegatePair(event_delegate_pair ^ pEventDelegatePair)
{
	return m_delegate_map_proxy.get_proxy(this)->store_EventDelegatePair(pEventDelegatePair);
}

void CDotNetHostWnd::delegate_proxy_type::set_control(System::Windows::Forms::Control ^ control)
{
	try
	{
		m_pControl = control;
		m_pWrappedControls->Add(control, this);
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

Value* CDotNetHostWnd::delegate_proxy_type::get_MXS_container_for_object(System::Object ^ object)
{
	delegate_proxy_type ^ theProxy = safe_cast<delegate_proxy_type ^>(m_pWrappedControls[object]);
	if (theProxy)
		return theProxy->get_dotNetControl();
	return NULL;
}

void CDotNetHostWnd::delegate_proxy_type::detach() 
{ 
	using namespace System;
	using namespace System::Reflection;

	try
	{
		// HandleDestroyed event happens after detach. Do a fake one now before detaching.
		ProcessEvent("HandleDestroyed", gcnew array<System::Object^>{m_pControl, System::EventArgs::Empty});
		m_p_native_target = NULL; 
		if (m_pControl)
		{
			Type ^ targetType = m_pControl->GetType();

			// unhook all the event handlers
			if (m_pEventDelegatePairs)
			{
				System::Collections::IEnumerator^ myEnum = m_pEventDelegatePairs->GetEnumerator();
				while ( myEnum->MoveNext() )
				{
					event_delegate_pair^ pEventDelegatePair = safe_cast<event_delegate_pair^>(myEnum->Current);
					EventInfo ^l_pEvent = targetType->GetEvent(pEventDelegatePair->m_pEventName);
					if (l_pEvent)
						l_pEvent->RemoveEventHandler(m_pControl, pEventDelegatePair->m_pEventDelegate);
				}
				m_pEventDelegatePairs = nullptr;
			}
			// unhook GotFocus handler
			System::Reflection::EventInfo ^l_pGotFocusEvent = targetType->GetEvent("GotFocus");
			if (l_pGotFocusEvent && m_pSetFocusHandler)
				l_pGotFocusEvent->RemoveEventHandler(m_pControl, m_pSetFocusHandler);
			m_pSetFocusHandler = nullptr;

			// unhook Resize handler
			System::Reflection::EventInfo ^l_pResizeEvent = targetType->GetEvent("Resize");
			if (l_pResizeEvent && m_pResizeHandler)
				l_pResizeEvent->RemoveEventHandler(m_pControl, m_pResizeHandler);
			m_pResizeHandler = nullptr;

			m_pWrappedControls->Remove(m_pControl);
			m_pControl = nullptr;
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

void CDotNetHostWnd::delegate_proxy_type::ProcessEvent(System::String ^eventName, System::Object ^delegateArgs)
{
	// hack! If m_in_ResizeMethod is true, we are processing the ControlResizeMethod method, that 
	// does 2 resizes of the control in order for it to update to the new CWnd size. We don't want to
	// call the scripted event handlers due to events fired by those resizings....
	if (m_in_ResizeMethod)
		return;

//	System::Diagnostics::Debug::WriteLine( "Event: " + eventName );
	if (!m_p_native_target && eventName == "HandleDestroyed") 
		return; // happens after detach

//	MSTR xx = MXS_dotNet::MNETStr::ToMSTR(eventName);
//	xx.Append(_M("\n"));
//	CString xxx(xx);
//	OutputDebugString(xxx);

	DbgAssert(m_p_native_target);
	if(m_p_native_target)
		m_p_native_target->ProcessEvent(eventName, delegateArgs);
}


//  returns dotNetObject if object already wrapped
Value* CDotNetHostWnd::GetMXSContainerForObject(System::Object ^ object)
{
	return delegate_proxy_type::get_MXS_container_for_object(object);
}

BOOL CDotNetHostWnd::set_focus()
{
	System::Windows::Forms::Control ^ pControl = m_delegate_map_proxy.get_proxy(this)->get_control();
	if (pControl)
		return pControl->Focus();
	return FALSE;
}

/* -------------------- dotNetControl  ------------------- */
// 
#pragma unmanaged

#pragma warning(disable:4835)
visible_class_instance (MXS_dotNet::dotNetControl, "dotNetControl")
#pragma warning(default:4835)

dotNetControl::dotNetControl(Value* name, Value* caption, Value** keyparms, int keyparm_count)
: RolloutControl(name, caption, keyparms, keyparm_count), m_pDotNetHostWnd(NULL)
{
	// dotNetControl <name> <class_type_string> [{<property_name>:<value>}]
	tag = class_tag(dotNetControl); 
}

dotNetControl::~dotNetControl()
{
	if (m_pDotNetHostWnd)
		delete m_pDotNetHostWnd;
}

void
dotNetControl::gc_trace()
{
	RolloutControl::gc_trace();
}

void
dotNetControl::sprin1(CharStream* s)
{
	if (m_typeName.isNull() && m_pDotNetHostWnd)
		m_typeName = m_pDotNetHostWnd->GetTypeName();
	s->printf(_M("dotNetControl:%s:%s"), name->to_string(), m_typeName);
}

void
dotNetControl::add_control(Rollout *ro, HWND parent, HINSTANCE hInstance, int& current_y)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	caption = caption->eval();

	MCHAR *controlTypeName = caption->to_string();

	parent_rollout = ro;
	control_ID = next_id();

	// compute bounding box, apply layout params
	layout_data pos;
	compute_layout(ro, &pos, current_y);

	ASSERT( !m_pDotNetHostWnd );
	
	Value**  evaled_keyparms = NULL;
	value_temp_array(evaled_keyparms, keyparm_count*2);
	for (int i = 0; i < keyparm_count*2; i++)
		evaled_keyparms[i] = keyparms[i]->eval();

	try
	{
		m_pDotNetHostWnd = new CDotNetHostWnd(this);   // To host .NET controls
		BOOL res = m_pDotNetHostWnd->Create(parent, control_ID, pos.left, pos.top, pos.width, pos.height, 
											controlTypeName, evaled_keyparms, keyparm_count);
	}
	catch (...) 
	{
		pop_value_temp_array(evaled_keyparms);
		throw;
	}
	pop_value_temp_array(evaled_keyparms);
}

BOOL
dotNetControl::handle_message(Rollout *ro, UINT message, WPARAM wParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (message == WM_CLOSE && m_pDotNetHostWnd)
	{
		CDotNetHostWnd *l_pDotNetHostWnd = m_pDotNetHostWnd;
		m_pDotNetHostWnd->DestroyWindow();
		m_pDotNetHostWnd = NULL;
		delete l_pDotNetHostWnd;

	}
	return FALSE;
}

void
dotNetControl::set_enable()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (parent_rollout != NULL && parent_rollout->page != NULL && m_pDotNetHostWnd)
	{
		// set ActiveX control enable
		m_pDotNetHostWnd->EnableWindow(enabled);
		::InvalidateRect(parent_rollout->page, NULL, TRUE);
	}
}

BOOL
dotNetControl::set_focus()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (parent_rollout && parent_rollout->page && m_pDotNetHostWnd)
	{
		// set ActiveX control enable
		return m_pDotNetHostWnd->set_focus();
	}
	return FALSE;
}

Value*
dotNetControl::get_property(Value** arg_list, int count)
{
	// getProperty <dotNetControl> <prop name> [asDotNetObject:<bool>]
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	Value* prop = arg_list[0];
	Value* result;

	if (prop == n_enabled)
		return bool_result(enabled);
	else if (m_pDotNetHostWnd && ( result = m_pDotNetHostWnd->get_property(arg_list, count)))
		return_protected(result)
	else
	return RolloutControl::get_property(arg_list, count);
}

Value*
dotNetControl::set_property(Value** arg_list, int count)
{
	// setProperty <dotNetControl> <prop name> <value>
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Value* val = arg_list[0];
	// Value* prop = arg_list[1];
	one_value_local(result);
	if (m_pDotNetHostWnd && (vl.result = m_pDotNetHostWnd->set_property(arg_list, count)))
		return_value(vl.result)
	else
		return RolloutControl::set_property(arg_list, count);
}

Value*
dotNetControl::get_props_vf(Value** arg_list, int count)
{
	// getPropNames <dotNetControl> [showStaticOnly:<bool>] [declaredOnTypeOnly:<bool>] -- returns array of prop names of wrapped obj

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	bool showStaticOnly = key_arg_or_default(showStaticOnly,&false_value)->to_bool() != FALSE;
	bool declaredOnTypeOnly = key_arg_or_default(declaredOnTypeOnly,&false_value)->to_bool() != FALSE;

	one_typed_value_local(Array* result);
	vl.result = new Array (0);
	if (m_pDotNetHostWnd)
		m_pDotNetHostWnd->get_property_names(vl.result, showStaticOnly, declaredOnTypeOnly);
	return_value(vl.result);
}

Value*
dotNetControl::show_props_vf(Value** arg_list, int count)
{
	// showProperties <dotNetControl> ["prop_pat"] [to:<stream>] [showStaticOnly:<bool>] [showMethods:<bool>] [showAttributes:<bool>] [declaredOnTypeOnly:<bool>]
	CharStream*	out;
	MCHAR*		prop_pat = _M("*");

	if (!m_pDotNetHostWnd)
		return &false_value;

	if (count >= 1 && arg_list[0] != &keyarg_marker)	
		prop_pat = arg_list[0]->to_string();	

	if ((out = (CharStream*)key_arg(to)) != (CharStream*)&unsupplied)
	{
		if (out == (CharStream*)&undefined)   // to:undefined -> no output, return true or false if prop found
			out = NULL;
		else if (!is_charstream(out))
			throw TypeError (GetString(IDS_SHOWPROPERTIES_NEED_STREAM), out);
	}
	else
		out = thread_local(current_stdout);

	bool showStaticOnly = key_arg_or_default(showStaticOnly,&false_value)->to_bool() != FALSE;
	bool showMethods = key_arg_or_default(showMethods,&false_value)->to_bool() != FALSE;
	bool showAttributes = key_arg_or_default(showAttributes,&false_value)->to_bool() != FALSE;
	bool declaredOnTypeOnly = key_arg_or_default(declaredOnTypeOnly,&false_value)->to_bool() != FALSE;

	// look in properties, then fields
	bool res = m_pDotNetHostWnd->show_properties(prop_pat, out, showStaticOnly, showMethods, showAttributes, declaredOnTypeOnly);
	if (out || !res) // not searching for prop match, or prop match failed
		res |= m_pDotNetHostWnd->show_fields(prop_pat, out, showStaticOnly, showAttributes, declaredOnTypeOnly);
	return bool_result(res);
}

Value*
dotNetControl::show_methods_vf(Value** arg_list, int count)
{
	// showMethods <dotNetControl> ["prop_pat"] [to:<stream>] [showStaticOnly:<bool>] [showSpecial:<bool>] [showAttributes:<bool>] [declaredOnTypeOnly:<bool>]
	CharStream*	out;
	MCHAR*		prop_pat = _M("*");

	if (!m_pDotNetHostWnd)
		return &false_value;

	if (count >= 1 && arg_list[0] != &keyarg_marker)	
		prop_pat = arg_list[0]->to_string();	

	if ((out = (CharStream*)key_arg(to)) != (CharStream*)&unsupplied)
	{
		if (out == (CharStream*)&undefined)   // to:undefined -> no output, return true or false if prop found
			out = NULL;
		else if (!is_charstream(out))
			throw TypeError (GetString(IDS_SHOWMETHODS_NEED_STREAM), out);
	}
	else
		out = thread_local(current_stdout);

	bool showStaticOnly = key_arg_or_default(showStaticOnly,&false_value)->to_bool() != FALSE;
	bool showSpecial = key_arg_or_default(showSpecial,&false_value)->to_bool() != FALSE;
	bool showAttributes = key_arg_or_default(showAttributes,&false_value)->to_bool() != FALSE;
	bool declaredOnTypeOnly = key_arg_or_default(declaredOnTypeOnly,&false_value)->to_bool() != FALSE;

	bool res = m_pDotNetHostWnd->show_methods(prop_pat, out, showStaticOnly, showSpecial, showAttributes, declaredOnTypeOnly);
	return bool_result(res);
}

Value*
dotNetControl::show_events_vf(Value** arg_list, int count)
{
	// showEvents <dotNetControl> ["prop_pat"] [to:<stream>] [showStaticOnly:<bool>] [declaredOnTypeOnly:<bool>]
	CharStream*	out;
	MCHAR*		prop_pat = _M("*");

	if (!m_pDotNetHostWnd)
		return &false_value;

	if (count >= 1 && arg_list[0] != &keyarg_marker)	
		prop_pat = arg_list[0]->to_string();	

	if ((out = (CharStream*)key_arg(to)) != (CharStream*)&unsupplied)
	{
		if (out == (CharStream*)&undefined)   // to:undefined -> no output, return true or false if prop found
			out = NULL;
		else if (!is_charstream(out))
			throw TypeError (GetString(IDS_SHOWEVENTS_NEED_STREAM), out);
	}
	else
		out = thread_local(current_stdout);

	bool showStaticOnly = key_arg_or_default(showStaticOnly,&false_value)->to_bool() != FALSE;
	bool declaredOnTypeOnly = key_arg_or_default(declaredOnTypeOnly,&false_value)->to_bool() != FALSE;

	bool res = m_pDotNetHostWnd->show_events(prop_pat, out, showStaticOnly, declaredOnTypeOnly);
	return bool_result(res);
}

/*
// see comments in utils.cpp
Value*
dotNetControl::show_interfaces_vf(Value** arg_list, int count)
{
	// showInterfaces <dotNetControl> ["prop_pat"] [to:<stream>]
	CharStream*	out;
	MCHAR*		prop_pat = _M("*");

	if (!m_pDotNetHostWnd)
		return &false_value;

	if (count >= 1 && arg_list[0] != &keyarg_marker)	
		prop_pat = arg_list[0]->to_string();	

	if ((out = (CharStream*)key_arg(to)) != (CharStream*)&unsupplied)
	{
		if (out == (CharStream*)&undefined)   // to:undefined -> no output, return true or false if prop found
			out = NULL;
		else if (!is_charstream(out))
			throw TypeError (GetString(IDS_SHOWMETHODS_NEED_STREAM), out);
	}
	else
		out = thread_local(current_stdout);

	bool res = m_pDotNetHostWnd->show_interfaces(prop_pat, out);
	return bool_result(res);
}

Value*	
dotNetControl::get_interfaces_vf(Value** arg_list, int count)
{
	// getInterfaces <dotNetControl> ["prop_pat"] [to:<stream>]
	check_arg_count(getInterfaces, 1, count+1);
	one_typed_value_local(Array* result);
	vl.result = new Array (0);
	if (m_pDotNetHostWnd)
		m_pDotNetHostWnd->get_interfaces(vl.result);
	return_value(vl.result);
}
*/
