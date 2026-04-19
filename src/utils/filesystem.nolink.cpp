#include "filesystem.h"

void BlobOrFileCtors()
{
    using namespace em;
    using namespace em::Filesystem;

    blob_or_file a = zblob_or_file{};
    blob_or_file b(zblob_or_file{});
    blob_or_file c = blob{};
    blob_or_file d(blob{});
    blob_or_file e = zblob{};
    blob_or_file f(zblob{});
    zblob_or_file g = zblob{};
    zblob_or_file h(zblob{});

    static_assert(!std::is_constructible_v<zblob_or_file, blob_or_file>);
    static_assert(!std::is_constructible_v<zblob_or_file, blob>);
}
