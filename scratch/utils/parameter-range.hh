#ifndef PARAM_RANGE_H
#define PARAM_RANGE_H

#include <ostream>
#include <vector>
#include <sstream>
#include <cmath>
#include "string.h"

class ParameterRange
{
    public: 
        double min;
        double max;
        double step;

    ParameterRange(double min, double max, double step) :
        min(min),
        max(max),
        step(step)
        {}
};


#endif /* PARAM_RANGE_H */