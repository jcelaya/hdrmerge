#include "../RawExposureStack.hpp"
using namespace hdrmerge;

int main(int argc, char * argv[]) {
    RawExposureStack::Exposure e1(argv[1]);
    RawExposureStack::LoadResult result = e1.load(nullptr);
    if (result == RawExposureStack::LOAD_SUCCESS) {
        e1.dumpInfo();
        RawExposureStack::Exposure e2(argv[2]);
        result = e2.load(&e1);
        if (result == RawExposureStack::LOAD_SUCCESS) {
            e2.dumpInfo();
        }
    }
    return result;
}
