/**********************************************************************
 *<
	FILE:			ParticleChannelFloat.h

	DESCRIPTION:	ParticleChannelFloat channel interface
					This generic channel is used to store floating point numbers

	CREATED BY:		Chung-An Lin

	HISTORY:		created 01-24-02

 *>	Copyright (c) 2001, All Rights Reserved.
 **********************************************************************/

#ifndef _PARTICLECHANNELFLOAT_H_
#define _PARTICLECHANNELFLOAT_H_

#include "IParticleChannel.h"
#include "IParticleChannelAmount.h"
#include "IParticleChannelFloat.h"

namespace PFChannels {

class ParticleChannelFloat:		public IObject, 
								public IParticleChannel,
								public IParticleChannelAmountR,
								public IParticleChannelAmountW,
								public IParticleChannelFloatR,
								public IParticleChannelFloatW
{
public:

	ParticleChannelFloat();
	~ParticleChannelFloat();

	// from IObject interface
	TCHAR* GetIObjectName();
	int NumInterfaces() const { return 5; }
	BaseInterface* GetInterfaceAt(int index) const;
	BaseInterface* GetInterface(Interface_ID id);
	void AcquireIObject();
	void ReleaseIObject();
	void DeleteIObject();

	// from FPMixinInterface
	FPInterfaceDesc* GetDescByID(Interface_ID id);

	// from IParticleChannel
	Class_ID GetClassID() const;
	IObject*  Clone() const;
	IOResult Save(ISave* isave) const;
	IOResult Load(ILoad* iload);
	int MemoryUsed() const;

	// from IParticleChannelAmountR
	int Count() const;

	// from IParticleChannelAmountW
	void	ZeroCount();
	bool	SetCount(int n);
	int	Delete(int start,int num);
	int	Delete(BitArray& toRemove);
	IObject*	Split(BitArray& toSplit);
	bool	Spawn( Tab<int>& spawnTable );
	bool	AppendNum(int num);
	bool	Append(IObject* channel);

	// from IParticleChannelFloatR
	float	GetValue(int index) const;
	bool	IsGlobal() const;
	float	GetValue() const;

	// from IParticleChannelFloatW
	void	SetValue(int index, float value);
	void	SetValue(float value);

private:
	// const access to class members
	float				data(int index)	const	{ return m_data[index]; }
	const Tab<float>&	data()			const	{ return m_data; }
	bool				isGlobal()		const	{ return m_isGlobal; }
	int					globalCount()	const	{ return m_globalCount; }
	float				globalValue()	const	{ return m_globalValue; }

	// access to class members
	float&			_data(int index)			{ return m_data[index]; }
	Tab<float>&		_data()						{ return m_data; }
	bool&			_isGlobal()					{ return m_isGlobal; }
	int&			_globalCount()				{ return m_globalCount; }
	float&			_globalValue()				{ return m_globalValue; }

protected:
	// data
	Tab<float>	m_data;

	bool		m_isGlobal;
	int			m_globalCount; // for global values
	float		m_globalValue;
};

} // end of namespace PFChannels

#endif // _PARTICLECHANNELFLOAT_H_
