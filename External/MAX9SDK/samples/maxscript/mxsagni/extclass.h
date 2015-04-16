/**********************************************************************
 *<
	FILE: ExtClass.h

	DESCRIPTION: MXSAgni extension classes

	CREATED BY: Ravi Karra, 1998

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _H_EXT_CLASS
#define _H_EXT_CLASS

#include "MXSAgni.h"
#include "MNBigMat.h"
#include "notetrck.h"
#include "shadgen.h"

class BigMatrixValue;

visible_class_debug_ok (BigMatrixRowArray)

class BigMatrixRowArray : public Value
{
public:
	BigMatrixValue		*bmv;
	int					row;
						BigMatrixRowArray(BigMatrixValue* bmv);

						classof_methods (BigMatrixRowArray, Value);
	void				collect() { delete this; }
	void				sprin1(CharStream* s);
	void				gc_trace();
	
#include "defimpfn.h"
	def_generic	( get,		"get" );
	def_generic	( put,		"put" );
};


applyable_class_debug_ok (BigMatrixValue)

class BigMatrixValue : public Value
{
public:
	BigMatrix			bm;
	BigMatrixRowArray	*rowArray;

						BigMatrixValue(int mm, int nn);
						BigMatrixValue(const BigMatrix& from);

//	ValueMetaClass*		local_base_class() { return class_tag(BigMatrixValue); }			
						classof_methods (BigMatrixValue, Value);
	
	void				collect() { delete this; }
	void				sprin1(CharStream* s);
	void				gc_trace();
#	define				is_bigmatrix(p) ((p)->tag == class_tag(BigMatrixValue))
	Value*				get_property(Value** arg_list, int count);
	Value*				set_property(Value** arg_list, int count);
	BigMatrix&			to_bigmatrix() { return bm; }

	/* operations */

#include "defimpfn.h"
#	include "bmatpro.h"
	
};


applyable_class (MAXNoteTrack)

class MAXNoteTrack : public MAXWrapper
{
	
public:
	DefNoteTrack*		note_track;
	TCHAR*				name;
	MAXNoteTrack(TCHAR* iname);
	MAXNoteTrack(DefNoteTrack* dnt);
	static ScripterExport Value* intern(DefNoteTrack* dnt);
	
						classof_methods (MAXNoteTrack, MAXWrapper);
	void				collect() { delete this; }
	void				sprin1(CharStream* s);
	void				gc_trace();
#	define				is_notetrack(p) ((p)->tag == class_tag(MAXNoteTrack))
	Value*				get_property(Value** arg_list, int count);
	Value*				set_property(Value** arg_list, int count);
	DefNoteTrack*		to_notetrack() { return note_track; }
	TCHAR*				class_name() { return _T("NoteTrack"); }
	
	def_property		( keys );
#include "defimpfn.h"
#	include	"notespro.h"	
	def_generic ( copy,		"copy");
};

visible_class (MAXNoteKeyArray)

class MAXNoteKeyArray : public Value
{
public:
	MAXNoteTrack*	note_track;		/* the note track							*/

	ScripterExport	MAXNoteKeyArray(DefNoteTrack* icont);
	ScripterExport	MAXNoteKeyArray(MAXNoteTrack* icont);

					classof_methods (MAXNoteKeyArray, Value);
	BOOL			_is_collection() { return 1; }
	void			collect() { delete this; }
	void			gc_trace();
	ScripterExport	void sprin1(CharStream* s);

	/* operations */
	
#include "defimpfn.h"
#	include "arraypro.h"
#	include	"notespro.h"	
	ScripterExport Value* map(node_map& m);

	/* built-in property accessors */

	def_property ( count );
};


visible_class (MAXNoteKey)

class MAXNoteKey : public Value
{
public:
	MAXNoteTrack*	note_track;		/* MAX-side note track						*/
	int				key_index;

	ScripterExport	MAXNoteKey (DefNoteTrack* nt, int ikey);
	ScripterExport	MAXNoteKey (MAXNoteTrack* nt, int ikey);

					classof_methods (MAXNoteKey, Value);
	void			collect() { delete this; }
	void			gc_trace();
	ScripterExport	void sprin1(CharStream* s);

	def_generic		( delete, "delete" );

	def_property ( time );
	def_property ( selected );
	def_property ( value );
};

/* ---------------------- MAXFilterKernel ----------------------- */

visible_class (MAXFilter)

class MAXFilter : public MAXWrapper
{
public:
	FilterKernel*	filter;			// the MAX-side FilterKernel

	ScripterExport MAXFilter(FilterKernel* ifilter);
	static ScripterExport Value* intern(FilterKernel* ifilter);

	BOOL		is_kind_of(ValueMetaClass* c) { return (c == class_tag(MAXFilter)) ? 1 : MAXWrapper::is_kind_of(c); }
	void		collect() { delete this; }
	ScripterExport void		sprin1(CharStream* s);
	TCHAR*		class_name() { return _T("Filter"); }

	def_property	( name );

	FilterKernel*	to_filter() { check_for_deletion(); return filter; }
};


#endif //_H_EXT_CLASS
