#include <iostream>
#include "..\includes\stdafx.h"
#include "..\Wrappers\wrapper.h"
#include "dllmain.h"
#include "WndProcHook.h"

std::string RealDllPath;
std::string WrapperMode;
std::string WrapperName;

HMODULE wrapper_dll = nullptr;
HMODULE proxy_dll = nullptr;

uint32_t ResPointer;

double fNewHealthBoxPos = 0.0;
double fNewPortraitPos = 16.0;
double fNewPowersPos;
double fNewCirclePos;

float fNewEyeBloodStretch;

void Init()
{
	std::cout << "Sono me... dare no me?" << std::endl;

	Init_WndProcHook();

	// Get game resolution
	auto pattern = hook::pattern("8B 0D ? ? ? ? 8B 01 A3 ? ? ? ? A1 ? ? ? ? 8B 51");
	ResPointer = injector::ReadMemory<uint32_t>(*pattern.count(1).get(0).get<uint32_t*>(2), true);
	struct GetGameRes
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = ResPointer;

			// Calculate HUD pos
			double fOrigPowerPos = 528.0;
			double fOrigCirclePos = 110.0;

			double fGameWidth = *(int32_t*)ResPointer;
			double fGameHeight = *(int32_t*)(ResPointer + 0x4);

			double fAspectRatio = (fGameWidth / fGameHeight);

			double fPowerPosOffset = (double)(((480.0f * fAspectRatio) - 640.0f) / 2.0f);

			fNewPowersPos = fOrigPowerPos + fPowerPosOffset;
			fNewCirclePos = fOrigCirclePos - fPowerPosOffset;

			// Actual magic
			fNewEyeBloodStretch = (float)(round(39.8113 - pow(0.000854096 * fGameHeight, -3.45158)));

			#ifdef VERBOSE
			std::cout << std::hex << "ResPointer = " << ResPointer << std::endl;
			std::cout << "fOrigPowerPos = " << fOrigPowerPos << std::endl;
			std::cout << "fOrigCirclePos = " << fOrigCirclePos << std::endl;
			std::cout << "fGameWidth = " << fGameWidth << std::endl;
			std::cout << "fGameHeight = " << fGameHeight << std::endl;
			std::cout << "fAspectRatio = " << fAspectRatio << std::endl;
			std::cout << "fPowerPosOffset = " << fPowerPosOffset << std::endl;
			std::cout << "fNewPowersPos = " << fNewPowersPos << std::endl;
			std::cout << "fNewCirclePos = " << fNewCirclePos << std::endl;
			#endif
			
			// Lock the cursor once we press Play
			EnableClipCursor(hWindow);
		}
	}; injector::MakeInline<GetGameRes>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

	// Fix "powers" section of the HUD
	pattern = hook::pattern("DC 0D ? ? ? ? 89 75 ? D8 05 ? ? ? ? D9 5D ? DB 05");
	injector::WriteMemory(pattern.get_first(2), &fNewPowersPos, true);

	// Fix HUD circle around the powers section
	pattern = hook::pattern("DD 05 ? ? ? ? 89 BD ? ? ? ? D8 C9 DE EA D9 C9 D9 9D");
	injector::WriteMemory(pattern.get_first(2), &fNewCirclePos, true);

	// Fix HUD health box
	pattern = hook::pattern("D9 05 ? ? ? ? DE C2 D9 C9 D9 5D ? D8 4D");
	injector::WriteMemory(pattern.get_first(2), &fNewHealthBoxPos, true);

	// Fix offset Rayne portrait
	pattern = hook::pattern("DC 05 ? ? ? ? D9 5D ? D9 45 ? DD 45 ? D8 C1");
	injector::WriteMemory(pattern.get_first(2), &fNewPortraitPos, true);

	// Fix offset red portrait overlay
	pattern = hook::pattern("DC 05 ? ? ? ? D9 5D ? D9 C1 DC 45 ? DD 55 ? D9 5D");
	injector::WriteMemory(pattern.get_first(2), &fNewPortraitPos, true);

	// Fix blood on Rayne's eye.
	// Looks like the blood is supposed to be actually on top of her eye, but it is misaligned in every resolution above 640x480.
	// This still doesn't make it perfect, but it is better I guess.
	pattern = hook::pattern("D8 05 ? ? ? ? 03 D1 C1 FA ? 8B CA D9 5D ? C1 E9"); // X pos
	injector::MakeNOP(pattern.get_first(0), 6, true);

	pattern = hook::pattern("D8 C9 89 75 ? D9 C9 DC 0D ? ? ? ? DE C1 D9 5D ? D9 C9"); // Y pos
	injector::MakeNOP(pattern.get_first(0), 2, true);

	pattern = hook::pattern("D9 5D ? D9 05 ? ? ? ? D9 5D ? DC 0D");
	injector::WriteMemory(pattern.get_first(5), &fNewEyeBloodStretch, true);

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

		Init();

		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return true;
}