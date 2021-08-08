#include <iostream>
#include "..\includes\stdafx.h"
#include "..\Wrappers\wrapper.h"
#include "CursorLock.h"

std::string RealDllPath;
std::string WrapperMode;
std::string WrapperName;

HMODULE wrapper_dll = nullptr;
HMODULE proxy_dll = nullptr;

//#define VERBOSE

uintptr_t jmpAddrGetGameRes;

float fNewHealthBoxPos = 0.0f;
float fNewPortraitPos = 16.0f;
float fNewPowersPos;
float fNewCirclePos;

void CalculateHudPos(DWORD ResPointer)
{
	float fOrigPowerPos = 528;
	float fOrigCirclePos = 110;

	double fGameWidth = injector::ReadMemory<int>(ResPointer, true);
	double fGameHeight = injector::ReadMemory<int>(ResPointer + 0x4, true);

	float fAspectRatio = (fGameWidth / fGameHeight);
	float fPowerPosOffset = ((480.0f * fAspectRatio) - 640.0f) / 2.0f;

	fNewPowersPos = fOrigPowerPos + fPowerPosOffset;
	fNewCirclePos = fOrigCirclePos - fPowerPosOffset;

	#ifdef VERBOSE
	std::cout << "fNewPowersPos = " << fNewPowersPos << std::endl;
	std::cout << "fNewCirclePos = " << fNewCirclePos << std::endl;
	#endif

	SetCursorLock("BloodRayne");
}

DWORD _EAX;
DWORD _ECX;
DWORD ResPointer;
void __declspec(naked) GetGameRes()
{
	_asm
	{
		mov _EAX, eax
		mov _ECX, ecx
		mov ResPointer, ecx
	}

	CalculateHudPos(ResPointer);

	_asm
	{
		mov eax, _EAX
		mov ecx, _ECX
		mov eax, [ecx + 0x4]
		add eax, eax
		jmp jmpAddrGetGameRes
	}
}

void __declspec(naked) HUDFixHealthBox()
{
	_asm
	{
		fld   dword ptr ds : [fNewHealthBoxPos]
		ret
	}
}

void __declspec(naked) HUDFixRaynePortrait()
{
	_asm
	{
		fadd   dword ptr ds : [fNewPortraitPos]
		ret
	}
}

void __declspec(naked) HUDFixCircle()
{
	_asm
	{
		fld   dword ptr ds : [fNewCirclePos]
		ret
	}
}

void __declspec(naked) HUDFixPowers()
{
	_asm
	{
		fmul dword ptr ds : [fNewPowersPos]
		ret
	}
}

DWORD WINAPI Init(LPVOID)
{
	std::cout << "Sono me... dare no me?" << std::endl;

	// Get game resolution
	auto pattern = hook::pattern("8B 41 04 03 C0 A3 ? ? ? ? C7 41 08 ? ? ? ? 8B 0D ? ? ? ? 51");
	injector::MakeNOP(pattern.get_first(0), 5, true);
	injector::MakeJMP(pattern.get_first(0), GetGameRes, true);
	jmpAddrGetGameRes = (uintptr_t)pattern.count(1).get(0).get<uint32_t>(5);

	// Fix "powers" section of the HUD
	pattern = hook::pattern("DC 0D ? ? ? ? 89 75 D4 D8 05 ? ? ? ? D9 5D ?");
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

	// Fix offset red portrait overlay
	pattern = hook::pattern("DC 05 ? ? ? ? D9 5D ? D9 C1 DC 45 ? DD 55 ? D9 5D ? D9 45 ?");
	injector::MakeNOP(pattern.get_first(0), 6, true);
	injector::MakeCALL(pattern.get_first(0), HUDFixRaynePortrait, true);

	// Fix blood on Rayne's eye
	pattern = hook::pattern("D8 05 ? ? ? ? 03 D1 C1 FA ? 8B CA D9 5D ? C1 E9 ?");
	injector::MakeNOP(pattern.get_first(0), 6, true);
	
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