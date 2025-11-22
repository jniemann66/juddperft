/*
* Copyright (C) 2016 - 2025 Judd Niemann - All Rights Reserved
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef _RAIITIMER_H
#define _RAIITIMER_H 1

#include <iomanip>
#include <iostream>
#include <chrono>
#include <sstream>

class RaiiTimer {

public:
    RaiiTimer()
    {
		beginTimer = std::chrono::high_resolution_clock::now();
	}

    ~RaiiTimer()
    {
		endTimer = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTimer - beginTimer).count();
        std::cout << "Duration=";
        std::cout << duration << "ms (" << format_duration(std::chrono::milliseconds(duration)) << ")" << std::endl;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> beginTimer;
	std::chrono::time_point<std::chrono::high_resolution_clock> endTimer;

    std::string format_duration(std::chrono::milliseconds ms)
    {
        using namespace std::chrono;
        auto ss = duration_cast<seconds>(ms);
        ms -= duration_cast<milliseconds>(ss);
        auto mm = duration_cast<minutes>(ss);
        ss -= duration_cast<seconds>(mm);
        auto hh = duration_cast<hours>(mm);
        mm -= duration_cast<minutes>(hh);

        std::stringstream stream;
        stream << std::setfill('0')
               << std::setw(2) << hh.count() << ":"
               << std::setw(2) << mm.count() << ":"
               << std::setw(2) << ss.count() << "."
               << std::setw(3) << ms.count();
        return stream.str();
    }
};

#endif // _RAIITIMER_H
