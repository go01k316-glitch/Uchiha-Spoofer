#pragma execution_character_set("utf-8")
#include "CacheManager.h"

namespace UchihaSpoofer::Utils {

bool RegistryCache::GetCached(const std::string& path, const std::string& key, std::string& value) {
    std::string cacheKey = MakeCacheKey(path, key);
    return cache_.Get(cacheKey, value);
}

void RegistryCache::SetCached(const std::string& path, const std::string& key, const std::string& value) {
    std::string cacheKey = MakeCacheKey(path, key);
    cache_.Put(cacheKey, value);
}

void RegistryCache::InvalidateCache() {
    cache_.Clear();
}

} // namespace UchihaSpoofer::Utils
