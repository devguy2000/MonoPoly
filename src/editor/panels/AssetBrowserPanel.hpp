#pragma once
#include <string>
#include <vector>
#include <functional>

// Callback: (contentName, absoluteFullPath)
using AssetSelectCb = std::function<void(const std::string&, const std::string&)>;

class AssetBrowserPanel {
public:
    void SetProject(const std::string& projectPath, const std::string& contentDir);
    void SetOnSelect(AssetSelectCb cb) { m_onSelect = std::move(cb); }
    void OnImGuiRender();
    void Refresh();

private:
    struct Asset {
        std::string name;      // content name, no extension
        std::string filename;  // e.g. "player.png"
        std::string fullPath;
    };

    void DrawDropTarget();
    void ScanDirectory();

    std::string          m_projectPath;
    std::string          m_contentDir;
    std::vector<Asset>   m_assets;
    AssetSelectCb        m_onSelect;
    int                  m_selected = -1;
};
