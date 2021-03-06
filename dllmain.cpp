// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "SDK.hpp"

bool bStop = false;
bool bCheatFlying = false;
float AtkSpeed = 1.f;
float MaxWalkSpeed = 1.f;
float MaxWalkSpeedCrouched = 1.f;
float MaxAcceleration = 1.f;
float MaxCustomMovementSpeed = 1.f;
float MaxFlySpeed = 1.f;
float CameraDefaultDistance = 1.f;
float JumpZVelocity = 100.f;
float GravityScale = 1.f;

bool match(const PBYTE memory, const BYTE* sig, const std::string& mask)
{
	//if (!is_good_protect(memory))
	//return false;
	for (DWORD j = 0; j < mask.length(); j++)
		if (mask[j] != '?' && memory[j] != sig[j])
			return false;
	return true;
}

PVOID FindSignature(ULONG_PTR address, size_t size, const BYTE* sig, const std::string & mask)
{
	address += 0x1000;
	PBYTE pAddress = (PBYTE)address;
	if (size < mask.length())
		return nullptr;
	for (SIZE_T i = 0; i < (size - mask.length()) + 1; i++) {
		if (match(&pAddress[i], sig, mask))
			return &pAddress[i];
	}
	return nullptr;
}

DWORD get_module_size(ULONG_PTR lib)
{
	PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER)(lib);
	if (!pIDH || pIDH->e_magic != IMAGE_DOS_SIGNATURE || pIDH->e_lfanew > 0xFFFF)
		return 0;
	PIMAGE_NT_HEADERS pINH = (PIMAGE_NT_HEADERS)(ULONG_PTR(pIDH) + pIDH->e_lfanew);
	if ((ULONG_PTR)pINH < (ULONG_PTR)pIDH || pINH->Signature != IMAGE_NT_SIGNATURE)
		return 0;
	return pINH->OptionalHeader.SizeOfImage + pINH->OptionalHeader.SizeOfHeaders;
}

bool initialize()
{
	auto module_size = get_module_size((ULONG_PTR)GetModuleHandle(NULL));
	auto sig = FindSignature((ULONG_PTR)GetModuleHandle(NULL), module_size, (PBYTE)"\x48\x89\x1D\x00\x00\x00\x00\x48\x8B\x5C\x24\x00\x48\x83\xC4\x28\xC3\x48\x8B\x5C\x24\x00\x48\x89\x05\x00\x00\x00\x00\x48\x83\xC4\x28\xC3", "xxx????xxxx?xxxxxxxxx?xxx????xxxxx");
	if (sig == nullptr) {
		return false;
	}
	auto GNamesPtr = (ULONG_PTR)sig + (*PDWORD((ULONG_PTR)sig + 3) + 7);

	sig = FindSignature((ULONG_PTR)GetModuleHandle(NULL), module_size, (PBYTE)"\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x01\x33\xC9\x84\xD2\x41\x8B\x40\x08\x49\x89\x48\x10\x0F\x45\x05\x00\x00\x00\x00\xFF\xC0\x49\x89\x48\x10\x41\x89\x40\x08", "xxx????xxxxxxxxxxxxxxxxxx????xxxxxxxxxx");
	if (sig == nullptr) {
		return false;
	}
	auto GObjectsPtr = (ULONG_PTR)sig + (*PDWORD((ULONG_PTR)sig + 3) + 7);

	SDK::FName::GNames = *reinterpret_cast<SDK::TNameEntryArray**>(GNamesPtr);
	if (SDK::FName::GNames == nullptr) {
		return false;
	}
	SDK::UObject::GObjects = reinterpret_cast<SDK::FUObjectArray*>(GObjectsPtr);
	if (SDK::UObject::GObjects == nullptr) {
		return false;
	}
	return true;
}

