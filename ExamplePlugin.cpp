// ExamplePlugin.cpp : Defines the exported functions for the MysticThumbs plugin.
//
#include "ExamplePlugin.h"
#include <Shlwapi.h>
#include <atlbase.h>
#include <new>
#include <filesystem>
#include <Windows.h>
#include <assert.h>
#include "MysticThumbsPlugin.h" // this should be in the example source
#include "resource.h"

#ifndef PI
#define PI 3.14159265358979f
#endif

// Define a name and the extensions supported by this plugin
static const LPCWSTR s_name = L"Example Plugin";
static const LPCWSTR s_extensions[] = { L".mtp" };

// Generate a GUID using Visual Studio Tools menu -> Create GUID and copy and paste here for uniqueness.
// {4501A6B8-65EF-42A4-8AB1-F5F9FB23A91F}
static const GUID s_guid = { 0x4501a6b8, 0x65ef, 0x42a4, { 0x8a, 0xb1, 0xf5, 0xf9, 0xfb, 0x23, 0xa9, 0x1f } };

// Where your plugin settings will be stored in the registry appended with s_name / the name of your plugin
// It is HIGHLY recommended to use this path because MysticThumbs has built-in backup and restore of plugin settings under this key and everything is stored in the same registry name space.
constexpr LPCWSTR SZ_REGISTRY_PLUGINS_ROOT_KEY = L"Software\\MysticCoder\\MysticThumbs\\Plugins";

// Store the module handle from DllMain for use in dialogs
static HMODULE g_hModule;


// Added in VERSION 3.
// If you need the full path of the file being read you can query the IStream for IPersistFile.
// Note that this may not always be available and may not be the full path depending on how the stream was created, but it should be.
#if 0
#include <atlbase.h> // to get the smart pointers
CComQIPtr<IPersistFile> pPersistFile(pStream);
if(pPersistFile) {
    CComHeapPtr<OLECHAR> pszFileName; // Don't leak memory - this is required. Use CoTaskMemFree if not using smart pointers
    if(SUCCEEDED(pPersistFile->GetCurFile(&pszFileName))) {
        // pszFileName contains the full path. use it as needed.
    }
}
#endif



class CExamplePlugin : public IMysticThumbsPlugin
{
private:
    // Cache this from CreateInstance() for use through the lifetime of the plugin instance until the end of Destroy().
    const IMysticThumbsPluginContext* context;

    // Example configuration structure to hold plugin settings and Load/Save to the registry.
    // It doesn't need to be done this way, this example just keeps the configuration code and settings in one place.
    struct PluginConfig
    {
        bool alternateColorSchemeEnabled{};

