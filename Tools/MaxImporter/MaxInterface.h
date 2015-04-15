#pragma once

#include "max.h"
#include "cs/bipexp.h"
#include "cs/phyexp.h"
#include "iparamb2.h"
#include "iskin.h"
#include "stdmat.h"
#include <string>
#include <vector>

enum
{
	MODIFIER_NONE,
	MODIFIER_SKIN,
	MODIFIER_PHYSIQUE
};

class CMaxInterface
{
public:
	CMaxInterface(void);
	~CMaxInterface(void);

	void Create(ExpInterface *pExpInterface, Interface *pInterface);
	Interface* GetIP();
	INode* GetRootNode();
protected:
	ExpInterface *m_pExpInterface;
	Interface *m_pInterface;
};

CMaxInterface* GetMaxIP();