//=======================================================================
// Copyright Baptiste Wicht 2013-2014.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef STOP_WATCH_HPP
#define STOP_WATCH_HPP

#include <chrono>

typedef std::chrono::high_resolution_clock clock_type;

template<typename precision = std::chrono::milliseconds>
class stop_watch {
public:
    stop_watch(){
        start_point = clock_type::now();
    }

    double elapsed(){
        auto end_point = clock_type::now();
        auto time = std::chrono::duration_cast<precision>(end_point - start_point);
        return time.count();
    }

private:
    clock_type::time_point start_point;
};

template<typename precision = std::chrono::milliseconds>
class auto_stop_watch {
public:
    auto_stop_watch(std::string title) : title(title) {
        //Empty
    }

    ~auto_stop_watch(){
        std::cout << title << " took " << watch.elapsed() << std::endl;
    }

private:
    std::string title;
    stop_watch<precision> watch;
};


#endif
