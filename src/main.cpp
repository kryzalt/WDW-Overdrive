#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0700

#include <windows.h>
#include <dinput.h>
#include <cguid.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "user32.lib")

#pragma pack(push, 1)
struct WDW_D3DTLVertex {
    float sx;
    float sy;
    float sz;
    float rhw;
    uint32_t color;
    uint32_t specular;
    float tu;
    float tv;
};
#pragma pack(pop)




#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30

typedef struct _XINPUT_GAMEPAD {
    WORD  wButtons;
    BYTE  bLeftTrigger;
    BYTE  bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE {
    DWORD          dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef DWORD (WINAPI *XInputGetState_t)(DWORD dwUserIndex, XINPUT_STATE* pState);
static XInputGetState_t Real_XInputGetState = NULL;

void InitXInput() {
    const char* xinputDlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    for (int i = 0; i < 3; ++i) {
        HMODULE hMod = LoadLibraryA(xinputDlls[i]);
        if (hMod) {
            Real_XInputGetState = (XInputGetState_t)GetProcAddress(hMod, "XInputGetState");
            if (Real_XInputGetState) break;
        }
    }
}




static int   g_TargetWidth         = 0;
static int   g_TargetHeight        = 0;
static float g_AspectMultiplier    = 1.0f;

static uint32_t g_PGXPEnabled       = 1;
static uint32_t g_WidescreenEnabled = 1;
static uint32_t g_ShowDebugOverlay  = 0;
static uint32_t g_RubberbandAI      = 1;
static uint32_t g_NemesisAI         = 1;

static int   g_EnableDebugMenu     = 1;
static int   g_DebugMenuHotkey     = VK_F1;
static int   g_FreecamHotkey       = VK_F3;
static int   g_AutoSkipFlyby       = 0;

inline float GetCurrentAspectMultiplier() {
    return (g_WidescreenEnabled & 1) ? g_AspectMultiplier : 1.0f;
}

void LoadSettings() {
    char iniPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, iniPath);
    strcat_s(iniPath, "\\wdw_fix.ini");

    g_TargetWidth  = GetPrivateProfileIntA("Display", "Width", 0, iniPath);
    g_TargetHeight = GetPrivateProfileIntA("Display", "Height", 0, iniPath);

    if (g_TargetWidth <= 0 || g_TargetHeight <= 0) {
        g_TargetWidth  = GetSystemMetrics(SM_CXSCREEN);
        g_TargetHeight = GetSystemMetrics(SM_CYSCREEN);
    }

    if (g_TargetWidth <= 0 || g_TargetHeight <= 0) {
        g_TargetWidth  = 640;
        g_TargetHeight = 480;
    }

    float currentAspect = (float)g_TargetWidth / (float)g_TargetHeight;
    g_AspectMultiplier  = currentAspect / (4.0f / 3.0f);
    if (g_AspectMultiplier < 1.0f) g_AspectMultiplier = 1.0f;

    g_EnableDebugMenu  = GetPrivateProfileIntA("Cheats", "EnableDebugMenu",    1, iniPath);
    g_DebugMenuHotkey  = GetPrivateProfileIntA("Cheats", "DebugMenuHotkey", VK_F1, iniPath);
    g_FreecamHotkey    = GetPrivateProfileIntA("Cheats", "FreecamHotkey",   VK_F3, iniPath);
    g_RubberbandAI     = GetPrivateProfileIntA("Cheats", "AICatchUp",          1, iniPath);
    g_NemesisAI        = GetPrivateProfileIntA("Cheats", "NemesisAI",          1, iniPath);
    g_ShowDebugOverlay = GetPrivateProfileIntA("Cheats", "DebugOverlay",       0, iniPath);
    g_AutoSkipFlyby    = GetPrivateProfileIntA("Cheats", "AutoSkipFlyby",      0, iniPath);
}




typedef HRESULT (WINAPI *DirectInputCreateA_t)(HINSTANCE, DWORD, LPDIRECTINPUTA*, LPUNKNOWN);
typedef HRESULT (WINAPI *DirectInputCreateW_t)(HINSTANCE, DWORD, LPDIRECTINPUTW*, LPUNKNOWN);
typedef HRESULT (WINAPI *DirectInputCreateEx_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
typedef HRESULT (WINAPI *DllGetClassObject_t)(REFCLSID, REFIID, LPVOID*);
typedef HRESULT (WINAPI *DllCanUnloadNow_t)();
typedef HRESULT (WINAPI *DllRegisterServer_t)();
typedef HRESULT (WINAPI *DllUnregisterServer_t)();

static HMODULE               g_hRealDInput            = NULL;
static DirectInputCreateA_t  Real_DirectInputCreateA  = NULL;
static DirectInputCreateW_t  Real_DirectInputCreateW  = NULL;
static DirectInputCreateEx_t Real_DirectInputCreateEx = NULL;
static DllGetClassObject_t   Real_DllGetClassObject   = NULL;
static DllCanUnloadNow_t     Real_DllCanUnloadNow     = NULL;
static DllRegisterServer_t   Real_DllRegisterServer   = NULL;
static DllUnregisterServer_t Real_DllUnregisterServer = NULL;

bool LoadRealDInput() {
    if (g_hRealDInput) return true;
    char sysPath[MAX_PATH];
    GetSystemDirectoryA(sysPath, MAX_PATH);
    strcat_s(sysPath, "\\dinput.dll");

    g_hRealDInput = LoadLibraryA(sysPath);
    if (!g_hRealDInput) return false;

    Real_DirectInputCreateA  = (DirectInputCreateA_t)GetProcAddress(g_hRealDInput, "DirectInputCreateA");
    Real_DirectInputCreateW  = (DirectInputCreateW_t)GetProcAddress(g_hRealDInput, "DirectInputCreateW");
    Real_DirectInputCreateEx = (DirectInputCreateEx_t)GetProcAddress(g_hRealDInput, "DirectInputCreateEx");
    Real_DllGetClassObject   = (DllGetClassObject_t)GetProcAddress(g_hRealDInput, "DllGetClassObject");
    Real_DllCanUnloadNow     = (DllCanUnloadNow_t)GetProcAddress(g_hRealDInput, "DllCanUnloadNow");
    Real_DllRegisterServer   = (DllRegisterServer_t)GetProcAddress(g_hRealDInput, "DllRegisterServer");
    Real_DllUnregisterServer = (DllUnregisterServer_t)GetProcAddress(g_hRealDInput, "DllUnregisterServer");
    return true;
}

class ProxyDirectInput7A : public IDirectInput7A {
private:
    IDirectInput7A* m_pReal;
    ULONG m_refCount;
public:
    ProxyDirectInput7A(IDirectInput7A* pReal) : m_pReal(pReal), m_refCount(1) {}
    virtual ~ProxyDirectInput7A() { if (m_pReal) { m_pReal->Release(); m_pReal = nullptr; } }

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj) override {
        if (!ppvObj) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDirectInputA) ||
            IsEqualIID(riid, IID_IDirectInput2A) || IsEqualIID(riid, IID_IDirectInput7A)) {
            *ppvObj = this; AddRef(); return S_OK;
        }
        return m_pReal->QueryInterface(riid, ppvObj);
    }
    STDMETHOD_(ULONG, AddRef)() override { return InterlockedIncrement(&m_refCount); }
    STDMETHOD_(ULONG, Release)() override {
        ULONG ref = InterlockedDecrement(&m_refCount);
        if (ref == 0) { delete this; return 0; }
        return ref;
    }
    STDMETHOD(CreateDevice)(REFGUID rguid, LPDIRECTINPUTDEVICEA* lplpDirectInputDevice, LPUNKNOWN pUnkOuter) override {
        if (!lplpDirectInputDevice) return DIERR_INVALIDPARAM;
        if (IsEqualGUID(rguid, GUID_Joystick)) return DIERR_DEVICENOTREG;
        return m_pReal->CreateDevice(rguid, lplpDirectInputDevice, pUnkOuter);
    }
    STDMETHOD(EnumDevices)(DWORD dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback, LPVOID pvRef, DWORD dwFlags) override {
        if (!lpCallback) return DIERR_INVALIDPARAM;
        if (dwDevType == DIDEVTYPE_JOYSTICK) return DI_OK;
        if (dwDevType == 0 || dwDevType == DIDEVTYPE_KEYBOARD) {
            DIDEVICEINSTANCEA kbdDdi;
            ZeroMemory(&kbdDdi, sizeof(kbdDdi));
            kbdDdi.dwSize = sizeof(DIDEVICEINSTANCEA);
            kbdDdi.guidInstance = GUID_SysKeyboard;
            kbdDdi.guidProduct  = GUID_SysKeyboard;
            kbdDdi.dwDevType    = DIDEVTYPE_KEYBOARD | (DIDEVTYPEKEYBOARD_PCENH << 8);
            strcpy_s(kbdDdi.tszInstanceName, "Keyboard");
            strcpy_s(kbdDdi.tszProductName, "Keyboard");
            kbdDdi.wUsagePage = 1;
            kbdDdi.wUsage = 6;
            __try {
                if (lpCallback(&kbdDdi, pvRef) == DIENUM_STOP) return DI_OK;
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
            if (dwDevType == DIDEVTYPE_KEYBOARD) return DI_OK;
        }
        if (dwDevType == 0 || dwDevType == DIDEVTYPE_MOUSE) {
            DIDEVICEINSTANCEA mouseDdi;
            ZeroMemory(&mouseDdi, sizeof(mouseDdi));
            mouseDdi.dwSize = sizeof(mouseDdi);
            mouseDdi.guidInstance = GUID_SysMouse;
            mouseDdi.guidProduct  = GUID_SysMouse;
            mouseDdi.dwDevType    = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
            strcpy_s(mouseDdi.tszInstanceName, "Mouse");
            strcpy_s(mouseDdi.tszProductName, "Mouse");
            mouseDdi.wUsagePage = 1;
            mouseDdi.wUsage = 2;
            __try {
                if (lpCallback(&mouseDdi, pvRef) == DIENUM_STOP) return DI_OK;
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
            if (dwDevType == DIDEVTYPE_MOUSE) return DI_OK;
        }
        return DI_OK;
    }
    STDMETHOD(GetDeviceStatus)(REFGUID rguidInstance) override { return m_pReal->GetDeviceStatus(rguidInstance); }
    STDMETHOD(RunControlPanel)(HWND hwndOwner, DWORD dwFlags) override { return m_pReal->RunControlPanel(hwndOwner, dwFlags); }
    STDMETHOD(Initialize)(HINSTANCE hinst, DWORD dwVersion) override { return m_pReal->Initialize(hinst, dwVersion); }
    STDMETHOD(FindDevice)(REFGUID rguidClass, LPCSTR ptszName, LPGUID pguidInstance) override { return m_pReal->FindDevice(rguidClass, ptszName, pguidInstance); }
    STDMETHOD(CreateDeviceEx)(REFGUID rguid, REFIID riid, LPVOID* pvOut, LPUNKNOWN lpUnknownOuter) override {
        if (IsEqualGUID(rguid, GUID_Joystick)) return DIERR_DEVICENOTREG;
        return m_pReal->CreateDeviceEx(rguid, riid, pvOut, lpUnknownOuter);
    }
};

extern "C" {
HRESULT WINAPI DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA* lplpDirectInput, LPUNKNOWN punkOuter) {
    if (!LoadRealDInput() || !Real_DirectInputCreateA) return DIERR_NOTINITIALIZED;
    if (!lplpDirectInput) return DIERR_INVALIDPARAM;
    LPDIRECTINPUTA pRealDI = nullptr;
    HRESULT hr = Real_DirectInputCreateA(hinst, dwVersion, &pRealDI, punkOuter);
    if (FAILED(hr) || !pRealDI) return hr;
    IDirectInput7A* pDI7 = nullptr;
    hr = pRealDI->QueryInterface(IID_IDirectInput7A, (void**)&pDI7);
    if (SUCCEEDED(hr) && pDI7) {
        pRealDI->Release();
        *lplpDirectInput = (LPDIRECTINPUTA)(new ProxyDirectInput7A(pDI7));
        return DI_OK;
    }
    *lplpDirectInput = (LPDIRECTINPUTA)(new ProxyDirectInput7A((IDirectInput7A*)pRealDI));
    return DI_OK;
}
HRESULT WINAPI DirectInputCreateW(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTW* lplpDirectInput, LPUNKNOWN punkOuter) {
    if (!LoadRealDInput() || !Real_DirectInputCreateW) return DIERR_NOTINITIALIZED;
    return Real_DirectInputCreateW(hinst, dwVersion, lplpDirectInput, punkOuter);
}
HRESULT WINAPI DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* lplpDirectInput, LPUNKNOWN punkOuter) {
    if (!LoadRealDInput() || !Real_DirectInputCreateEx) return DIERR_NOTINITIALIZED;
    if (!lplpDirectInput) return DIERR_INVALIDPARAM;
    LPVOID pRealDI = nullptr;
    HRESULT hr = Real_DirectInputCreateEx(hinst, dwVersion, riidltf, &pRealDI, punkOuter);
    if (FAILED(hr) || !pRealDI) return hr;
    if (IsEqualIID(riidltf, IID_IDirectInput7A) || IsEqualIID(riidltf, IID_IDirectInput2A) || IsEqualIID(riidltf, IID_IDirectInputA)) {
        *lplpDirectInput = new ProxyDirectInput7A((IDirectInput7A*)pRealDI);
        return DI_OK;
    }
    *lplpDirectInput = pRealDI;
    return DI_OK;
}
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    if (!LoadRealDInput() || !Real_DllGetClassObject) return CLASS_E_CLASSNOTAVAILABLE;
    return Real_DllGetClassObject(rclsid, riid, ppv);
}
HRESULT WINAPI DllCanUnloadNow() {
    if (!LoadRealDInput() || !Real_DllCanUnloadNow) return S_FALSE;
    return Real_DllCanUnloadNow();
}
HRESULT WINAPI DllRegisterServer() {
    if (!LoadRealDInput() || !Real_DllRegisterServer) return E_FAIL;
    return Real_DllRegisterServer();
}
HRESULT WINAPI DllUnregisterServer() {
    if (!LoadRealDInput() || !Real_DllUnregisterServer) return E_FAIL;
    return Real_DllUnregisterServer();
}
}




