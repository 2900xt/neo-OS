#ifndef PAIR_H
#define PAIR_H

namespace stdlib {
template <typename T1, typename T2>
struct pair {
    T1 first;
    T2 second;

    pair() : first(), second() {}
    pair(const T1& f, const T2& s) : first(f), second(s) {}
};
}
#endif // PAIR_H