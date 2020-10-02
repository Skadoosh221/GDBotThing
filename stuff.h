#pragma once
#include <iostream>
#include <fstream>
#include <chrono>
#include <Windows.h>
#include <thread>
#include <tchar.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
using namespace std;
using namespace chrono;

//*--Variables--*//
//All the normal values that you can tweak
float blockDivision = 1;

float rewardRange = 10, blockSize = 30, placeCheckpointRange = 30;
float baseChance = 50 / blockDivision; // Higher it is, the more likely for a click

float preLoadSize = floor(200000 / blockSize); // The amount of vectors you want preloaded if chosen

float reward = 25; // How likely the ai will do whatever it did at the block rewardRange
float rewardMin = 0, rewardMax = 100;

float deathFailRange = rewardRange; // if the amount of deaths at the same place pass the threshold, how many blocks behind should be deemed as bad
float deathFail = 20; // the amount to fail the actions when on a death mutation
int amountOfSameDeathToMut = 5; // Amount of times to die at the same spot to mutate

bool startingNew = false;
bool aiDisabled = false;
bool guideMode = false;
bool allowCheckpoints = true;

//Values that you CANNOT TWEAK!!!
int xpos, lastxpos, vlastxpos, furthestXpos;
int dieSPTimes = 0, lastDeathPos, vLastDeathPos;

bool checkIsDead;
bool checkedIfAiActive, checkedGuideMode;

long long currentGamemode;

enum State {
	CLICK, NONE
};

_TCHAR gameName[] = _T("GeometryDash.exe");
DWORD pid, baseAddress = NULL, gameBaseAddress, offsetGameToBaseAdress = 0x003222D0;
vector < DWORD > pointsOffsets{ 0x164, 0x224, 0x67C };
vector < DWORD > pointsOffsetsGM{ 0x164, 0x224, 0x638 };
DWORD pointsAddress, pointsAddressGM;

INPUT input;
SHORT key = VK_SPACE;
SHORT key2 = VkKeyScan('z');
UINT mappedKey = MapVirtualKey(LOBYTE(key), 0);
UINT mappedKey2 = MapVirtualKey(LOBYTE(key2), 0);

vector<State> states;
vector<float> clickChance;

//*--Functions--*//
DWORD GetModuleBaseAddress(TCHAR* modname, DWORD pid) {
	DWORD modBaseAddr = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	MODULEENTRY32 ModuleEntry32 = { 0 };
	ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(hSnapshot, &ModuleEntry32)) {
		do {
			if (_tcscmp(ModuleEntry32.szModule, modname) == 0) {
				modBaseAddr = (DWORD)ModuleEntry32.modBaseAddr; break;
			}
		} while (Module32Next(hSnapshot, &ModuleEntry32));
	}
	CloseHandle(hSnapshot);
	return modBaseAddr;
}

bool FileExist(string name) {
	fstream file;
	file.open(name);
	if (file.fail()) {
		return false;
	}
	else {
		return true;
	}
}

void GetAddressData() {
	HWND hProc = FindWindowA(0, "Geometry Dash");
	DWORD processId = GetWindowThreadProcessId(hProc, &pid);
	HANDLE wHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	pointsAddress = 0;
	pointsAddressGM = 0;

	while (pointsAddress < 0x100000) {
		//Gets base address + offset
		gameBaseAddress = GetModuleBaseAddress(gameName, pid);
		ReadProcessMemory(wHandle, (LPVOID)(gameBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);

		//Get xpos addr
		pointsAddress = baseAddress;
		for (int i = 0; i < pointsOffsets.size() - 1; i++) {
			ReadProcessMemory(wHandle, (LPVOID)(pointsAddress + pointsOffsets.at(i)), &pointsAddress, sizeof(pointsAddress), NULL);
		}
		pointsAddress += pointsOffsets.at(pointsOffsets.size() - 1);

		//Get gamemode addr
		pointsAddressGM = baseAddress;
		for (int i = 0; i < pointsOffsetsGM.size() - 1; i++) {
			ReadProcessMemory(wHandle, (LPVOID)(pointsAddressGM + pointsOffsetsGM.at(i)), &pointsAddressGM, sizeof(pointsAddressGM), NULL);
		}
		pointsAddressGM += pointsOffsetsGM.at(pointsOffsetsGM.size() - 1);
	}

	CloseHandle(wHandle);
}