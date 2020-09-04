#pragma once

#include <my.h>
#include <KrkrClientProxyer.h>
#include "CoTaskBase.h"

class TaskAutoUnlocker
{
public:
	TaskAutoUnlocker() = delete;
	TaskAutoUnlocker(const TaskAutoUnlocker&) = delete;
	TaskAutoUnlocker(const CoTaskBase* Instance, KrkrClientProxyer* Proxyer) : 
		m_Instance(Instance),
		m_Proxyer(Proxyer)
	{
	};

	~TaskAutoUnlocker();


private:
	const CoTaskBase* m_Instance = nullptr;
	KrkrClientProxyer* m_Proxyer = nullptr;
};

