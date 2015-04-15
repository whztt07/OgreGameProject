

#ifndef __OgreSingleton_h__
#define __OgreSingleton_h__

#include "OgrePrerequisites.h"

namespace Ogre
{
	template <class T>
	class OgreExport CSingleton
	{
	public:
		typedef CSingleton<T> Base;

		CSingleton()
		{
			msInstance = static_cast<T*>(this);
		}

		virtual ~CSingleton()
		{
			msInstance = NULL;
		}

		static T& GetInstance()
		{
			return (*GetInstancePtr());
		}

		static T* GetInstancePtr()
		{
			return msInstance;
		}

		static const char* GetClassTypeName()
		{
			return mClassTypeName;
		}

	private:
		static T* msInstance;

	protected:
		static const char* mClassTypeName;
	};

}

#endif