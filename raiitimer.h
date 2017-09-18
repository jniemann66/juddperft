/*
* Copyright (C) 2016 - 2017 Judd Niemann - All Rights Reserved
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef _RAIITIMER_H
#define _RAIITIMER_H 1

#include <iostream>
#include <chrono>

class RaiiTimer {
public:
    RaiiTimer() {
        beginTimer = std::chrono::high_resolution_clock::now();    
    }
    ~RaiiTimer() {
        endTimer = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTimer - beginTimer).count();
        std::cout << "Time=" << duration << " ms" << std::endl;
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> beginTimer;
    std::chrono::time_point<std::chrono::high_resolution_clock> endTimer;
};

#endif // _RAIITIMER_H