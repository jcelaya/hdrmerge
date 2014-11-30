/*
 *  HDRMerge - HDR exposure merging software.
 *  Copyright 2012 Javier Celaya
 *  jcelaya@gmail.com
 *
 *  This file is part of HDRMerge.
 *
 *  HDRMerge is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  HDRMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with HDRMerge. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _LOG_HPP_
#define _LOG_HPP_

#include <ostream>
#include <string>
#include <chrono>
#include <QString>

namespace hdrmerge {


inline std::ostream & operator<<(std::ostream & os, const QString & s) {
    return os << std::string(s.toLocal8Bit().constData());
}


class Log {
public:
    enum {
        DEBUG = 0,
        PROGRESS = 1,
    } Priority;

    template <typename... Args>
    static void msg(int priority, const Args &... params) {
        Log & l = getInstance();
        if (l.out && priority >= l.minPriority) {
            l.output(params...);
            *l.out << std::endl;
        }
    }

    template <typename... Args>
    static void msgN(int priority, const Args &... params) {
        Log & l = getInstance();
        if (l.out && priority >= l.minPriority) {
            l.output(params...);
        }
    }

    template <typename... Args>
    static void debug(const Args &... params) {
        msg(DEBUG, params...);
    }

    template <typename... Args>
    static void debugN(const Args &... params) {
        msgN(DEBUG, params...);
    }

    template <typename... Args>
    static void progress(const Args &... params) {
        msg(PROGRESS, params...);
    }

    template <typename... Args>
    static void progressN(const Args &... params) {
        msgN(PROGRESS, params...);
    }

    static void setMinimumPriority(int p) {
        getInstance().minPriority = p;
    }

    static void setOutputStream(std::ostream & o) {
        getInstance().out = &o;
    }

private:
    int minPriority;
    std::ostream * out;

    Log() : minPriority(2), out(nullptr) {}

    static Log & getInstance() {
        static Log instance;
        return instance;
    }

    void output() {}

    template<typename T, typename... Args>
    void output(const T & value, const Args &... args) {
        *out << value;
        output(args...);
    }
};



class Timer {
public:
    Timer(const char * n) : name(n) {
        start = std::chrono::steady_clock::now();
    }
    ~Timer() {
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        double t = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
        Log::debug(name, ": ", t, " seconds");
    }

private:
    std::chrono::steady_clock::time_point start;
    const char * name;
};


template <typename Func> auto measureTime(const char * name, Func f) -> decltype(f()) {
    Timer t(name);
    return f();
}

} // namespace hdrmerge

#endif // _LOG_HPP_
