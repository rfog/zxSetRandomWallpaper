#define _WIN32_IE 0x0500 

// 2. Windows y OLE
#include <windows.h>

// 3. ¡CRUCIAL! Tipos de Internet necesarios para IActiveDesktop (struct COMPONENT, etc.)
#include <wininet.h> 

// 4. Shell headers
#include <shlobj.h>
#include <shlobj_core.h>
#include <shobjidl.h>
#include <shlguid.h>

#include <iostream>
#include <string>
#include <filesystem>
#include <system_error>
#include <vector>
#include <random>
// #include <codecvt> // Not needed for the C++20 u8string fix

using namespace std::filesystem;

// Try setting wallpaper using SystemParametersInfoW
static bool SetWallpaperSPI(const path& p)
{
    std::wstring w = p.wstring();
    BOOL res = SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)w.c_str(),
                                     SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    return res != FALSE;
}

// Try setting wallpaper using IDesktopWallpaper (preferred) and fallback to IActiveDesktop
static bool SetWallpaperActiveDesktop(const path& p)
{
    // Initialize COM (may return RPC_E_CHANGED_MODE if another threading model is already set;
    // continue in that case)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    bool coInitialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return false;

    std::wstring w = p.wstring();
    bool ok = false;

    // Preferred modern API: IDesktopWallpaper (applies per-monitor)
    IDesktopWallpaper* pDesktopWallpaper = nullptr;
    hr = CoCreateInstance(CLSID_DesktopWallpaper, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDesktopWallpaper));
    if (SUCCEEDED(hr) && pDesktopWallpaper)
    {
        UINT count = 0;
        if (SUCCEEDED(pDesktopWallpaper->GetMonitorDevicePathCount(&count)))
        {
            for (UINT i = 0; i < count; ++i)
            {
                PWSTR monitorId = nullptr;
                if (SUCCEEDED(pDesktopWallpaper->GetMonitorDevicePathAt(i, &monitorId)) && monitorId)
                {
                    // Set wallpaper for this monitor
                    hr = pDesktopWallpaper->SetWallpaper(monitorId, w.c_str());
                    CoTaskMemFree(monitorId);
                    if (SUCCEEDED(hr))
                    {
                        ok = true; // at least one monitor succeeded
                    }
                    // continue trying remaining monitors even if one fails
                }
            }
        }
        // Optionally set wallpaper position for all monitors (CENTER/CROP/FIT/...)
        // pDesktopWallpaper->SetPosition(DWPOS_CENTER); // example
        pDesktopWallpaper->Release();
    }

    // Fallback: IActiveDesktop (older Windows, also used previously)
    if (!ok)
    {
        IActiveDesktop* pActiveDesktop = nullptr;
        // With headers included in the correct order, CLSID_ActiveDesktop and IActiveDesktop 
        // should now be recognized properly.
        hr = CoCreateInstance(CLSID_ActiveDesktop, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pActiveDesktop));
        if (SUCCEEDED(hr) && pActiveDesktop)
        {
            hr = pActiveDesktop->SetWallpaper(w.c_str(), 0);
            if (SUCCEEDED(hr))
            {
                hr = pActiveDesktop->ApplyChanges(AD_APPLY_ALL);
                ok = SUCCEEDED(hr);
            }
            pActiveDesktop->Release();
        }
    }

    if (coInitialized)
        CoUninitialize();

    return ok;
}

int main(int argc, char** argv)
{
#ifdef _WIN32
    // Ensure UTF-8 output in Windows console
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc != 2)
    {
        std::cout << "USAGE: zxSetRandomWallpaper <wallpaper_path>\n";
        return -1;
    }

    const std::string folder(argv[1]);

    try
    {
        path dir = path(folder);

        if (!exists(dir))
        {   
            std::cerr << "Error: path does not exist: " << folder << '\n';
            return -1;
        }
        if (!is_directory(dir))
        {
            std::cerr << "Error: not a directory: " << folder << '\n';
            return -1;
        }

        // Collect regular files
        std::vector<path> files;
        for (const auto& entry : directory_iterator(dir))
        {
            std::error_code ec;
            if (entry.is_regular_file(ec) && !ec)
            {
                files.push_back(entry.path());
            }
        }

        if (files.empty())
        {
            std::cerr << "No files found in: " << folder << '\n';
            return -1;
        }

        // Pick random file
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
        path chosen = files[dist(gen)];

        // FIX 2: In C++20, u8string() returns std::u8string (char8_t), which std::cout doesn't accept.
        // Since we set CP_UTF8, we can reinterpret_cast to const char*.
        const auto& u8str = chosen.u8string();
        std::cout << "Chosen wallpaper: " << reinterpret_cast<const char*>(u8str.c_str()) << '\n';

        // Try SPI first
        if (SetWallpaperSPI(chosen))
        {
            std::cout << "Wallpaper set via SystemParametersInfo.\n";
            return 0;
        }

        // Fallback to IActiveDesktop
        if (SetWallpaperActiveDesktop(chosen))
        {
            std::cout << "Wallpaper set via IActiveDesktop.\n";
            return 0;
        }

        std::cerr << "Failed to set wallpaper (both SPI and ActiveDesktop failed).\n";
        return -1;
    }
    catch (const filesystem_error& e)
    {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        return -1;
    }

    return 0;
}
