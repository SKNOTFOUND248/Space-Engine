#pragma once

#include "Json.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace Orbital {

namespace fs = std::filesystem;

struct AddonManifest {
    int schemaVersion = 1;
    std::string id;
    std::string displayName;
    std::string version = "1.0.0";
    std::string minimumEngineVersion = "0.1.0";
    std::string description;
    std::string license = "CC-BY-4.0";
    std::vector<std::string> dependencies;
    std::vector<std::string> contentRoots;
    fs::path addonDirectory;
    bool isEnabled = true;
};

class VFS {
public:
    static VFS& get() {
        static VFS instance;
        return instance;
    }

    void initialize(const fs::path& baseDirectory) {
        m_baseDir = fs::absolute(baseDirectory);

        // Ensure default physical directories exist
        createDirectoryIfMissing(m_baseDir / "data");
        createDirectoryIfMissing(m_baseDir / "data" / "catalogs");
        createDirectoryIfMissing(m_baseDir / "data" / "scenarios");
        createDirectoryIfMissing(m_baseDir / "data" / "shaders");
        createDirectoryIfMissing(m_baseDir / "data" / "textures");
        createDirectoryIfMissing(m_baseDir / "addons");

        // Mount default virtual roots
        mountVirtualRoot("catalogs", m_baseDir / "data" / "catalogs");
        mountVirtualRoot("scenarios", m_baseDir / "data" / "scenarios");
        mountVirtualRoot("shaders",  m_baseDir / "data" / "shaders");
        mountVirtualRoot("textures", m_baseDir / "data" / "textures");

        // Discover and mount addons
        discoverAddons();
    }

    void mountVirtualRoot(const std::string& virtualPrefix, const fs::path& physicalPath) {
        if (!fs::exists(physicalPath)) {
            try {
                fs::create_directories(physicalPath);
            } catch (...) {}
        }
        m_mountPoints[virtualPrefix].push_back(fs::absolute(physicalPath));
    }

    // Resolves a virtual path (e.g., "catalogs://sol_system.json" or "catalogs/sol_system.json") to a physical path
    std::optional<fs::path> resolvePath(const std::string& virtualUri) const {
        // Prevent path traversal
        if (virtualUri.find("..") != std::string::npos) {
            std::cerr << "[VFS ERROR] Path traversal detected and rejected: " << virtualUri << "\n";
            return std::nullopt;
        }

        std::string cleaned = virtualUri;
        // Strip protocol prefix if present
        if (cleaned.rfind("vfs://", 0) == 0) {
            cleaned = cleaned.substr(6);
        }

        // Extract virtual root
        size_t sep = cleaned.find("://");
        std::string prefix;
        std::string subpath;
        if (sep != std::string::npos) {
            prefix = cleaned.substr(0, sep);
            subpath = cleaned.substr(sep + 3);
        } else {
            size_t slash = cleaned.find_first_of("/\\");
            if (slash != std::string::npos) {
                prefix = cleaned.substr(0, slash);
                subpath = cleaned.substr(slash + 1);
            } else {
                prefix = "";
                subpath = cleaned;
            }
        }

        // 1. Search in addons first (higher priority override)
        for (const auto& addon : m_discoveredAddons) {
            if (!addon.isEnabled) continue;
            for (const auto& root : addon.contentRoots) {
                if (root == prefix || prefix.empty()) {
                    fs::path candidate = addon.addonDirectory / root / subpath;
                    if (fs::exists(candidate)) {
                        return candidate;
                    }
                }
            }
        }

        // 2. Search in base mount points
        auto it = m_mountPoints.find(prefix);
        if (it != m_mountPoints.end()) {
            for (const auto& dir : it->second) {
                fs::path candidate = dir / subpath;
                if (fs::exists(candidate)) {
                    return candidate;
                }
            }
        }

        // 3. Fallback: direct relative path from baseDir
        fs::path direct = m_baseDir / cleaned;
        if (fs::exists(direct)) {
            return direct;
        }

        return std::nullopt;
    }

