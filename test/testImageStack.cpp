#include "../ImageStack.hpp"
using namespace hdrmerge;

int main(int argc, char * argv[]) {
    Image e1(argv[1]);
    e1.dumpInfo();
    Image::LoadResult result = e1.load(nullptr);
    if (result == Image::LOAD_SUCCESS) {
        e1.dumpInfo();
        Image e2(argv[2]);
        result = e2.load(&e1);
        if (result == Image::LOAD_SUCCESS) {
            e2.dumpInfo();
        }
    }
    return result;
}