void PatchMemory(uintptr_t address, const void* data, size_t size) {
    DWORD oldProtect;
    VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy((LPVOID)address, data, size);
    VirtualProtect((LPVOID)address, size, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)address, size);
}

void NopMemory(uintptr_t address, size_t size) {
    DWORD oldProtect;
    VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset((LPVOID)address, 0x90, size);
    VirtualProtect((LPVOID)address, size, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)address, size);
}

void PatchJmp(uintptr_t from, uintptr_t to) {
    uint8_t jmpOpcode = 0xE9;
    int32_t relativeOffset = (int32_t)(to - (from + 5));
    PatchMemory(from, &jmpOpcode, 1);
    PatchMemory(from + 1, &relativeOffset, 4);
}

void PatchCall(uintptr_t from, uintptr_t to) {
    uint8_t callOpcode = 0xE8;
    int32_t relativeOffset = (int32_t)(to - (from + 5));
    PatchMemory(from, &callOpcode, 1);
    PatchMemory(from + 1, &relativeOffset, 4);
}

void* CreateTrampoline(uintptr_t targetAddr, void* hookFunc, size_t stolenLen) {
    uint8_t* trampoline = (uint8_t*)VirtualAlloc(NULL, stolenLen + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(trampoline, (void*)targetAddr, stolenLen);
    trampoline[stolenLen] = 0xE9;
    int32_t relBack = (int32_t)((targetAddr + stolenLen) - ((uintptr_t)trampoline + stolenLen + 5));
    *(int32_t*)(trampoline + stolenLen + 1) = relBack;
    
    DWORD oldProtect;
    VirtualProtect((LPVOID)targetAddr, stolenLen, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)targetAddr = 0xE9;
    int32_t relHook = (int32_t)((uintptr_t)hookFunc - (targetAddr + 5));
    *(int32_t*)(targetAddr + 1) = relHook;
    for (size_t i = 5; i < stolenLen; ++i) *((uint8_t*)targetAddr + i) = 0x90;
    VirtualProtect((LPVOID)targetAddr, stolenLen, oldProtect, &oldProtect);

    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)targetAddr, stolenLen);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)trampoline, stolenLen + 5);

    return trampoline;
}

