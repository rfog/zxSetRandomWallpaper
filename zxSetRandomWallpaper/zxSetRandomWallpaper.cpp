#define _WIN32_IE 0x0500 

// 2. Windows y OLE
#include <windows.h>

// 3. �CRUCIAL! Tipos de Internet necesarios para IActiveDesktop (struct COMPONENT, etc.)
#include <wininet.h> 

// 4. Shell headers
#include <shlobj.h>
#include <shlobj_core.h>
#include <shobjidl.h>
#include <shlguid.h>


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

    // Preferred modern API: IDesktopWallpaper (applies to all monitors)
    IDesktopWallpaper* pDesktopWallpaper = nullptr;
    hr = CoCreateInstance(CLSID_DesktopWallpaper, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDesktopWallpaper));
    if (SUCCEEDED(hr) && pDesktopWallpaper)
    {
        // Passing nullptr for the monitor ID sets the wallpaper for all monitors.
        hr = pDesktopWallpaper->SetWallpaper(nullptr, w.c_str());
        if (SUCCEEDED(hr))
        {
            ok = true;
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

#include <shellapi.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc != 2)
    {
        if(argv) LocalFree(argv);
        return -1;
    }

    const path dir(argv[1]);
    LocalFree(argv);

    try
    {
        if (!exists(dir) || !is_directory(dir))
        {
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
            return -1;
        }

        // Pick random file
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
        path chosen = files[dist(gen)];

        // Try modern IDesktopWallpaper/IActiveDesktop API first
        if (SetWallpaperActiveDesktop(chosen))
        {
            return 0;
        }

        // Fallback to SPI
        if (SetWallpaperSPI(chosen))
        {
            return 0;
        }

        return -1;
    }
    catch (const filesystem_error&)
    {
        return -1;
    }

    return 0;
}
