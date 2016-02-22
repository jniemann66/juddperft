// engine.h
#ifndef _ENGINE_H
#define _ENGINE_H 1

#include "hashtable.h"
#include "movegen.h"
#include "search.h"
#include "timemanage.h"

class Engine{
public:
	Engine();
	bool isThinking;
	bool isPondering;
	bool PonderingActivated;
	ChessPosition CurrentPosition;
	unsigned int FullMove;
	bool StopSignal;
	unsigned int depth;
	bool ForceMode;
	bool ShowThinking;
	unsigned int nNumCores;
	TimeManager TM;
private:
};

#endif // _ENGINE_H