    std::optional<std::string> readTextFile(const std::string& virtualUri) const {
        auto resolved = resolvePath(virtualUri);
        if (!resolved) {
            return std::nullopt;
        }

        std::ifstream file(*resolved, std::ios::in);
        if (!file.is_open()) {
            return std::nullopt;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool writeTextFile(const std::string& virtualUri, const std::string& content) {
        if (virtualUri.find("..") != std::string::npos) return false;

        // Extract prefix and subpath
        std::string cleaned = virtualUri;
        if (cleaned.rfind("vfs://", 0) == 0) cleaned = cleaned.substr(6);

        size_t sep = cleaned.find("://");
        std::string prefix;
        std::string subpath;
        if (sep != std::string::npos) {
            prefix = cleaned.substr(0, sep);
            subpath = cleaned.substr(sep + 3);
        } else {
            size_t slash = cleaned.find_first_of("/\\");
            if (slash != std::string::npos) {
                prefix = cleaned.substr(0, slash);
                subpath = cleaned.substr(slash + 1);
            } else {
                prefix = "";
                subpath = cleaned;
            }
        }

        fs::path targetDir;
        auto it = m_mountPoints.find(prefix);
        if (it != m_mountPoints.end() && !it->second.empty()) {
            targetDir = it->second.front();
        } else {
            targetDir = m_baseDir / "data" / prefix;
        }

        try {
            fs::create_directories(targetDir);
            fs::path filePath = targetDir / subpath;
            std::ofstream out(filePath, std::ios::out | std::ios::trunc);
            if (!out.is_open()) return false;
            out << content;
            return true;
        } catch (...) {
            return false;
        }
    }

    void discoverAddons() {
        m_discoveredAddons.clear();
        fs::path addonsDir = m_baseDir / "addons";
        if (!fs::exists(addonsDir)) return;

        for (const auto& entry : fs::directory_iterator(addonsDir)) {
            if (entry.is_directory()) {
                fs::path manifestPath = entry.path() / "manifest.json";
                if (fs::exists(manifestPath)) {
                    std::ifstream f(manifestPath);
                    if (f.is_open()) {
                        std::stringstream ss;
                        ss << f.rdbuf();
                        std::string err;
                        JsonValue manifestJson = JsonValue::parse(ss.str(), &err);
                        if (!manifestJson.isNull() && manifestJson.contains("id")) {
                            AddonManifest manifest;
                            manifest.schemaVersion = manifestJson["schemaVersion"].asInt(1);
                            manifest.id = manifestJson["id"].asString();
                            manifest.displayName = manifestJson["displayName"].asString(manifest.id);
                            manifest.version = manifestJson["version"].asString("1.0.0");
                            manifest.minimumEngineVersion = manifestJson["minimumEngineVersion"].asString("0.1.0");
                            manifest.description = manifestJson["description"].asString("");
                            manifest.license = manifestJson["license"].asString("CC-BY-4.0");
                            manifest.addonDirectory = entry.path();
                            manifest.isEnabled = true;

                            for (const auto& root : manifestJson["contentRoots"].asArray()) {
                                manifest.contentRoots.push_back(root.asString());
                            }
                            if (manifest.contentRoots.empty()) {
                                manifest.contentRoots = {"catalogs", "scenarios", "textures"};
                            }

                            m_discoveredAddons.push_back(manifest);
                            std::cout << "[VFS] Discovered add-on: " << manifest.displayName 
                                      << " (" << manifest.id << " v" << manifest.version << ")\n";
                        }
                    }
                }
            }
        }
    }

    const std::vector<AddonManifest>& getAddons() const { return m_discoveredAddons; }
    std::vector<AddonManifest>& getAddons() { return m_discoveredAddons; }

    const fs::path& getBaseDirectory() const { return m_baseDir; }

private:
    VFS() = default;

    void createDirectoryIfMissing(const fs::path& p) {
        if (!fs::exists(p)) {
            try {
                fs::create_directories(p);
            } catch (...) {}
        }
    }

    fs::path m_baseDir;
    std::unordered_map<std::string, std::vector<fs::path>> m_mountPoints;
    std::vector<AddonManifest> m_discoveredAddons;
};

} // namespace Orbital