        void Load(HWND hwndDlg = nullptr) // hwndDlg is only used in the control panel configuration, if provided the dialog controls will be updated. When loading for normal use it will be nullptr.
        {
            // Registry path to load settings from
            auto subKey = std::filesystem::path(SZ_REGISTRY_PLUGINS_ROOT_KEY) / s_name;

            HKEY hKey;
            if(RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD dwSize = sizeof(DWORD);

                if(RegQueryValueExW(hKey, L"AlternateColorScheme", nullptr, nullptr, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
                    alternateColorSchemeEnabled = (dwValue != 0);
                    if(hwndDlg) {
                        CheckDlgButton(hwndDlg, IDC_ALTERNATE_COLOR_SCHEME, alternateColorSchemeEnabled ? BST_CHECKED : BST_UNCHECKED);
                    }
                }

                // Load other options using the hKey parent as required.

                RegCloseKey(hKey);
            }
        }

        void Save(HWND hwndDlg) const
        {
            // Registry path to load settings from
            auto subKey = std::filesystem::path(SZ_REGISTRY_PLUGINS_ROOT_KEY) / s_name;

            HKEY hKey;
            if(RegCreateKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
                DWORD dwValue;
                
                dwValue = alternateColorSchemeEnabled ? 1 : 0;
                RegSetValueExW(hKey, L"AlternateColorScheme", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

                // Save other options using the hKey parent has required.

                RegCloseKey(hKey);
            }
        }
    } config{};

    static INT_PTR CALLBACK ConfigureDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    virtual ~CExamplePlugin()
    {
        // Called when the plugin is being destroyed.
        // Free any allocated resources here.
    }

public:

    CExamplePlugin(_In_ IMysticThumbsPluginContext* context) : context(context)
    {
        // Called when the plugin is initially created.
        // It may not necesserally be used immediately, it may just be an object to check the interface information so be careful about allocating anything here.
    }

private:

    _Notnull_ LPCWSTR GetName() override
    {
        return s_name;
    }

    _Notnull_ LPCGUID GetGuid() override
    {
        return &s_guid;
    }

    _Notnull_ LPCWSTR GetDescription() override
    {
        return L"Example Plugin/nGenerates procedural images for demonstration purposes./nCopyright (c) 2026 MysticCoder";
    }

    _Notnull_ LPCWSTR GetAuthor() override
    {
        return L"MysticCoder/nmysticcoder.net";
    }

    unsigned int GetExtensionCount() override
    {
        return ARRAYSIZE(s_extensions);
    }

    _Notnull_ LPCWSTR GetExtension(_In_ unsigned int index) override
    {
        return s_extensions[index];
    }

    void Destroy() override
    {
        // Delete this instance
        this->~CExamplePlugin();
        CoTaskMemFree(this);
    }

    bool Ping(_Inout_ MysticThumbsPluginPing& ping) override
    {
        // Load our configuration if needed (PluginCapabilities_CanConfigure is set)
        config.Load();

        // Fill in the output fields of the ping struct.
        // 
        // In a normal image type plugin, fill out the width, height, bitdepth with the actual image dimensions.
        // For this example plugin that is dynamic (using PluginCapabilities_CanNonUniformSize),
        // use either the QuickView window size or the requested thumbnail size.
        // 
        // Any ping members untouched will be ignored.

        bool isQuickView = !!(ping.flags & MysticThumbsPluginPingFlags_QuickView);

        ping.width = isQuickView && ping.requestedWidth ? ping.requestedWidth : 256;
        ping.height = isQuickView && ping.requestedHeight ? ping.requestedHeight : 256;
        ping.bitDepth = 32;

        context->logf(L"ExamplePlugin: PingImage called, returning %ux%u @ %u bpp", ping.width, ping.height, ping.bitDepth);

        return true;
    }



    /////////////////////////////////////////////////////////////////////////////
    // VERSION 3 additions
    /////////////////////////////////////////////////////////////////////////////


    bool GetCapabilities(_Out_ MysticThumbsPluginCapabilities& capabilities) override
    {
        capabilities = {};
        capabilities |= PluginCapabilities_CanConfigure | PluginCapabilities_CanNonUniformSize;
        return true;
    };

    bool Configure(_In_ MysticThumbsPluginConfigureInfo& configInfo) override
    {
#ifdef _DEBUG
        context->log(L"ExamplePlugin: Configure log called");
        context->logf(L"ExamplePlugin: Configure logf called with %f", 1.618f);
#endif // _DEBUG

        INT_PTR result = DialogBoxParamW(g_hModule,
                                         MAKEINTRESOURCE(IDD_EXAMPLE_PLUGIN_CONFIGURE),
                                         configInfo.hParentDialog,
                                         ConfigureDialogProc,
                                         (LPARAM)this);

        return result == IDOK;
    };

    // A new, more flexible image generation method using WIC and ATL. Highly recommended.
    // Does the same thing as the GenerateImage example but using WIC to create the bitmap source directly. This way the result can be efficiently handled by MysticThumbs, especially if it is a GUID_WICPixelFormat32bppPBGRA format image.
    HRESULT Generate(_Inout_ MysticThumbsPluginGenerateParams& params, _COM_Outptr_result_maybenull_ IWICBitmapSource** lplpOutputImage) override
    {
        if(!lplpOutputImage)
            return E_POINTER;

        *lplpOutputImage = nullptr;

        HRESULT hr;

        // The stream is pointing to the head of the file stream on entry so you can begin by reading the file header etc.
        CComPtr<IWICImagingFactory> pWICFactory;
        if(FAILED(hr = pWICFactory.CoCreateInstance(CLSID_WICImagingFactory))) {
            return hr;
        }

        CComPtr<IWICBitmap> pBitmap;
        if(FAILED(hr = pWICFactory->CreateBitmap(params.desiredWidth, params.desiredHeight, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &pBitmap))) {
            return hr;
        }

        CComPtr<IWICBitmapLock> pBitmapLock;
        if(FAILED(pBitmap->Lock(nullptr, WICBitmapLockWrite, &pBitmapLock))) { // Lock the entire bitmap for riting
            return hr;
        }

        UINT outStride;
        if(FAILED(hr = pBitmapLock->GetStride(&outStride))) {
            return hr;
        }

        UINT bufferSize;
        WICInProcPointer image{};
        if(FAILED(hr = pBitmapLock->GetDataPointer(&bufferSize, &image))) {
            return hr;
        }
        if(!image)
            return E_UNEXPECTED;

        params.flags |= MT_HasAlpha;

        // Decode
        unsigned int width = params.desiredWidth;
        unsigned int height = params.desiredHeight;

        for(unsigned int y = 0; y < height; ++y) {
            float fy = ((float)y / (height - 1)) * 2 - 1;
            unsigned char* ptr = image + y * outStride;
            for(unsigned int x = 0; x < width; ++x) {
                // For this example, calculate some RGBA values since we don't actually have a real file stream
                // Instead of calculating values here we would normally read from pStream to gather the image data
                float fx = ((float)x / (width - 1)) * 2 - 1;
                float r = (1 + sinf(fx * PI)) * 0.5f;
                float g = (1 + cosf(fy * PI)) * 0.5f;
                float b = (1 + sinf(fx * fy * PI)) * 0.5f;
                float a = sqrtf(fx * fx + fy * fy);

                // Fill in the channel information for this pixel
                if(config.alternateColorSchemeEnabled) {
                    *ptr++ = (unsigned char)(b * 255);
                    *ptr++ = (unsigned char)(r * 255);
                    *ptr++ = (unsigned char)(g * 255);
                } else {
                    *ptr++ = (unsigned char)(r * 255);
                    *ptr++ = (unsigned char)(g * 255);
                    *ptr++ = (unsigned char)(b * 255);
                }
                *ptr++ = (unsigned char)(a * 255);
            }
        }

        *lplpOutputImage = pBitmap.Detach(); // Detach the smart pointer to return the bitmap. It is now owned by the caller process.

        context->logf(L"ExamplePlugin: Generate called, returning %ux%u @ %u bpp", params.desiredWidth, params.desiredHeight, 32);

        return hr;
    }
};



// Dialog procedure for configuration dialog
INT_PTR CALLBACK CExamplePlugin::ConfigureDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
        case WM_INITDIALOG:
        {
            // Get the plugin instance pointer and store it in window user data for later use
            CExamplePlugin* plugin = (CExamplePlugin*)lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)plugin);

