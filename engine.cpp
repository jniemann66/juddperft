// engine.cpp

#include "engine.h"

Engine::Engine()
{
	Engine::isThinking = false;
	Engine::isPondering = false;
	Engine::PonderingActivated = false;
	Engine::StopSignal=false;
	Engine::depth=8;
	Engine::ForceMode=false;
	Engine::ShowThinking=true;
	Engine::nNumCores = MAX_THREADS;	// Hard maximum: 
										// App should only ever dispatch min(concurrency, nNumCores, MAX_THREADS) threads
}

