#include "DiskCache.hpp"
#include <filesystem>
#include <fstream>

namespace slippygl::cache
{
	DiskCache::DiskCache(const CacheConfig& cfg)
		: cfg_(cfg)
	{
		if (cfg_.rootDir().empty()) {
			throw std::invalid_argument("Cache root directory must be set");
		}
	}

	bool DiskCache::loadRaster(const slippygl::core::TileID& id, std::vector<std::uint8_t>& outBytes) noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		const std::string path = rasterPath(id);
		
		if (!std::filesystem::exists(path)) 
		{
			return false; // cache miss
		}

		// Read file
		try 
		{
			std::ifstream ifs(path, std::ios::binary);
			if (!ifs) 
            {
				return false; // file open failed
			}
			outBytes.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
			return true; // cache hit
		} 
		catch (const std::exception&) 
		{
			return false; // file read failed
		}
	}

	bool DiskCache::saveRaster(const slippygl::core::TileID& id,
							   const std::vector<std::uint8_t>& bytes,
							   const std::optional<CacheMeta>& meta) noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		const std::string path = rasterPath(id);
		
		if (!ensureParentDir(path)) 
		{
			return false; // Directory creation failed
		}
		if (!atomicWriteFile(path + ".part", bytes)) 
		{
			return false; // Temp file write failed
		}
		if (meta.has_value()) 
		{
			saveMetaInternal(id, meta.value()); // Call internal method (no lock)
		}

		try {
			std::filesystem::rename(path + ".part", path); // Atomic rename
			return true;
		} catch (const std::exception&) {
			return false;
		}
	}

	bool DiskCache::loadMeta(const slippygl::core::TileID& id, CacheMeta& out) noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		const std::string path = metaPath(id);
		if (!std::filesystem::exists(path)) 
		{
			return false; // Cache miss
		}
		try 
		{
			std::ifstream ifs(path);
			if (!ifs) 
			{
				return false; // File open failed
			}
			std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
			out = CacheMeta::fromJsonString(json); // JSON parse (CacheMeta requires fromJson static function)
			return true; // Cache hit
		} 
		catch (const std::exception&) 
		{
			return false; // File read failed
		}
	}

	bool DiskCache::saveMeta(const slippygl::core::TileID& id, const CacheMeta& meta) noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return saveMetaInternal(id, meta);
	}

	bool DiskCache::saveMetaInternal(const slippygl::core::TileID& id, const CacheMeta& meta) noexcept
	{
		// This method is called when lock is already held (no lock)
		const std::string path = metaPath(id);
		
		if (!ensureParentDir(path)) 
		{
			return false; // Directory creation failed
		}
		
		try 
		{
			std::string json = CacheMeta::toJsonString(meta); // JSON serialize
			if (!atomicWriteText(path + ".part", json)) {
				return false;
			}
			std::filesystem::rename(path + ".part", path);
			return true;
		} 
		catch (const std::exception&) 
		{
			return false; // File write failed
		}
	}

	bool DiskCache::exists(const slippygl::core::TileID& id) const noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return std::filesystem::exists(rasterPath(id));
	}

	bool DiskCache::remove(const slippygl::core::TileID& id) noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		try 
		{
			std::filesystem::remove(rasterPath(id));
			std::filesystem::remove(metaPath(id));
			return true; // 성공적으로 삭제
		} 
		catch (const std::exception&) 
		{
			return false; // 삭제 실패
		}
	}

	void DiskCache::clearAll() noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		try 
		{
			std::filesystem::remove_all(cfg_.rootDir());
		} 
		catch (const std::exception&) 
		{
			// 예외 무시: 전체 캐시 비우기 실패는 무시
		}
	}

	std::string DiskCache::rasterPath(const slippygl::core::TileID& id) const
	{
		return cfg_.rootDir() + "/" + cfg_.rasterDirName() + "/" + id.toString() + ".png";
	}

	std::string DiskCache::metaPath(const slippygl::core::TileID& id) const
	{
		return cfg_.rootDir() + "/" + cfg_.metaDirName() + "/" + id.toString() + ".json";
	}

	bool DiskCache::ensureParentDir(const std::string& filePath) const noexcept
	{
		try 
		{
			std::filesystem::path dir = std::filesystem::path(filePath).parent_path();
			if (!std::filesystem::exists(dir)) 
			{
				std::filesystem::create_directories(dir); // 부모 디렉토리 생성
			}
			return true;
		} 
		catch (const std::exception&) 
		{
			return false; // 디렉토리 생성 실패
		}
	}

	bool DiskCache::atomicWriteFile(const std::string& dstPath, const std::vector<std::uint8_t>& bytes) const noexcept
	{
		try 
		{
			std::ofstream ofs(dstPath, std::ios::binary);
			if (!ofs)
			{
				return false; // 파일 열기 실패
			}
			ofs.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
			return ofs.good(); // 쓰기 성공 여부 반환
		} 
		catch (const std::exception&) 
		{
			return false; // 예외 발생 시 실패
		}
	}

	bool DiskCache::atomicWriteText(const std::string& dstPath, const std::string& text) const noexcept
	{
		try 
		{
			std::ofstream ofs(dstPath);
			if (!ofs) return false; // 파일 열기 실패
			ofs << text;
			return ofs.good(); // 쓰기 성공 여부 반환
		} 
		catch (const std::exception&) 
		{
			return false; // 예외 발생 시 실패
		}
	}
};