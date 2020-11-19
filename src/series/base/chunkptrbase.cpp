#include "chunkptrbase.h"

#include "series/base/chunkbase.h"

namespace series {

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
