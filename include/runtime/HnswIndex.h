#ifndef RUNTIME_HNSWINDEX_H
#define RUNTIME_HNSWINDEX_H
#include "Index.h"
#include "runtime/Buffer.h"
#include "runtime/RecordBatchInfo.h"
#include <arrow/type_fwd.h>
namespace runtime {
    class HnswIndex : public Index {
        void addPoint(const float* point);
        void build();

    public:
        // todo: ctor and dtor
        void ensureLoaded() override;
        void appendRows(std::shared_ptr<arrow::Table> table) override;
        void setPersist(bool value) override;
    };
}
#endif //RUNTIME_HNSWINDEX_H