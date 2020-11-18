#include "dataseries.h"

namespace series {

thread_local ChunkPtrBase activeChunk = ChunkPtrBase::null();

ChunkPtrBase::ChunkPtrBase(ChunkBase *ptr)
   : target(ptr)
{
    assert(ptr);
    target->incRefs();
}

ChunkPtrBase::~ChunkPtrBase() {
   if (target) {
       target->decRefs();
   }
}

}
