#include <router.hpp>

#include <iostream>
#include <string>

int main() {
    Router r;
    Request rq;

    auto res = r.dispatch(rq);

    std::cout << res.serialize() << std::endl;

    return 0;
}