void cheat()
{
	MessageBoxA(0, "Injected", "Dauntless Cheats", 0);
	while (!bStop) {
		if (!initialize()) {
			MessageBoxA(0, "Failed to initialize!", "", MB_ICONERROR);
			break;
		}

		try {
			auto UWorld = *(SDK::UWorld**)((ULONG_PTR)GetModuleHandle(NULL) + 0x3FEB988);


			if (UWorld == nullptr || UWorld->OwningGameInstance == nullptr)
				continue;
			if (UWorld->OwningGameInstance->LocalPlayers.Num() == 0)
				continue;
			auto controller = UWorld->OwningGameInstance->LocalPlayers[0]->PlayerController;

			auto local_player = reinterpret_cast<SDK::ABP_PlayerCharacter_C*>(controller->AcknowledgedPawn);
			if (local_player && !local_player->IsA(SDK::ABP_PlayerCharacter_C::StaticClass()))
				local_player = nullptr;

			if (!local_player)
				continue;

			// Inf Stamina
			if (auto ab = local_player->AbilitySystemComponent) {
				for (int i = 0; i < ab->SpawnedAttributes.Num(); i++) {
					auto item = ab->SpawnedAttributes[i];
					if (!item)
						continue;
					if (item->IsA(SDK::UArchonHealthAttributeSet::StaticClass())) {
						auto attrib = reinterpret_cast<SDK::UArchonHealthAttributeSet*>(item);
						attrib->Stamina = attrib->MaxStamina;
						attrib->StaminaRegen = attrib->MaxStamina;
						attrib->LanternChargeMultiplier = 1000.f;
						attrib->StaminaConsumption = 0.f;
						attrib->EvadeStaminaCostMultiplier = 0.f;
					}
				}
			}

			// Items
			auto hud = UWorld->OwningGameInstance->LocalPlayers[0]->PlayerController->MyHUD;
			auto load = reinterpret_cast<SDK::AArchonPlayerController*>(hud->PlayerOwner);
			if (hud->PlayerOwner && hud->PlayerOwner->IsA(SDK::AArchonPlayerController::StaticClass()))
				load = reinterpret_cast<SDK::AArchonPlayerController*>(hud->PlayerOwner);
			else
				load = nullptr;

			if (auto loadout = load->LOADOUT)
			{
				for (int i = 0; i < loadout->CurrentLoadout.QuickItems.Num(); i++)
				{
					auto &item = loadout->CurrentLoadout.QuickItems[i];
					item.AmountRemaining = 5;
				}
			}						

			// Movement 

			local_player->Jump_Enabled = TRUE; 

			if (auto a = local_player->CharMovement)
			{
				if (GetAsyncKeyState(VK_LSHIFT) & 1)
				{
					a->MaxWalkSpeed += 500.f;
					a->MaxWalkSpeedCrouched += 500.f;
					a->MaxAcceleration += 100.f;
					a->MaxCustomMovementSpeed += 500.f;
				}
				if (GetAsyncKeyState(VK_HOME) & 1)
				{
					a->JumpZVelocity *= 5.f;
				}
				if (GetAsyncKeyState(VK_LMENU) & 1)
				{
					a->bCheatFlying = TRUE;
					a->GravityScale = 0.f;
					a->MaxFlySpeed += 500.f;
					a->SetMovementMode(SDK::EMovementMode::MOVE_Flying, 0); // (BYTE)SDK::EMovementMode::MOVE_Flying
				}
				else 
				{
					a->GravityScale = 1.f;
					a->MaxFlySpeed -= 500.f;
					a->bCheatFlying = FALSE;
				}
			}

			// Camera
			if (GetAsyncKeyState(VK_NEXT))
			{
				local_player->CameraDefaultDistance += 100.f;
			}
			if (GetAsyncKeyState(VK_PRIOR))
			{
				local_player->CameraDefaultDistance -= 100.f;
			}

			// Attack Speed
						
			if (auto ab = local_player->AbilitySystemComponent) {
				for (int i = 0; i < ab->SpawnedAttributes.Num(); i++) {
					auto item = ab->SpawnedAttributes[i];
					if (!item)
						continue;

					if (item->IsA(SDK::UArchonDamageAttributeSet::StaticClass())) {
						auto attrib = reinterpret_cast<SDK::UArchonDamageAttributeSet*>(item);
						//float values[] = { 1.f, 3.f, 5.f, 10.f };
						//auto index = ui.player.fast_attack->index;
						//if (index < 0 || index > 3)
						//index = 0;
						if (GetAsyncKeyState(VK_F1) & 1)
						{
							AtkSpeed = 1.f;
						}
						if (GetAsyncKeyState(VK_F2) & 1)
						{
							AtkSpeed = 3.f;
						}
						if (GetAsyncKeyState(VK_F3) & 1)
						{
							AtkSpeed = 7.f;
						}
						if (GetAsyncKeyState(VK_F4) & 1)
						{
							AtkSpeed = 20.f;
						}
						if (GetAsyncKeyState(VK_F5) & 1)
						{
							AtkSpeed = 50.f;
						}
						attrib->CombatPlayRateRateModifier = AtkSpeed;
					}
				}
			}

			// Items
			if (auto ps_base = local_player->PlayerState) {
				if (ps_base->IsA(SDK::Aplayer_state_bp_C::StaticClass())) {
					auto ps = reinterpret_cast<SDK::Aplayer_state_bp_C*>(ps_base);
					if (auto inv_component = ps->ArchonInventory) {
						for (int i = 0; i < inv_component->QuickSlots.Num(); i++) {
							auto &item = inv_component->QuickSlots[i];
							item.RechargeDuration = 0.f;
							item.RechargeTime = 0.f;
							//	item.AmountRemaining = 1000;
						}
					}
				}
			}

		}
		catch (...) {
			//MessageBoxA(0, "Access violation", "", 0);
		}

		Sleep(10);
	}
	MessageBoxA(0, "Goodbye!", "Dauntless Cheats", 0);
}

HANDLE hThread = NULL;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hThread = ::CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)cheat, NULL, NULL, nullptr);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		bStop = true;
		if (hThread != NULL) {
			::WaitForSingleObject(hThread, INFINITE);
			::CloseHandle(hThread);
		}
		break;
	}
	return TRUE;
}