bool HookIAT(HMODULE hMod, const char* dllName, const char* funcName, void* newFunc, void** oldFunc) {
    if (!hMod) hMod = GetModuleHandleA(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hMod;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((uintptr_t)hMod + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;
    IMAGE_DATA_DIRECTORY importDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!importDir.Size) return false;
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((uintptr_t)hMod + importDir.VirtualAddress);
    for (; importDesc->Name; importDesc++) {
        const char* modName = (const char*)((uintptr_t)hMod + importDesc->Name);
        if (_stricmp(modName, dllName) == 0) {
            PIMAGE_THUNK_DATA thunkOrig = (PIMAGE_THUNK_DATA)((uintptr_t)hMod + importDesc->OriginalFirstThunk);
            PIMAGE_THUNK_DATA thunkIAT  = (PIMAGE_THUNK_DATA)((uintptr_t)hMod + importDesc->FirstThunk);
            if (!thunkOrig) thunkOrig = thunkIAT;
            for (; thunkIAT->u1.Function; thunkOrig++, thunkIAT++) {
                if (!(thunkOrig->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
                    PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)((uintptr_t)hMod + thunkOrig->u1.AddressOfData);
                    if (strcmp(importByName->Name, funcName) == 0) {
                        DWORD oldProtect;
                        VirtualProtect(&thunkIAT->u1.Function, sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect);
                        if (oldFunc) *oldFunc = (void*)thunkIAT->u1.Function;
                        thunkIAT->u1.Function = (uintptr_t)newFunc;
                        VirtualProtect(&thunkIAT->u1.Function, sizeof(uintptr_t), oldProtect, &oldProtect);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}




typedef HANDLE (WINAPI *CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static CreateFileA_t Real_CreateFileA = CreateFileA;

HANDLE WINAPI Hooked_CreateFileA(
    LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile
) {
    if (lpFileName) {
        if (GetFileAttributesA(lpFileName) != INVALID_FILE_ATTRIBUTES) {
            return Real_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
        const char* pSlash = strrchr(lpFileName, '\\');
        const char* pName  = pSlash ? (pSlash + 1) : lpFileName;
        if (GetFileAttributesA(pName) != INVALID_FILE_ATTRIBUTES) {
            return Real_CreateFileA(pName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
        char moviePath[MAX_PATH];
        sprintf_s(moviePath, "movie\\%s", pName);
        if (GetFileAttributesA(moviePath) != INVALID_FILE_ATTRIBUTES) {
            return Real_CreateFileA(moviePath, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
    }
    return Real_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}




struct PGXPEntry {
    uint32_t key;
    float    dx;
    float    dy;
    uint32_t frameId;
};

static const uint32_t PGXP_CACHE_SIZE = 131072;
static const uint32_t PGXP_MASK = PGXP_CACHE_SIZE - 1;
static PGXPEntry g_PGXPCache[PGXP_CACHE_SIZE] = { 0 };
static uint32_t  g_CurrentFrame = 1;
static int g_2DPrimitiveDepth = 0;

inline uint32_t HashKey(uint32_t k) {
    k ^= k >> 16;
    k *= 0x7feb352d;
    k ^= k >> 15;
    k *= 0x846ca68b;
    k ^= k >> 16;
    return k & PGXP_MASK;
}

inline void StoreSubpixel(uint32_t key, float dx, float dy) {
    uint32_t idx = HashKey(key);
    for (int i = 0; i < 32; ++i) {
        uint32_t pos = (idx + i) & PGXP_MASK;
        if (g_PGXPCache[pos].frameId == g_CurrentFrame && g_PGXPCache[pos].key == key) {
            g_PGXPCache[pos].dx = dx;
            g_PGXPCache[pos].dy = dy;
            return;
        }
        if (g_PGXPCache[pos].frameId != g_CurrentFrame) {
            g_PGXPCache[pos].key     = key;
            g_PGXPCache[pos].dx      = dx;
            g_PGXPCache[pos].dy      = dy;
            g_PGXPCache[pos].frameId = g_CurrentFrame;
            return;
        }
    }
}

inline bool LookupSubpixel(uint32_t key, float& outDx, float& outDy) {
    uint32_t idx = HashKey(key);
    for (int i = 0; i < 32; ++i) {
        uint32_t pos = (idx + i) & PGXP_MASK;
        if (g_PGXPCache[pos].frameId == g_CurrentFrame && g_PGXPCache[pos].key == key) {
            outDx = g_PGXPCache[pos].dx;
            outDy = g_PGXPCache[pos].dy;
            return true;
        }
        if (g_PGXPCache[pos].frameId != g_CurrentFrame) {
            return false;
        }
    }
    return false;
}

static int32_t* pGteFocalLength   = NULL;
static int32_t* pGteScreenCenterX = NULL;
static int32_t* pGteScreenCenterY = NULL;

static int32_t* const pDword_481D20 = (int32_t*)0x00481D20;
static int32_t* const pDword_481D24 = (int32_t*)0x00481D24;
static int32_t* const pDword_481D28 = (int32_t*)0x00481D28;
static int32_t* const pDword_481D50 = (int32_t*)0x00481D50;
static int32_t* const pDword_481D54 = (int32_t*)0x00481D54;
static int32_t* const pDword_481D40 = (int32_t*)0x00481D40;
static int32_t* const pDword_481D44 = (int32_t*)0x00481D44;

static int32_t* pGteDepthZ0 = NULL;
static int32_t* pGteDepthZ1 = NULL;
static int32_t* pGteDepthZ2 = NULL;

inline int ClampCoord(int v) {
    if (v < -1024) return -1024;
    if (v >  1023) return  1023;
    return v;
}

static const uintptr_t RET_ADDR_PERSPECTIVE_SINGLE = 0x004374AD;
void __cdecl CalcProjection_Single() {
    int focal = *pGteFocalLength;
    int cx    = *pGteScreenCenterX;
    int cy    = *pGteScreenCenterY;
    int z     = *pDword_481D28;

    if (z > 0) {
        int x_cam = *pDword_481D20;
        int y_cam = *pDword_481D24;

        int sx = (int)((int64_t)x_cam * focal / z) + cx;
        int sy = (int)((int64_t)y_cam * focal / z) + cy;
        *pDword_481D20 = sx;
        *pDword_481D24 = sy;

        float fx = (float)focal * (float)x_cam / (float)z + (float)cx;
        float fy = (float)focal * (float)y_cam / (float)z + (float)cy;

        int csx = ClampCoord(sx);
        int csy = ClampCoord(sy);
        uint32_t key = (uint16_t)(int16_t)csx | ((uint32_t)(uint16_t)(int16_t)csy << 16);

        StoreSubpixel(key, fx - (float)sx, fy - (float)sy);
    }
}

__declspec(naked) void AsmHook_PerspectiveCalc_Single() {
    __asm {
        pushad
        call CalcProjection_Single
        popad
        jmp  ds:[RET_ADDR_PERSPECTIVE_SINGLE]
    }
}

static const uintptr_t RET_ADDR_PERSPECTIVE_TRIANGLE = 0x00437A61;
void __cdecl CalcProjection_Triangle() {
    int focal = *pGteFocalLength;
    int cx    = *pGteScreenCenterX;
    int cy    = *pGteScreenCenterY;

    int z0 = *pGteDepthZ0;
    if (z0 > 0) {
        int x = *pDword_481D20, y = *pDword_481D24;
        int sx = (int)((int64_t)x * focal / z0) + cx;
        int sy = (int)((int64_t)y * focal / z0) + cy;
        *pDword_481D20 = sx; *pDword_481D24 = sy;
        float fx = (float)focal * (float)x / (float)z0 + (float)cx;
        float fy = (float)focal * (float)y / (float)z0 + (float)cy;
        int csx = ClampCoord(sx), csy = ClampCoord(sy);
        uint32_t key = (uint16_t)(int16_t)csx | ((uint32_t)(uint16_t)(int16_t)csy << 16);
        StoreSubpixel(key, fx - (float)sx, fy - (float)sy);
    }

    int z1 = *pGteDepthZ1;
    if (z1 > 0) {
        int x = *pDword_481D50, y = *pDword_481D54;
        int sx = (int)((int64_t)x * focal / z1) + cx;
        int sy = (int)((int64_t)y * focal / z1) + cy;
        *pDword_481D50 = sx; *pDword_481D54 = sy;
        float fx = (float)focal * (float)x / (float)z1 + (float)cx;
        float fy = (float)focal * (float)y / (float)z1 + (float)cy;
        int csx = ClampCoord(sx), csy = ClampCoord(sy);
        uint32_t key = (uint16_t)(int16_t)csx | ((uint32_t)(uint16_t)(int16_t)csy << 16);
        StoreSubpixel(key, fx - (float)sx, fy - (float)sy);
    }

    int z2 = *pGteDepthZ2;
    if (z2 > 0) {
        int x = *pDword_481D40, y = *pDword_481D44;
        int sx = (int)((int64_t)x * focal / z2) + cx;
        int sy = (int)((int64_t)y * focal / z2) + cy;
        *pDword_481D40 = sx; *pDword_481D44 = sy;
        float fx = (float)focal * (float)x / (float)z2 + (float)cx;
        float fy = (float)focal * (float)y / (float)z2 + (float)cy;
        int csx = ClampCoord(sx), csy = ClampCoord(sy);
        uint32_t key = (uint16_t)(int16_t)csx | ((uint32_t)(uint16_t)(int16_t)csy << 16);
        StoreSubpixel(key, fx - (float)sx, fy - (float)sy);
    }
}

__declspec(naked) void AsmHook_PerspectiveCalc_Triangle() {
    __asm {
        pushad
        call CalcProjection_Triangle
        popad
        jmp  ds:[RET_ADDR_PERSPECTIVE_TRIANGLE]
    }
}

typedef int (__cdecl *Clip_ComputeVertexFlags_fn)(WDW_D3DTLVertex* pVertex);
static const Clip_ComputeVertexFlags_fn Orig_Clip_ComputeVertexFlags = 
    (Clip_ComputeVertexFlags_fn)0x0040FEE0;

int __cdecl Hooked_Clip_ComputeVertexFlags(WDW_D3DTLVertex* pVertex) {
    if (g_2DPrimitiveDepth > 0 || !(g_PGXPEnabled & 1)) {
        return Orig_Clip_ComputeVertexFlags(pVertex);
    }

    int16_t ix = (int16_t)(int32_t)(pVertex->sx + (pVertex->sx >= 0.0f ? 0.05f : -0.05f));
    int16_t iy = (int16_t)(int32_t)(pVertex->sy + (pVertex->sy >= 0.0f ? 0.05f : -0.05f));
    uint32_t key = (uint16_t)ix | ((uint32_t)(uint16_t)iy << 16);

    float dx, dy;
    if (LookupSubpixel(key, dx, dy)) {
        if (dx >= -1.0f && dx <= 1.0f && dy >= -1.0f && dy <= 1.0f) {
            pVertex->sx += dx;
            pVertex->sy += dy;
        }
    }

    uint8_t alpha = (uint8_t)(pVertex->color >> 24);
    if (alpha < 0xFE && pVertex->sz > 0.005f) {
        pVertex->sz -= 0.0025f;
    }

    return Orig_Clip_ComputeVertexFlags(pVertex);
}

typedef void (__cdecl *Render_ClearOT_fn)(void* pState);
static const Render_ClearOT_fn Orig_Render_ClearOT = (Render_ClearOT_fn)0x00426900;

void __cdecl Hooked_Render_ClearOT(void* pState) {
    g_CurrentFrame++;
    Orig_Render_ClearOT(pState);
}

typedef int (__cdecl *Render_DrawSprt_fn)(void* pSprt);
static Render_DrawSprt_fn Orig_Render_DrawSprt = NULL;

int __cdecl Hooked_Render_DrawSprt(void* pSprt) {
    g_2DPrimitiveDepth++;
    int res = Orig_Render_DrawSprt(pSprt);
    g_2DPrimitiveDepth--;
    return res;
}

typedef int (__cdecl *Render_DrawPrim_Tile_fn)(void* pTile);
static Render_DrawPrim_Tile_fn Orig_Render_DrawPrim_Tile = NULL;

int __cdecl Hooked_Render_DrawPrim_Tile(void* pTile) {
    g_2DPrimitiveDepth++;
    int res = Orig_Render_DrawPrim_Tile(pTile);
    g_2DPrimitiveDepth--;
    return res;
}




typedef int (__cdecl *Camera_ComputeViewMatrix_fn)(int pCamera);
static const Camera_ComputeViewMatrix_fn Orig_Camera_ComputeViewMatrix = 
    (Camera_ComputeViewMatrix_fn)0x0043B740;

static float s_SmoothCamX = 0.0f;
static float s_SmoothCamY = 0.0f;
static float s_SmoothCamZ = 0.0f;
static bool  s_CamInitialized = false;

static bool  g_FreecamActive      = false;
static float s_FreecamX           = 0.0f;
static float s_FreecamY           = 0.0f;
static float s_FreecamZ           = 0.0f;
static float s_FreecamPitch       = 0.0f;
static float s_FreecamYaw         = 0.0f;
static bool  s_FreecamInitPosDone = false;

static POINT s_LastMousePos      = { 0, 0 };
static bool  s_MouseTrackingInit = false;

void UpdateFreecamControls() {
    POINT cur;
    if (GetCursorPos(&cur)) {
        if (!s_MouseTrackingInit) {
            s_LastMousePos = cur;
            s_MouseTrackingInit = true;
        }

        int dx = cur.x - s_LastMousePos.x;
        int dy = cur.y - s_LastMousePos.y;
        s_LastMousePos = cur;

        if (dx != 0 || dy != 0) {
            float mouseSens = 2.2f;
            s_FreecamYaw   -= (float)dx * mouseSens;
            s_FreecamPitch -= (float)dy * mouseSens;
        }

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        if (cur.x < 100 || cur.x > screenW - 100 || cur.y < 100 || cur.y > screenH - 100) {
            POINT center = { screenW / 2, screenH / 2 };
            SetCursorPos(center.x, center.y);
            s_LastMousePos = center;
        }
    }

    float rotSpeed = 24.0f;
    if ((GetAsyncKeyState(VK_UP) & 0x8000) != 0)    s_FreecamPitch += rotSpeed;
    if ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0)  s_FreecamPitch -= rotSpeed;
    if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0)  s_FreecamYaw   += rotSpeed;
    if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0) s_FreecamYaw   -= rotSpeed;

    if (s_FreecamPitch >  950.0f) s_FreecamPitch =  950.0f;
    if (s_FreecamPitch < -950.0f) s_FreecamPitch = -950.0f;
    s_FreecamYaw = (float)((int)s_FreecamYaw & 4095);

    float radYaw   = (s_FreecamYaw   * 6.2831853f) / 4096.0f;
    float radPitch = (s_FreecamPitch * 6.2831853f) / 4096.0f;

    float fwdX =  sinf(radYaw) * cosf(radPitch);
    float fwdY = -cosf(radYaw) * cosf(radPitch);
    float fwdZ =  sinf(radPitch);

    float rightX =  cosf(radYaw);
    float rightY =  sinf(radYaw);

    float moveSpeed = 22.0f;
    if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
        moveSpeed = 65.0f;
    }

    if ((GetAsyncKeyState('W') & 0x8000) != 0) {
        s_FreecamX -= fwdX * moveSpeed;
        s_FreecamY -= fwdY * moveSpeed;
        s_FreecamZ -= fwdZ * moveSpeed;
    }
    if ((GetAsyncKeyState('S') & 0x8000) != 0) {
        s_FreecamX += fwdX * moveSpeed;
        s_FreecamY += fwdY * moveSpeed;
        s_FreecamZ += fwdZ * moveSpeed;
    }
    if ((GetAsyncKeyState('D') & 0x8000) != 0) {
        s_FreecamX += rightX * moveSpeed;
        s_FreecamY += rightY * moveSpeed;
    }
    if ((GetAsyncKeyState('A') & 0x8000) != 0) {
        s_FreecamX -= rightX * moveSpeed;
        s_FreecamY -= rightY * moveSpeed;
    }

    if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0 || (GetAsyncKeyState('R') & 0x8000) != 0) {
        s_FreecamZ += moveSpeed;
    }
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 || (GetAsyncKeyState('F') & 0x8000) != 0) {
        s_FreecamZ -= moveSpeed;
    }
}

int __cdecl Hooked_Camera_ComputeViewMatrix(int pCamera) {
    if (pCamera) {
        int16_t* pRawPos = (int16_t*)pCamera;
        int16_t* pAngles = (int16_t*)((char*)pCamera + 8);

        if (g_FreecamActive) {
            if (!s_FreecamInitPosDone) {
                s_FreecamX     = (float)pRawPos[0];
                s_FreecamY     = (float)pRawPos[1];
                s_FreecamZ     = (float)pRawPos[2];
                s_FreecamPitch = (float)pAngles[0];
                s_FreecamYaw   = (float)pAngles[2];
                s_FreecamInitPosDone = true;
            }

            UpdateFreecamControls();

            pRawPos[0] = (int16_t)floorf(s_FreecamX + 0.5f);
            pRawPos[1] = (int16_t)floorf(s_FreecamY + 0.5f);
            pRawPos[2] = (int16_t)floorf(s_FreecamZ + 0.5f);

            pAngles[0] = (int16_t)s_FreecamPitch;
            pAngles[1] = 0;
            pAngles[2] = (int16_t)s_FreecamYaw;

            *(int16_t*)((char*)pCamera + 0xA8) = pRawPos[0];
            *(int16_t*)((char*)pCamera + 0xAA) = pRawPos[1];
            *(int16_t*)((char*)pCamera + 0xAC) = pRawPos[2];
            *(int16_t*)((char*)pCamera + 0xB0) = pAngles[0];
            *(int16_t*)((char*)pCamera + 0xB2) = 0;
            *(int16_t*)((char*)pCamera + 0xB4) = pAngles[2];
        } else {
            s_FreecamInitPosDone = false;

            float targetX = (float)pRawPos[0];
            float targetY = (float)pRawPos[1];
            float targetZ = (float)pRawPos[2];

            if (!s_CamInitialized) {
                s_SmoothCamX = targetX;
                s_SmoothCamY = targetY;
                s_SmoothCamZ = targetZ;
                s_CamInitialized = true;
            } else {
                float distSq = (targetX - s_SmoothCamX)*(targetX - s_SmoothCamX) +
                               (targetY - s_SmoothCamY)*(targetY - s_SmoothCamY) +
                               (targetZ - s_SmoothCamZ)*(targetZ - s_SmoothCamZ);
                
                if (distSq > 40000.0f) {
                    s_SmoothCamX = targetX;
                    s_SmoothCamY = targetY;
                    s_SmoothCamZ = targetZ;
                } else {
                    s_SmoothCamX += (targetX - s_SmoothCamX) * 0.85f;
                    s_SmoothCamY += (targetY - s_SmoothCamY) * 0.85f;
                    s_SmoothCamZ += (targetZ - s_SmoothCamZ) * 0.85f;
                }
            }

            pRawPos[0] = (int16_t)floorf(s_SmoothCamX + 0.5f);
            pRawPos[1] = (int16_t)floorf(s_SmoothCamY + 0.5f);
            pRawPos[2] = (int16_t)floorf(s_SmoothCamZ + 0.5f);
        }
    }

    return Orig_Camera_ComputeViewMatrix(pCamera);
}




typedef int (__cdecl *HUD_DrawSpriteScaled_fn)(int spriteId, __int16 x, int y, __int16 scaleX, __int16 scaleY, int flags);
static HUD_DrawSpriteScaled_fn Orig_HUD_DrawSpriteScaled = NULL;

int __cdecl Hooked_HUD_DrawSpriteScaled(int spriteId, __int16 x, int y, __int16 scaleX, __int16 scaleY, int flags) {
    g_2DPrimitiveDepth++;
    __int16 newScaleX = (__int16)((float)scaleX / GetCurrentAspectMultiplier());
    int res = Orig_HUD_DrawSpriteScaled(spriteId, x, y, newScaleX, scaleY, flags);
    g_2DPrimitiveDepth--;
    return res;
}

typedef void* (__cdecl *HUD_DrawRotatedSprite_fn)(int spriteId, int a2, int x, int y, int scaleX, int scaleY, int a7, __int16 angle);
static HUD_DrawRotatedSprite_fn Orig_HUD_DrawRotatedSprite = NULL;

void* __cdecl Hooked_HUD_DrawRotatedSprite(int spriteId, int a2, int x, int y, int scaleX, int scaleY, int a7, __int16 angle) {
    g_2DPrimitiveDepth++;
    int newScaleX = (int)((float)scaleX / GetCurrentAspectMultiplier());
    void* res = Orig_HUD_DrawRotatedSprite(spriteId, a2, x, y, newScaleX, scaleY, a7, angle);
    g_2DPrimitiveDepth--;
    return res;
}

typedef void* (__cdecl *HUD_UpdateMinimapPos_fn)(int racerSlot, int posX, int posY);
static HUD_UpdateMinimapPos_fn Orig_HUD_UpdateMinimapPos = NULL;

void* __cdecl Hooked_HUD_UpdateMinimapPos(int racerSlot, int posX, int posY) {
    void* res = Orig_HUD_UpdateMinimapPos(racerSlot, posX, posY);
    if (res && racerSlot >= 0) {
        int* pX = (int*)((char*)res + 96 * racerSlot + 0x3C);
        *pX = (int)((float)*pX / GetCurrentAspectMultiplier());
    }
    return res;
}




typedef char (__cdecl *Racer_ClampSpeed_fn)(void* pRacer, void* pEntity);
static Racer_ClampSpeed_fn Orig_Racer_ClampSpeed = NULL;

typedef int (__cdecl *Racer_GetLapDistance_fn)(void* pRacer);
static const Racer_GetLapDistance_fn Call_Racer_GetLapDistance = (Racer_GetLapDistance_fn)0x00403D10;

typedef int (__cdecl *Race_GetNumPlayers_fn)();
static const Race_GetNumPlayers_fn Call_Race_GetNumPlayers = (Race_GetNumPlayers_fn)0x00429560;

typedef void* (__cdecl *Racer_GetByIndex_fn)(int idx);
static const Racer_GetByIndex_fn Call_Racer_GetByIndex = (Racer_GetByIndex_fn)0x00445ED0;

typedef void (__cdecl *Racer_GiveBoostItem_fn)(void* pRacer);
static const Racer_GiveBoostItem_fn Call_Racer_GiveBoostItem = (Racer_GiveBoostItem_fn)0x00444630;

typedef int (__cdecl *Racer_GiveItem_fn)(void* pRacer, int itemId);
static const Racer_GiveItem_fn Call_Racer_GiveItem = (Racer_GiveItem_fn)0x004403E0;

typedef int (__cdecl *Racer_FireWeapon_fn)(void* pWeaponCtrl, void* pItem);
static const Racer_FireWeapon_fn Call_Racer_FireWeapon = (Racer_FireWeapon_fn)0x00405930;

typedef int (__cdecl *Race_IsPausedOrOver_fn)();
static const Race_IsPausedOrOver_fn Call_Race_IsPausedOrOver = (Race_IsPausedOrOver_fn)0x0042B9F0;

typedef int (__cdecl *Race_IsStarted_fn)();
static const Race_IsStarted_fn Call_Race_IsStarted = (Race_IsStarted_fn)0x00401230;


static DWORD g_LastPlayerWeaponFireTick = 0;


typedef int (__cdecl *Racer_AI_ComputeSteering_fn)(void* pRacer, int arg_4);
static Racer_AI_ComputeSteering_fn Orig_Racer_AI_ComputeSteering = NULL;


typedef int (__cdecl *Racer_IntegrateForces_fn)(void* pRacer, void* pEntity);
static Racer_IntegrateForces_fn Orig_Racer_IntegrateForces = NULL;


typedef int (__cdecl *Racer_HandleWallCollision_fn)(void* pRacer, void* pEntity, void* pColInfo);
static Racer_HandleWallCollision_fn Orig_Racer_HandleWallCollision = NULL;


typedef int16_t (__cdecl *Racer_OnHit_fn)(void* pRacer, void* pEntity, int msgId);
static Racer_OnHit_fn Orig_Racer_OnHit = NULL;


typedef int (__cdecl *Racer_FireWeapon_fn)(void* pWeaponCtrl, void* pItem);
static Racer_FireWeapon_fn Orig_Racer_FireWeapon = NULL;

static int  g_CurrentHeatLevel        = 0;
static char s_NemesisStatusLine[64]   = "nemesis: idle";
static char s_NemesisLastDropLine[64] = "last drop: none";
static DWORD s_GreenLightTick          = 0;
static DWORD s_LeadingStartTick        = 0;
static DWORD s_LastWeaponGrantTick     = 0;

const char* GetWeaponName(int wpnId) {
    switch (wpnId) {
        case 0: return "Green Acorn";
        case 1: return "Homing Acorn";
        case 2: return "Teacup Mine";
        case 3: return "Frog Spell";
        case 4: return "Turbo Boost";
        case 5: return "Guided Rocket";
        case 6: return "Shield";
        default: return "Unknown";
    }
}




int __cdecl Hooked_Racer_AI_ComputeSteering(void* pRacer, int arg_4) {
    int res = Orig_Racer_AI_ComputeSteering(pRacer, arg_4);

    if (pRacer && (g_NemesisAI & 1) && g_CurrentHeatLevel >= 3) {
        int racerIdx = *(int*)pRacer;
        int numPlayers = Call_Race_GetNumPlayers();

        if (racerIdx >= numPlayers) {
            
            char* pPhysics = (char*)0x0048D240 + (332 * racerIdx);

            
            int maxSpeed = *(int*)((char*)pRacer + 0x5C);
            *(int*)(pPhysics + 8) = maxSpeed;

            
            int16_t* pSteer = (int16_t*)(pPhysics + 0x148);
            if (*pSteer > 25)  *pSteer = 127;
            if (*pSteer < -25) *pSteer = -127;
        }
    }
    return res;
}




int __cdecl Hooked_Racer_IntegrateForces(void* pRacer, void* pEntity) {
    int res = Orig_Racer_IntegrateForces(pRacer, pEntity);

    if (pRacer && (g_NemesisAI & 1) && g_CurrentHeatLevel >= 3) {
        int racerIdx = *(int*)pRacer;
        int numPlayers = Call_Race_GetNumPlayers();

        if (racerIdx >= numPlayers) {
            
            *(int16_t*)((char*)pRacer + 0x90) = 0; 
            *(int16_t*)((char*)pRacer + 0x92) = 0; 
            *(int16_t*)((char*)pRacer + 0x74) = 0; 
            *(int16_t*)((char*)pRacer + 0x78) = 0; 

            
            int32_t vx = *(int32_t*)((char*)pRacer + 0x14);
            int32_t vy = *(int32_t*)((char*)pRacer + 0x18);
            int32_t vz = *(int32_t*)((char*)pRacer + 0x1C);

            
            int16_t fwdX = *(int16_t*)((char*)pRacer + 0x40);
            int16_t fwdY = *(int16_t*)((char*)pRacer + 0x42);
            int16_t fwdZ = *(int16_t*)((char*)pRacer + 0x44);

            
            double currentSpeed = sqrt((double)vx * vx + (double)vy * vy + (double)vz * vz);

            
            *(int32_t*)((char*)pRacer + 0x14) = (int32_t)((currentSpeed * fwdX) / 4096.0);
            *(int32_t*)((char*)pRacer + 0x18) = (int32_t)((currentSpeed * fwdY) / 4096.0);
            *(int32_t*)((char*)pRacer + 0x1C) = (int32_t)((currentSpeed * fwdZ) / 4096.0);
        }
    }
    return res;
}




int __cdecl Hooked_Racer_HandleWallCollision(void* pRacer, void* pEntity, void* pColInfo) {
    if (pRacer && (g_NemesisAI & 1) && g_CurrentHeatLevel >= 3) {
        int racerIdx = *(int*)pRacer;
        int numPlayers = Call_Race_GetNumPlayers();

        if (racerIdx >= numPlayers) {
            
            if (pColInfo && pEntity) {
                int16_t* pPushDelta = (int16_t*)((char*)pColInfo + 0x54);
                *(int16_t*)((char*)pEntity + 0x58) += pPushDelta[0];
                *(int16_t*)((char*)pEntity + 0x5A) += pPushDelta[1];
                *(int16_t*)((char*)pEntity + 0x5C) += pPushDelta[2];
            }
            return 0; 
        }
    }
    return Orig_Racer_HandleWallCollision(pRacer, pEntity, pColInfo);
}




int __cdecl Hooked_Racer_FireWeapon(void* pWeaponCtrl, void* pItem) {
    void* pPlayer = Call_Racer_GetByIndex(0);
    if (pPlayer && pWeaponCtrl) {
        void* pPlayerWpnCtrl = *(void**)((char*)pPlayer + 0xF4);
        if (pWeaponCtrl == pPlayerWpnCtrl) {
            g_LastPlayerWeaponFireTick = GetTickCount(); 
        }
    }
    return Orig_Racer_FireWeapon(pWeaponCtrl, pItem);
}




int16_t __cdecl Hooked_Racer_OnHit(void* pRacer, void* pEntity, int msgId) {
    if (pRacer && (g_NemesisAI & 1)) {
        int victimIdx = *(int*)pRacer;
        int numPlayers = Call_Race_GetNumPlayers();

        
        if (victimIdx >= numPlayers && msgId == 0x7D7) {
            DWORD elapsedSincePlayerShot = GetTickCount() - g_LastPlayerWeaponFireTick;

            
            if (elapsedSincePlayerShot > 3500) {
                return 0; 
            }
        }
    }
    return Orig_Racer_OnHit(pRacer, pEntity, msgId);
}

char __cdecl Hooked_Racer_ClampSpeed(void* pRacer, void* pEntity) {
    if ((g_RubberbandAI & 1) && pRacer) {
        int racerIdx   = *(int*)pRacer;
        int numPlayers = Call_Race_GetNumPlayers();

        if (racerIdx >= numPlayers) {
            void* pAhead = *(void**)((char*)pRacer + 0xD4);

            if (pAhead) {
                int distThis  = Call_Racer_GetLapDistance(pRacer);
                int distAhead = Call_Racer_GetLapDistance(pAhead);
                int gap       = distAhead - distThis;

                if (gap > 200) {
                    int speedBonus = (gap - 200) / 90;

                    if (g_NemesisAI & 1) {
                        if (g_CurrentHeatLevel >= 3) {
                            speedBonus += 35;
                        } else {
                            speedBonus += g_CurrentHeatLevel * 10;
                        }
                    }

                    int maxCap = 50 + (g_CurrentHeatLevel * 20);
                    if (speedBonus > maxCap) speedBonus = maxCap;

                    int* pMaxSpeed = (int*)((char*)pRacer + 0x5C);
                    *pMaxSpeed += speedBonus;
                }
            }
        }
    }

    return Orig_Racer_ClampSpeed(pRacer, pEntity);
}




void ForceBotAttack(void* pRacer, int heat) {
    if (!pRacer) return;
    int numPlayers = Call_Race_GetNumPlayers();
    if (*(int*)pRacer < numPlayers) return;

    int slot = *(int*)pRacer;
    void* pWeaponCtrl = *(void**)((char*)pRacer + 0xF4);
    if (!pWeaponCtrl) return;

    void* pPlayer = Call_Racer_GetByIndex(0);
    if (!pPlayer) return;

    
    void* pOriginalAhead = *(void**)((char*)pRacer + 0xD4);
    *(void**)((char*)pRacer + 0xD4) = pPlayer;

    int weaponToGive = 1;
    if (heat == 1) {
        weaponToGive = (rand() % 2 == 0) ? 1 : 5; 
    } else if (heat == 2) {
        int roll = rand() % 3;
        if (roll == 0) weaponToGive = 3;      
        else if (roll == 1) weaponToGive = 5; 
        else weaponToGive = 1;                
    } else if (heat >= 3) {
        int roll = rand() % 100;
        if (roll < 60)      weaponToGive = 3; 
        else if (roll < 85) weaponToGive = 5; 
        else                weaponToGive = 1; 

        Call_Racer_GiveBoostItem(pRacer);
    }

    Call_Racer_GiveItem(pRacer, weaponToGive);
    Orig_Racer_FireWeapon(pWeaponCtrl, 0);

    
    *(void**)((char*)pRacer + 0xD4) = pOriginalAhead;

    sprintf_s(s_NemesisLastDropLine, "drop: %s -> bot #%d (target P1)", GetWeaponName(weaponToGive), slot);
}

void UpdateNemesisAI() {
    if (!(g_NemesisAI & 1)) {
        g_CurrentHeatLevel = 0;
        sprintf_s(s_NemesisStatusLine, "nemesis: disabled in menu");
        return;
    }

    
    if (Call_Race_IsPausedOrOver() || !Call_Race_IsStarted()) {
        g_CurrentHeatLevel = 0;
        s_GreenLightTick   = 0; 
        s_LeadingStartTick = 0;
        sprintf_s(s_NemesisStatusLine, "nemesis: waiting for green");
        return;
    }

    void* pPlayer = Call_Racer_GetByIndex(0);
    if (!pPlayer) return;

    DWORD now = GetTickCount();
    if (s_GreenLightTick == 0) {
        s_GreenLightTick = now;
    }

    int lapIndex   = *(int*)((char*)pPlayer + 0xC8);
    int playerRank = *(int*)((char*)pPlayer + 0xD0);
    int playerDist = Call_Racer_GetLapDistance(pPlayer);

    
    if (lapIndex == 0) {
        bool timeSafe = (now - s_GreenLightTick < 3500);
        bool distSafe = (playerDist < 200);

        if (timeSafe && distSafe) {
            g_CurrentHeatLevel = 0;
            sprintf_s(s_NemesisStatusLine, "nemesis: grid safe (d:%d/200)", playerDist);
            return;
        }
    }

    int currentLap = lapIndex + 1;
    int totalLaps  = 3;
    if (currentLap > totalLaps) currentLap = totalLaps;

    if (playerRank == 1) {
        if (s_LeadingStartTick == 0) {
            s_LeadingStartTick    = now;
            s_LastWeaponGrantTick = now;
        }

        DWORD leadDuration = now - s_LeadingStartTick;
        int leadSec = (int)(leadDuration / 1000);

        int heat = 0;
        if (leadDuration > 4000)  heat = 1;
        if (leadDuration > 10000) heat = 2;
        if (lapIndex >= 2)        heat = 3;

        g_CurrentHeatLevel = heat;

        sprintf_s(s_NemesisStatusLine, "heat: %d | L%d/%d | lead: %ds | d:%d", 
                  heat, currentLap, totalLaps, leadSec, playerDist);

        DWORD grantInterval = (heat >= 3) ? 1800 : ((heat >= 2) ? 3200 : 4500);

        if (heat >= 1 && (now - s_LastWeaponGrantTick > grantInterval)) {
            s_LastWeaponGrantTick = now;

            void* pChaser2 = *(void**)((char*)pPlayer + 0xD8);
            if (pChaser2) {
                ForceBotAttack(pChaser2, heat);

                if (heat >= 2) {
                    void* pChaser3 = *(void**)((char*)pChaser2 + 0xD8);
                    if (pChaser3) {
                        ForceBotAttack(pChaser3, heat);
                    }
                }
            }
        }
    } else {
        s_LeadingStartTick = 0;
        g_CurrentHeatLevel = 0;
        sprintf_s(s_NemesisStatusLine, "heat: 0 (player rank #%d | d:%d)", playerRank, playerDist);
    }
}




typedef int (__cdecl *Menu_GetNavigationInput_fn)(void* pMenu);
static Menu_GetNavigationInput_fn Orig_Menu_GetNavigationInput = NULL;

typedef void (__cdecl *Menu_HandleCancelAction_fn)(void* pMenu, int a2, int a3);
static const Menu_HandleCancelAction_fn Call_Menu_HandleCancelAction = 
    (Menu_HandleCancelAction_fn)0x004334E0;

typedef void (__cdecl *Race_TriggerPauseMenu_fn)(void* pApp, int a2, int a3);
static const Race_TriggerPauseMenu_fn Call_Race_TriggerPauseMenu = 
    (Race_TriggerPauseMenu_fn)0x00417A50;

int __cdecl Hooked_Menu_GetNavigationInput(void* pMenu) {
    int nav = Orig_Menu_GetNavigationInput(pMenu);

    if (Real_XInputGetState) {
        XINPUT_STATE xstate;
        if (Real_XInputGetState(0, &xstate) == ERROR_SUCCESS) {
            WORD wButtons = xstate.Gamepad.wButtons;

            static WORD  s_LastMenuBtns = 0;
            static DWORD s_NavRepeatTick = 0;
            DWORD now = GetTickCount();

            WORD pressed = wButtons & ~s_LastMenuBtns;
            bool isRepeat = false;

            bool isNav = (wButtons & (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN |
                                     XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT)) ||
                         (abs(xstate.Gamepad.sThumbLX) > 16000) ||
                         (abs(xstate.Gamepad.sThumbLY) > 16000);

            if (isNav) {
                if (pressed || s_LastMenuBtns == 0) {
                    s_NavRepeatTick = now + 350;
                } else if (now > s_NavRepeatTick) {
                    isRepeat = true;
                    s_NavRepeatTick = now + 140;
                }
            }
            s_LastMenuBtns = wButtons;

            if (nav <= 1) {
                if ((pressed & XINPUT_GAMEPAD_A) || (pressed & XINPUT_GAMEPAD_START)) {
                    return 7;
                }
                if ((wButtons & XINPUT_GAMEPAD_DPAD_UP) || (xstate.Gamepad.sThumbLY > 16000)) {
                    if (pressed || isRepeat) return 2;
                }
                if ((wButtons & XINPUT_GAMEPAD_DPAD_DOWN) || (xstate.Gamepad.sThumbLY < -16000)) {
                    if (pressed || isRepeat) return 3;
                }
                if ((wButtons & XINPUT_GAMEPAD_DPAD_LEFT) || (xstate.Gamepad.sThumbLX < -16000)) {
                    if (pressed || isRepeat) return 4;
                }
                if ((wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) || (xstate.Gamepad.sThumbLX > 16000)) {
                    if (pressed || isRepeat) return 5;
                }
            }

            if (pressed & XINPUT_GAMEPAD_B) {
                Call_Menu_HandleCancelAction(pMenu, 0, 0);
            }
        }
    }

    return nav;
}




typedef int (__cdecl *Camera_ProcessFlybyStep_fn)(void* pCamera, int arg_4);
static Camera_ProcessFlybyStep_fn Orig_Camera_ProcessFlybyStep = NULL;

typedef int (__cdecl *Camera_SkipFlybyToEnd_fn)(void* pCamera, int arg_4);
static const Camera_SkipFlybyToEnd_fn Call_Camera_SkipFlybyToEnd = 
    (Camera_SkipFlybyToEnd_fn)0x00407510;

int __cdecl Hooked_Camera_ProcessFlybyStep(void* pCamera, int arg_4) {
    if (pCamera) {
        bool skipRequested = (g_AutoSkipFlyby != 0);

        if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) 
        {
            skipRequested = true;
        }

        if (Real_XInputGetState) {
            XINPUT_STATE xs;
            if (Real_XInputGetState(0, &xs) == ERROR_SUCCESS) {
                if (xs.Gamepad.wButtons & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_START | 
                                           XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X)) 
                {
                    skipRequested = true;
                }
            }
        }

        if (skipRequested) {
            Call_Camera_SkipFlybyToEnd(pCamera, arg_4);
            return 0;
        }
    }

    return Orig_Camera_ProcessFlybyStep(pCamera, arg_4);
}




static uint32_t* const p_g_raceMode       = (uint32_t*)0x004663C0;
static uint32_t* const p_g_unlockedChars  = (uint32_t*)0x00480550;
static uint32_t* const p_g_debugFlag      = (uint32_t*)0x0048B854;
static const void*     pFn_EngineCheatAction = (void*)0x00417D50;

static void**    g_ppMenuOptions          = NULL;
static uint32_t* g_pMenuSelectedOption    = NULL;
static void*     g_pCurrentState          = NULL;
static void*     g_pGameGlobals           = NULL; 

#pragma pack(push, 4)
struct WDW_MenuItem {
    int         type;       
    int         minVal;
    int         maxVal;
    const char* name;
    void*       pTarget;
    int         mask;
};
#pragma pack(pop)

void __cdecl Action_ResumeGame() {
    if (g_pCurrentState) {
        *(uint16_t*)((char*)g_pCurrentState + 0x90A4) = 0;
    }
}

void __cdecl Action_UnlockAll() {
    if (p_g_unlockedChars) {
        *p_g_unlockedChars |= 7;
    }
}

static WDW_MenuItem g_CustomCheatMenu[] = {
    
    { 2, 0, 0, "resume race",         (void*)Action_ResumeGame,      0 },
    { 2, 0, 0, "win (instant 1st)",   (void*)pFn_EngineCheatAction,   0 },
    { 2, 0, 0, "restart race",        (void*)pFn_EngineCheatAction,   0 },

    
    { 0, 0, 0, "pgxp subpixel",       (void*)&g_PGXPEnabled,         1 },
    { 0, 0, 0, "widescreen fix",      (void*)&g_WidescreenEnabled,   1 },
    { 0, 0, 0, "debug stats overlay", (void*)&g_ShowDebugOverlay,    1 },

    
    { 0, 0, 0, "ai catch-up boost",   (void*)&g_RubberbandAI,        1 },
    { 0, 0, 0, "nemesis ai mode",     (void*)&g_NemesisAI,           1 },

    
    { 2, 0, 0, s_NemesisStatusLine,   (void*)Action_ResumeGame,      0 },
    { 2, 0, 0, s_NemesisLastDropLine, (void*)Action_ResumeGame,      0 },

    
    { 2, 0, 0, "toggle camera",       (void*)pFn_EngineCheatAction,   0 },
    { 2, 0, 0, "follow racer",        (void*)pFn_EngineCheatAction,   0 },

    
    { 2, 0, 0, "unlock all racers",   (void*)Action_UnlockAll,       0 },
    { 0, 0, 0, "jiminy cricket",      (void*)p_g_unlockedChars,      1 },
    { 0, 0, 0, "ned shredbetter",     (void*)p_g_unlockedChars,      2 },
    { 0, 0, 0, "xud 71 robot",        (void*)p_g_unlockedChars,      4 },

    
    { 3, 0, 0, "select track...",     (void*)0x00465AC8,             0 },

    
    { 4, 0, 0, NULL,                  NULL,                          0 }
};

typedef int (__cdecl *Race_GetRacerCount_fn)();
static const Race_GetRacerCount_fn Call_Race_GetRacerCount = (Race_GetRacerCount_fn)0x00429550;

inline bool IsInActiveRace(void* pApp) {
    if (!pApp) return false;

    if (Call_Race_IsPausedOrOver && Call_Race_IsPausedOrOver() != 0) {
        return false;
    }

    if (Call_Race_GetRacerCount && Call_Race_GetRacerCount() > 0) {
        uint16_t pauseState = *(uint16_t*)((char*)pApp + 0x90A4);
        if (pauseState == 0 && !g_FreecamActive) {
            return true;
        }
    }
    return false;
}

void OpenCustomCheatMenu(void* pState) {
    if (!g_ppMenuOptions) return;

    g_pCurrentState = pState;
    *g_ppMenuOptions = &g_CustomCheatMenu[0];

    if (g_pMenuSelectedOption) {
        *g_pMenuSelectedOption = 0;
    }
    if (p_g_debugFlag) {
        *p_g_debugFlag |= 0x0800;
    }

    *(uint16_t*)((char*)pState + 0x90A4) = 4;
}

void ToggleFreecamMode(void* pState) {
    g_FreecamActive = !g_FreecamActive;
    uint16_t* pPauseState = (uint16_t*)((char*)pState + 0x90A4);

    s_MouseTrackingInit = false;

    if (g_FreecamActive) {
        *pPauseState = 4;
        ShowCursor(FALSE);

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        SetCursorPos(screenW / 2, screenH / 2);
    } else {
        *pPauseState = 0;
        ShowCursor(TRUE);
    }
}




typedef char (__cdecl *Input_ReadKeyboard_fn)(int pApp);
static Input_ReadKeyboard_fn Orig_Input_ReadKeyboard = NULL;

char __cdecl Hooked_Input_ReadKeyboard(int pApp) {
    char res = Orig_Input_ReadKeyboard(pApp);

    if (pApp) {
        g_pGameGlobals = (void*)pApp;
    }

    if (Real_XInputGetState && pApp) {
        XINPUT_STATE xstate;
        ZeroMemory(&xstate, sizeof(XINPUT_STATE));

        if (Real_XInputGetState(0, &xstate) == ERROR_SUCCESS) {
            WORD wButtons = xstate.Gamepad.wButtons;

            static WORD  s_LastKbdBtns   = 0;
            static DWORD s_KbdRepeatTick = 0;
            static DWORD s_LastPauseTick = 0;
            DWORD now = GetTickCount();

            WORD pressed  = wButtons & ~s_LastKbdBtns;
            bool isRepeat = false;

            bool isNav = (wButtons & (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | 
                                     XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT)) ||
                         (abs(xstate.Gamepad.sThumbLX) > 16000) ||
                         (abs(xstate.Gamepad.sThumbLY) > 16000);

            if (isNav) {
                if (pressed || s_LastKbdBtns == 0) {
                    s_KbdRepeatTick = now + 350;
                } else if (now > s_KbdRepeatTick) {
                    isRepeat = true;
                    s_KbdRepeatTick = now + 140;
                }
            }
            s_LastKbdBtns = wButtons;

            bool backHeld = (wButtons & XINPUT_GAMEPAD_BACK) != 0;
            uint16_t* pPauseState = (uint16_t*)((char*)pApp + 0x90A4);

            if (backHeld) {
                if (pressed & (XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_A)) {
                    if (*pPauseState == 4 && !g_FreecamActive) {
                        *pPauseState = 0;
                    } else if (*pPauseState == 0) {
                        OpenCustomCheatMenu((void*)pApp);
                    }
                }

                if (pressed & (XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y)) {
                    ToggleFreecamMode((void*)pApp);
                }

                return res;
            }

            if ((wButtons & XINPUT_GAMEPAD_LEFT_THUMB) && (pressed & XINPUT_GAMEPAD_RIGHT_THUMB)) {
                ToggleFreecamMode((void*)pApp);
                return res;
            }

            bool inRace = IsInActiveRace((void*)pApp);

            if (!inRace) {
                uint32_t* pMasterHeld = (uint32_t*)((char*)pApp + 0x8A84);
                uint32_t* pMasterEdge = (uint32_t*)((char*)pApp + 0x8A88);
                uint32_t* pPlayerHeld = (uint32_t*)((char*)pApp + 0x8A5C);
                uint32_t* pPlayerEdge = (uint32_t*)((char*)pApp + 0x8A60);

                uint8_t* pKbdBuffer = NULL;
                uint32_t* pKbdIdxPtr = *(uint32_t**)(0x00412E40 + 2);
                uintptr_t kbdTableBase = *(uintptr_t*)(0x00412E5C + 3);

                if (pKbdIdxPtr && kbdTableBase) {
                    uint32_t slot = (*pKbdIdxPtr) * 11;
                    pKbdBuffer = *(uint8_t**)(kbdTableBase + slot * 8);
                }

                if ((wButtons & XINPUT_GAMEPAD_DPAD_LEFT) || (xstate.Gamepad.sThumbLX < -16000)) {
                    *pMasterHeld |= 0x0004;
                    *pPlayerHeld |= 0x0004;
                    if (pressed || isRepeat) {
                        *pMasterEdge |= 0x0004;
                        *pPlayerEdge |= 0x0004;
                        if (pKbdBuffer) pKbdBuffer[0xCB] = 0x80;
                    }
                }

                if ((wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) || (xstate.Gamepad.sThumbLX > 16000)) {
                    *pMasterHeld |= 0x0008;
                    *pPlayerHeld |= 0x0008;
                    if (pressed || isRepeat) {
                        *pMasterEdge |= 0x0008;
                        *pPlayerEdge |= 0x0008;
                        if (pKbdBuffer) pKbdBuffer[0xCD] = 0x80;
                    }
                }

                if ((wButtons & XINPUT_GAMEPAD_DPAD_UP) || (xstate.Gamepad.sThumbLY > 16000)) {
                    *pMasterHeld |= 0x0001;
                    *pPlayerHeld |= 0x0001;
                    if (pressed || isRepeat) {
                        *pMasterEdge |= 0x0001;
                        *pPlayerEdge |= 0x0001;
                        if (pKbdBuffer) pKbdBuffer[0xC8] = 0x80;
                    }
                }

                if ((wButtons & XINPUT_GAMEPAD_DPAD_DOWN) || (xstate.Gamepad.sThumbLY < -16000)) {
                    *pMasterHeld |= 0x0002;
                    *pPlayerHeld |= 0x0002;
                    if (pressed || isRepeat) {
                        *pMasterEdge |= 0x0002;
                        *pPlayerEdge |= 0x0002;
                        if (pKbdBuffer) pKbdBuffer[0xD0] = 0x80;
                    }
                }

                if (wButtons & XINPUT_GAMEPAD_A) {
                    *pMasterHeld |= 0x0080;
                    *pPlayerHeld |= 0x0080;
                    if (pressed & XINPUT_GAMEPAD_A) {
                        *pMasterEdge |= 0x0080;
                        *pPlayerEdge |= 0x0080;
                    }
                    if (pKbdBuffer) {
                        pKbdBuffer[0x1C] = 0x80;
                        pKbdBuffer[0x39] = 0x80;
                    }
                }

                if (wButtons & XINPUT_GAMEPAD_B) {
                    *pMasterHeld |= 0x4000;
                    *pPlayerHeld |= 0x0010;
                    if (pressed & XINPUT_GAMEPAD_B) {
                        *pMasterEdge |= 0x4000;
                        *pPlayerEdge |= 0x0010;
                    }
                    if (pKbdBuffer) pKbdBuffer[0x01] = 0x80;
                }

                if ((pressed & XINPUT_GAMEPAD_START) && (now - s_LastPauseTick > 400)) {
                    s_LastPauseTick = now;
                    *pMasterHeld |= 0x4000;
                }
            } else {
                if ((pressed & XINPUT_GAMEPAD_START) && (now - s_LastPauseTick > 400)) {
                    s_LastPauseTick = now;
                    uint32_t* pMasterHeld = (uint32_t*)((char*)pApp + 0x8A84);
                    *pMasterHeld |= 0x4000;
                }
            }
        }
    }

    return res;
}




typedef int (__cdecl *Input_ApplyPlayerBindings_fn)(int playerIndex, int pApp);
static Input_ApplyPlayerBindings_fn Orig_Input_ApplyPlayerBindings = NULL;

int __cdecl Hooked_Input_ApplyPlayerBindings(int playerIndex, int pApp) {
    int res = Orig_Input_ApplyPlayerBindings(playerIndex, pApp);

    if (playerIndex == 0 && Real_XInputGetState && pApp) {
        g_pGameGlobals = (void*)pApp;
        XINPUT_STATE xstate;
        ZeroMemory(&xstate, sizeof(XINPUT_STATE));

        if (Real_XInputGetState(0, &xstate) == ERROR_SUCCESS) {
            uint32_t btnHop   = **(uint32_t**)(0x00441478 + 2);
            uint32_t btnAccel = **(uint32_t**)(0x004414F9 + 2);
            uint32_t btnBrake = **(uint32_t**)(0x00441515 + 1);
            uint32_t btnFire  = **(uint32_t**)(0x00441763 + 2);
            uint32_t btnLook  = **(uint32_t**)(0x00441823 + 2);

            uint32_t* pPlayerHeld = (uint32_t*)((char*)pApp + 0x8A5C);
            int32_t*  pAnalogX    = (int32_t*)((char*)pApp + 0x8A68);
            int32_t*  pAnalogY    = (int32_t*)((char*)pApp + 0x8A6C);
            WORD curButtons       = xstate.Gamepad.wButtons;

            int rawThumbX = xstate.Gamepad.sThumbLX;
            int deadzone  = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
            float normX   = 0.0f;

            if (abs(rawThumbX) > deadzone) {
                float val = (float)(rawThumbX - (rawThumbX > 0 ? deadzone : -deadzone));
                normX = val / (32767.0f - (float)deadzone);
            }

            if (normX < -1.0f) normX = -1.0f;
            if (normX >  1.0f) normX =  1.0f;
            *pAnalogX = (int32_t)(normX * 127.0f);

            if (xstate.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
                *pPlayerHeld |= btnAccel;
            }

            if (xstate.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
                *pPlayerHeld |= btnBrake;
                *pAnalogY = 127;
            }

            if ((curButtons & XINPUT_GAMEPAD_A) || (curButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
                *pPlayerHeld |= btnHop;
            }

            if ((curButtons & XINPUT_GAMEPAD_X) || (curButtons & XINPUT_GAMEPAD_B)) {
                *pPlayerHeld |= btnFire;
            }

            if ((curButtons & XINPUT_GAMEPAD_Y) || (curButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)) {
                *pPlayerHeld |= btnLook;
            }
        }
    }

    return res;
}




void UpdateDebugOverlayPatch(bool force = false) {
    static int s_LastOverlayState = -1;
    int cur = (g_ShowDebugOverlay & 1);

    if (force || cur != s_LastOverlayState) {
        s_LastOverlayState = cur;
        if (cur) {
            uint8_t nops2[2] = { 0x90, 0x90 };
            PatchMemory(0x00420DE1, nops2, 2);
            PatchMemory(0x00420DEA, nops2, 2);
        } else {
            uint8_t jnz1[2] = { 0x75, 0x12 };
            uint8_t jnz2[2] = { 0x75, 0x09 };
            PatchMemory(0x00420DE1, jnz1, 2);
            PatchMemory(0x00420DEA, jnz2, 2);
        }
    }
}




typedef void (__cdecl *HUD_ProcessPauseMenuInput_fn)(void* pState);
static HUD_ProcessPauseMenuInput_fn Orig_HUD_ProcessPauseMenuInput = NULL;

typedef int (__cdecl *Menu_DrawAndHandleDebugMenu_fn)(void* pState);
static const Menu_DrawAndHandleDebugMenu_fn Orig_Menu_DrawAndHandleDebugMenu = 
    (Menu_DrawAndHandleDebugMenu_fn)0x00417430;

int __cdecl Hooked_Menu_DrawAndHandleDebugMenu(void* pState) {
    if (g_FreecamActive) {
        return 0;
    }
    return Orig_Menu_DrawAndHandleDebugMenu(pState);
}

void __cdecl Hooked_HUD_ProcessPauseMenuInput(void* pState) {
    if (!pState) return;
    g_pCurrentState = pState;
    g_pGameGlobals  = pState;

    static uint32_t s_LastWidescreenState = 1;
    uint32_t curWs = (g_WidescreenEnabled & 1);
    if (curWs != s_LastWidescreenState) {
        s_LastWidescreenState = curWs;
        uint32_t newIrisFactor = (uint32_t)(0x66666667ULL / GetCurrentAspectMultiplier());
        PatchMemory(0x0041ACB7, &newIrisFactor, 4);
    }

    UpdateDebugOverlayPatch(false);

    uint16_t* pPauseState = (uint16_t*)((char*)pState + 0x90A4);

    if (*pPauseState == 0 && !g_FreecamActive) {
        UpdateNemesisAI();
    }

    bool triggerFreecam = false;
    bool triggerDebugMenu = false;

    if (Real_XInputGetState) {
        XINPUT_STATE xstate;
        if (Real_XInputGetState(0, &xstate) == ERROR_SUCCESS) {
            WORD w = xstate.Gamepad.wButtons;

            static WORD s_LastComboBtns = 0;
            WORD pressed = w & ~s_LastComboBtns;
            s_LastComboBtns = w;

            if ((w & XINPUT_GAMEPAD_BACK) && (pressed & XINPUT_GAMEPAD_START)) {
                triggerDebugMenu = true;
            }

            if ((w & XINPUT_GAMEPAD_BACK) && (pressed & XINPUT_GAMEPAD_Y)) {
                triggerFreecam = true;
            }
            if ((w & XINPUT_GAMEPAD_LEFT_THUMB) && (pressed & XINPUT_GAMEPAD_RIGHT_THUMB)) {
                triggerFreecam = true;
            }
        }
    }

    static bool s_FreecamKeyDown = false;
    if ((GetAsyncKeyState(g_FreecamHotkey) & 0x8000) != 0) {
        if (!s_FreecamKeyDown) {
            s_FreecamKeyDown = true;
            triggerFreecam = true;
        }
    } else {
        s_FreecamKeyDown = false;
    }

    if (triggerFreecam) {
        ToggleFreecamMode(pState);
    }

    if (g_FreecamActive) {
        *pPauseState = 4;
    }

    static bool s_HotkeyDown = false;
    if ((GetAsyncKeyState(g_DebugMenuHotkey) & 0x8000) != 0) {
        if (!s_HotkeyDown) {
            s_HotkeyDown = true;
            triggerDebugMenu = true;
        }
    } else {
        s_HotkeyDown = false;
    }

    if (g_EnableDebugMenu && !g_FreecamActive && triggerDebugMenu) {
        if (*pPauseState == 4) {
            *pPauseState = 0;
        } else {
            OpenCustomCheatMenu(pState);
        }
    }

    Orig_HUD_ProcessPauseMenuInput(pState);
}




int __cdecl Hooked_Camera_GetDefaultAspect() {
    return (int)(4096.0f / (GetCurrentAspectMultiplier() * 1.05f));
}

int __cdecl Hooked_D3D_FindDisplayMode(int devIdx, int width, int height, int bpp) {
    uint32_t idx = devIdx * 48;
    uint32_t* pModes = *(uint32_t**)(0x00470514 + idx * 4);
    if (pModes) {
        pModes[0] = g_TargetWidth;
        pModes[1] = g_TargetHeight;
        pModes[2] = 16;
    }
    return 0;
}

int __cdecl Hooked_Sys_DetectCDRom() {
    uintptr_t pDestination = *(uintptr_t*)(0x004314D3 + 1);
    if (pDestination) {
        strcpy_s((char*)pDestination, 4, ".\\");
    }
    return 1;
}

int __cdecl Hooked_Sys_CheckSystemRequirements(int* pErrorCount) {
    if (pErrorCount) {
        *pErrorCount = 0;
    }
    return 0;
}

typedef int (__cdecl *Win_ProcessMessages_fn)();
static const Win_ProcessMessages_fn Call_Win_ProcessMessages = (Win_ProcessMessages_fn)0x00430040;

void __cdecl Hooked_Win_WaitAndProcessMessages() {
    Call_Win_ProcessMessages();

    HWND hWnd = *(HWND*)0x004806D0;
    if (hWnd) {
        bool isMinimized = (IsIconic(hWnd) != 0);
        bool lostFocus   = (GetForegroundWindow() != hWnd);

        if ((isMinimized || lostFocus) && IsInActiveRace(g_pGameGlobals)) {
            if (g_pGameGlobals) {
                *(uint32_t*)((char*)g_pGameGlobals + 0x8A84) |= 0x4000;
            }
        }

        if (isMinimized) {
            Sleep(10);
        }
    }
}

static const char** const p_off_baseDir   = (const char**)0x004663C8;
static const char** const p_off_levelName = (const char**)0x004663CC;
static const char** const p_off_selectDir  = (const char**)0x004663D8;
static const char** const p_off_selectName = (const char**)0x004663DC;

typedef void* (__cdecl *Menu_GetContextGlobals_fn)(void* pMenu);
static const Menu_GetContextGlobals_fn Call_Menu_GetContextGlobals = (Menu_GetContextGlobals_fn)0x00434070;

typedef void (__cdecl *Font_ResetCharQueue_fn)();
static const Font_ResetCharQueue_fn Call_Font_ResetCharQueue = (Font_ResetCharQueue_fn)0x0041D100;

typedef void* (__cdecl *Game_LoadLevel_fn)(const char* baseDir, const char* levelName, void* pApp);
static const Game_LoadLevel_fn Call_Game_LoadLevel = (Game_LoadLevel_fn)0x00421990;

typedef void (__cdecl *sub_4345C0_fn)(void* pMenu, int a2, int a3);
static const sub_4345C0_fn Call_sub_4345C0 = (sub_4345C0_fn)0x004345C0;

void ProcessMenuExit(void* pMenu, int a2, const char* baseDir, const char* levelName) {
	s_CamInitialized      = false;
    s_GreenLightTick      = 0;
    s_LeadingStartTick    = 0;
    s_LastWeaponGrantTick = 0;
    g_CurrentHeatLevel    = 0;
    s_CamInitialized = false;
    if (g_FreecamActive) {
        g_FreecamActive = false;
        ShowCursor(TRUE);
    }

    void* pApp = Call_Menu_GetContextGlobals(pMenu);
    Call_Font_ResetCharQueue();

    if (p_g_raceMode) {
        *p_g_raceMode = 0;
    }

    if (pApp) {
        *(uint32_t*)((char*)pApp + 0x90B0) = 0;
        *(uint32_t*)((char*)pApp + 35464)  = 0;
        *(uint32_t*)((char*)pApp + 35468)  = 0;
        *(uint32_t*)((char*)pApp + 35484)  = 0;
        *(uint32_t*)((char*)pApp + 35488)  = 0;
    }

    Call_Game_LoadLevel(baseDir, levelName, pApp);
    Call_sub_4345C0(pMenu, a2, 7);

    DWORD startWait = GetTickCount();
    while (((GetAsyncKeyState(VK_RETURN) & 0x8000) != 0 || 
            (GetAsyncKeyState(VK_SPACE)  & 0x8000) != 0) &&
           (GetTickCount() - startWait < 1500))
    {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        Sleep(10);
    }

    Sleep(150);

    if (pApp) {
        *(uint32_t*)((char*)pApp + 35464) = 0;
        *(uint32_t*)((char*)pApp + 35468) = 0;
        *(uint32_t*)((char*)pApp + 35484) = 0;
        *(uint32_t*)((char*)pApp + 35488) = 0;
    }
}

void __cdecl Hooked_RetireToSelect(void* pMenu, int a2, int a3) {
    if (a3 != 7) return;
    ProcessMenuExit(pMenu, a2, *p_off_selectDir, *p_off_selectName);
}

void __cdecl Hooked_QuitToMenu(void* pMenu, int a2, int a3) {
    if (a3 != 7) return;

    void* pApp = Call_Menu_GetContextGlobals(pMenu);
    uint16_t pauseState = pApp ? *(uint16_t*)((char*)pApp + 0x90A4) : 0;

    if (pauseState != 6) {
        HWND hWnd = *(HWND*)0x004806D0;
        if (hWnd) {
            DestroyWindow(hWnd);
        }
        exit(0);
        return;
    }

    ProcessMenuExit(pMenu, a2, *p_off_baseDir, *p_off_levelName);
}




void ApplyPatches() {
    SetProcessAffinityMask(GetCurrentProcess(), 0x00000001);
    LoadSettings();
    InitXInput();

    Orig_Input_ReadKeyboard = (Input_ReadKeyboard_fn)CreateTrampoline(
        0x00412E40, (void*)Hooked_Input_ReadKeyboard, 6
    );

    Orig_Camera_ProcessFlybyStep = (Camera_ProcessFlybyStep_fn)CreateTrampoline(
        0x00407860, (void*)Hooked_Camera_ProcessFlybyStep, 6
    );

    HookIAT(NULL, "kernel32.dll", "CreateFileA", (void*)Hooked_CreateFileA, (void**)&Real_CreateFileA);

    uint8_t pushOne = 0x01;
    PatchMemory(0x0040F972, &pushOne, 1);

    
    uint32_t newBufSize   = 0xF61A4;
    uint32_t newDwordCnt  = 0x3D860;
    uint32_t newMaxOffset = 0xF6050;
    PatchMemory(0x0041FFD5, &newBufSize, 4);
    PatchMemory(0x0041FFE4, &newBufSize, 4);
    PatchMemory(0x0041FFFA, &newDwordCnt, 4);
    PatchMemory(0x00420008, &newMaxOffset, 4);

    
    uint8_t patchNearZ[5] = { 0xB8, 0x19, 0x00, 0x00, 0x00 };
    PatchMemory(0x00407A4C, patchNearZ, 5);

    
    PatchCall(0x00426C4B, (uintptr_t)Hooked_Camera_ComputeViewMatrix);

    pGteFocalLength   = *(int32_t**)(0x00437470 + 3);
    pGteScreenCenterX = *(int32_t**)(0x0043747E + 2);
    pGteScreenCenterY = *(int32_t**)(0x0043749F + 2);

    pGteDepthZ0 = *(int32_t**)(0x00437650 + 1);
    pGteDepthZ1 = *(int32_t**)(0x004376D1 + 2);
    pGteDepthZ2 = *(int32_t**)(0x00437755 + 2);

    PatchCall(0x00420FD1, (uintptr_t)Hooked_Render_ClearOT);

    PatchJmp(0x0043746B, (uintptr_t)AsmHook_PerspectiveCalc_Single);
    NopMemory(0x00437470, 61);

    PatchJmp(0x00437980, (uintptr_t)AsmHook_PerspectiveCalc_Triangle);
    NopMemory(0x00437985, 220);

    Orig_Render_DrawSprt      = (Render_DrawSprt_fn)CreateTrampoline(0x004367D0, (void*)Hooked_Render_DrawSprt, 5);
    Orig_Render_DrawPrim_Tile = (Render_DrawPrim_Tile_fn)CreateTrampoline(0x00435200, (void*)Hooked_Render_DrawPrim_Tile, 5);

    
    for (uintptr_t addr = 0x00435200; addr < 0x00436B00; ++addr) {
        if (*(uint8_t*)addr == 0xE8) {
            int32_t rel = *(int32_t*)(addr + 1);
            if (addr + 5 + rel == (uintptr_t)Orig_Clip_ComputeVertexFlags) {
                PatchCall(addr, (uintptr_t)Hooked_Clip_ComputeVertexFlags);
            }
        }
    }

    uint32_t newIrisFactor = (uint32_t)(0x66666667ULL / GetCurrentAspectMultiplier());
    PatchMemory(0x0041ACB7, &newIrisFactor, 4);

    Orig_HUD_DrawSpriteScaled  = (HUD_DrawSpriteScaled_fn)CreateTrampoline(0x00423CF0, (void*)Hooked_HUD_DrawSpriteScaled, 7);
    Orig_HUD_DrawRotatedSprite = (HUD_DrawRotatedSprite_fn)CreateTrampoline(0x00423F60, (void*)Hooked_HUD_DrawRotatedSprite, 7);
    Orig_HUD_UpdateMinimapPos  = (HUD_UpdateMinimapPos_fn)CreateTrampoline(0x0042AE40, (void*)Hooked_HUD_UpdateMinimapPos, 7);
		
	Orig_Racer_AI_ComputeSteering = (Racer_AI_ComputeSteering_fn)CreateTrampoline(
		0x00402390, (void*)Hooked_Racer_AI_ComputeSteering, 5
	);

	
	Orig_Racer_IntegrateForces = (Racer_IntegrateForces_fn)CreateTrampoline(
		0x00441CD0, (void*)Hooked_Racer_IntegrateForces, 7
	);

	
	Orig_Racer_HandleWallCollision = (Racer_HandleWallCollision_fn)CreateTrampoline(
		0x00443970, (void*)Hooked_Racer_HandleWallCollision, 5
	);

	
	Orig_Racer_OnHit = (Racer_OnHit_fn)CreateTrampoline(
		0x00444410, (void*)Hooked_Racer_OnHit, 5
	);

	
	Orig_Racer_FireWeapon = (Racer_FireWeapon_fn)CreateTrampoline(
		0x00405930, (void*)Hooked_Racer_FireWeapon, 5
);
    
    g_ppMenuOptions       = *(void***)(0x0041718A + 2);
    g_pMenuSelectedOption = *(uint32_t**)(0x00417479 + 1);

    Orig_HUD_ProcessPauseMenuInput = (HUD_ProcessPauseMenuInput_fn)CreateTrampoline(
        0x004172F0, (void*)Hooked_HUD_ProcessPauseMenuInput, 5
    );

    
    PatchCall(0x00417323, (uintptr_t)Hooked_Menu_DrawAndHandleDebugMenu);

    
    Orig_Racer_ClampSpeed = (Racer_ClampSpeed_fn)CreateTrampoline(
        0x00444490, (void*)Hooked_Racer_ClampSpeed, 6
    );

    
    Orig_Input_ApplyPlayerBindings = (Input_ApplyPlayerBindings_fn)CreateTrampoline(
        0x00412CE0, (void*)Hooked_Input_ApplyPlayerBindings, 7
    );

    
    UpdateDebugOverlayPatch(true);

    
    PatchJmp(0x00426C90, (uintptr_t)Hooked_Camera_GetDefaultAspect);
    PatchJmp(0x00411500, (uintptr_t)Hooked_D3D_FindDisplayMode);
    PatchJmp(0x00431440, (uintptr_t)Hooked_Sys_DetectCDRom);
    PatchJmp(0x00430400, (uintptr_t)Hooked_Sys_CheckSystemRequirements);

    uint8_t zeroByte = 0x00;
    PatchMemory(0x0043153D, &zeroByte, 1);
    uint8_t jmpByte = 0xEB;
    PatchMemory(0x0043008F, &jmpByte, 1);
    PatchJmp(0x00430140, (uintptr_t)Hooked_Win_WaitAndProcessMessages);

    PatchJmp(0x004345F0, (uintptr_t)Hooked_RetireToSelect);
    PatchJmp(0x00434630, (uintptr_t)Hooked_QuitToMenu);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        ApplyPatches();
    }
    return TRUE;
}
