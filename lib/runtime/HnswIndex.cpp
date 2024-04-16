#include "runtime/HnswIndex.h"

#include "execution/Execution.h"
#include "runtime/helpers.h"

#include <filesystem>

#include <arrow/api.h>
#include <arrow/array/array_primitive.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>

namespace runtime {

    void HnswIndex::addPoint(const float* point) {
    }

    void HnswIndex::build() {
        // todo: need parameters
    }
}