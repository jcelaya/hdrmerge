#include "../RawExposureStack.hpp"
using namespace hdrmerge;

int main(int argc, char * argv[]) {
    RawExposure e1(argv[1]);
    e1.dumpInfo();
    RawExposure::LoadResult result = e1.load(nullptr);
    if (result == RawExposure::LOAD_SUCCESS) {
        e1.dumpInfo();
        RawExposure e2(argv[2]);
        result = e2.load(&e1);
        if (result == RawExposure::LOAD_SUCCESS) {
            e2.dumpInfo();
        }
    }
    return result;
}
