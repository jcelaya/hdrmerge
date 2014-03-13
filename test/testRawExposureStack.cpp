#include "../RawExposureStack.hpp"
using namespace hdrmerge;

int main(int argc, char * argv[]) {
    RawExposureStack::Exposure e(argv[1]);
    RawExposureStack::LoadResult result = e.load(nullptr);
    if (result == RawExposureStack::LOAD_SUCCESS) {
        e.dumpInfo();
    }
    return result;
}
