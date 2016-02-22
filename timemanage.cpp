#include "timemanage.h"

TimeManager::TimeManager()
{
	m_fTimeRemaining = 300.0;
	m_fCurrentTimeControl = 300.0;
	m_nMoveNumber = 0;
}

double TimeManager::GetTimeForMove()
{
	return m_fTimeRemaining / 15.0;	// to-do: Obviously, this needs more work :-)
}

