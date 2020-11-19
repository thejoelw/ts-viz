#include "chunk.h"

namespace series {

thread_local ChunkPtrBase activeChunk = ChunkPtrBase::null();

}
