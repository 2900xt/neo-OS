#include <math.h>

double fabs(double x)
{
    return (x < 0 ? -x : x);
}


const static double epsilon = 0.00001;

double sqrt(double x)
{
    double result = x;

    while(fabs(result * result - x) > epsilon)
    {
        result = (result + x / result) / 2.0;
    }

    return result;
}

double pow(double base, int exp)
{
    double result = base;
    for(int i = 0; i < exp; i++)
    {
        result *= base;
    }
    return result;
}


uint64_t min(uint64_t a, uint64_t b)
{
    return (a > b ? b : a);
}

uint64_t max(uint64_t a, uint64_t b)
{
    return (a > b ? a : b);
}