            // Load current registry value and set checkbox state
            plugin->config.Load(hwndDlg);

            return TRUE;
        }

        case WM_COMMAND:
        {
            CExamplePlugin* plugin = (CExamplePlugin*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
            assert(plugin);

            int wNotifyCode = HIWORD(wParam);
            int wID = LOWORD(wParam);

            switch(wID) {
                case IDOK:
                {
                    if(wNotifyCode == BN_CLICKED) {
                        // Save the current configuration to the registry
                        plugin->config.Save(hwndDlg);
                        EndDialog(hwndDlg, IDOK);
                        return TRUE;
                    }
                    break;
                }

                case IDCANCEL:
                {
                    if(wNotifyCode == BN_CLICKED) {
                        EndDialog(hwndDlg, IDCANCEL);
                        return TRUE;
                    }
                    break;
                }

                case IDC_ALTERNATE_COLOR_SCHEME:
                {
                    if(wNotifyCode == BN_CLICKED) {
                        // toggle the checkbox state
                        plugin->config.alternateColorSchemeEnabled = !plugin->config.alternateColorSchemeEnabled;
                        CheckDlgButton(hwndDlg, wID, plugin->config.alternateColorSchemeEnabled ? BST_CHECKED : BST_UNCHECKED);
                        return TRUE;
                    }
                    break;
                }
            }
            break;
        }
    }

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Pluigin DLL exported functions
/////////////////////////////////////////////////////////////////////////////


// Return the version of the plugin compiled.
extern "C" EXAMPLEPLUGIN_API int Version()
{
    return MYSTICTHUMBS_PLUGIN_VERSION;
}

// Initialize the plugin. Perform any work here that needs to be done for all instances such as loading DLLs or initializing globals.
extern "C" EXAMPLEPLUGIN_API bool Initialize()
{
    // For this example we don't need to do anything
    return true;
}

// Shutdown the plugin. Clean everything up.
extern "C" EXAMPLEPLUGIN_API bool Shutdown()
{
    // For this example we don't need to do anything
    return true;
}

/// <summary>
/// Create an instance of a thumbnail plugin.
/// Version 3 added the context parameter to provide logging and other information.
/// </summary>
/// <param name="context">Instance specific context. May be cached and used until the end of Destroy().</param>
/// <returns>A new instance of this plugin.</returns>
extern "C" EXAMPLEPLUGIN_API IMysticThumbsPlugin* CreateInstance(_In_ IMysticThumbsPluginContext* context)
{
    // Be sure to use CoTaskMemAlloc and placement new to create your object so it is within
    // the processes memory space or it may end up being in protected memory if the DLL shuts down.
    // For other allocations, also use CoTaskMemAlloc and CoTaskMemFree as needed to best avoid memory issues.
    // This is not a strict requirement but is recommended to avoid potential memory access violations.

    // Note that the IMysticThumbsPlugin::Destroy method takes care of deleting this object when MysticThumbs is finished with it.
    CExamplePlugin* plugin = (CExamplePlugin*)CoTaskMemAlloc(sizeof(CExamplePlugin));
    // Placement new to initialize the object in the allocated memory.
    return new(plugin) CExamplePlugin(context);
}


// Very important DllMain info: Don't use any non- Kernel32.dll calls in here. It's dangerous and strictly prohibited.
// If you want to check this - research DllMain in the MSDN docs to see what you should and should not do here.
EXAMPLEPLUGIN_API BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, void* lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch(ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            g_hModule = hModule;
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}
