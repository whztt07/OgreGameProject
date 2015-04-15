#include "MaxInterface.h"

static CMaxInterface g_MaxInterface;
CMaxInterface* GetMaxIP()
{
	return &g_MaxInterface;
}

CMaxInterface::CMaxInterface(void)
{
}

CMaxInterface::~CMaxInterface(void)
{
}

void CMaxInterface::Create(ExpInterface *pExpInterface, Interface *pInterface)
{
	m_pExpInterface = pExpInterface;
	m_pInterface = pInterface;
}

Interface* CMaxInterface::GetIP()
{
	return m_pInterface;
}

INode* CMaxInterface::GetRootNode()
{
	return m_pInterface->GetRootNode();
}