#include <iostream>
#include "..\includes\stdafx.h"
#include "..\Wrappers\wrapper.h"

std::string RealDllPath;
std::string WrapperMode;
std::string WrapperName;

HMODULE wrapper_dll = nullptr;
HMODULE proxy_dll = nullptr;

//#define VERBOSE

float fHUDHealthBoxPos = 0.0f;
float fHUDPortraitPos = 16.0f;
float fHUDRingPos = 0.0f;
float fHUDPowersPos = 637.0f;


void __declspec(naked) HUDFixHealthBox()
{
	_asm
	{
		fld   dword ptr ds : [fHUDHealthBoxPos]
		ret
	}
}

void __declspec(naked) HUDFixRaynePortrait()
{
	_asm
	{
		fadd   dword ptr ds : [fHUDPortraitPos]
		ret
	}
}

void __declspec(naked) HUDFixCircle()
{
	_asm
	{
		fld   dword ptr ds : [fHUDRingPos]
		ret
	}
}

void __declspec(naked) HUDFixPowers()
{
	_asm
	{
		fmul dword ptr ds : [fHUDPowersPos]
		ret
	}
}


DWORD WINAPI Init(LPVOID)
{
	std::cout << "yo" << std::endl;

	// Fix "powers" section of the HUD
	auto pattern = hook::pattern("DC 0D ? ? ? ? 89 75 D4 D8 05 ? ? ? ? D9 5D ?");
	injector::MakeNOP(pattern.get_first(0), 6, true);
	injector::MakeCALL(pattern.get_first(0), HUDFixPowers, true);

	// Fix HUD circle around the powers section
	pattern = hook::pattern("DD 05 ? ? ? ? 89 BD ? ? ? ? D8 C9 DE EA D9 C9");
	injector::MakeNOP(pattern.get_first(0), 6, true);
	injector::MakeCALL(pattern.get_first(0), HUDFixCircle, true);

	// Fix HUD health box
	pattern = hook::pattern("D9 05 ? ? ? ? DE C2 D9 C9 D9 ? ? D8 4D ?");
	injector::MakeNOP(pattern.get_first(0), 6, true);
	injector::MakeCALL(pattern.get_first(0), HUDFixHealthBox, true);
	
	// Fix offset Rayne portrait
	pattern = hook::pattern("DC 05 ? ? ? ? D9 5D ? D9 45 ? DD 45 ? D8 C1 D8 E4");
	injector::MakeNOP(pattern.get_first(0), 6, true);
	injector::MakeCALL(pattern.get_first(0), HUDFixRaynePortrait, true);
	
	return S_OK;
}

void LoadRealDLL(HMODULE hModule)
{
	char configname[MAX_PATH];
	GetModuleFileNameA(hModule, configname, MAX_PATH);
	WrapperName.assign(strrchr(configname, '\\') + 1);

	// Get wrapper mode
	const char* RealWrapperMode = Wrapper::GetWrapperName((WrapperMode.size()) ? WrapperMode.c_str() : WrapperName.c_str());

	proxy_dll = Wrapper::CreateWrapper((RealDllPath.size()) ? RealDllPath.c_str() : nullptr, (WrapperMode.size()) ? WrapperMode.c_str() : nullptr, WrapperName.c_str());
}

// Dll main function
bool APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:

#ifdef VERBOSE
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
#endif
		LoadRealDLL(hModule);

		CloseHandle(CreateThread(nullptr, 0, Init, nullptr, 0, nullptr));

		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return true;
}