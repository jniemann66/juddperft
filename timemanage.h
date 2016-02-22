#ifndef _TIMEMANAGE_H
#define _TIMEMANAGE_H 1

class TimeManager {
public:
	double m_fTimeRemaining;
	double m_fCurrentTimeControl;
	int m_nMoveNumber;
	TimeManager();
	double GetTimeForMove();
};

#define UNLIMITED_TIME -1

#endif // _TIMEMANAGE_H

