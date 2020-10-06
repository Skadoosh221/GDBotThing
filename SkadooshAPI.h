#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>

class SkadooshAPI {
private:
	INPUT input = { 0 };
	DWORD pid, modBaseAddr, pointAddr;
	DWORD GetBaseAddr(DWORD procId, const wchar_t* modName) {
		uintptr_t modBaseAddr = 0;
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
		if (hSnap != INVALID_HANDLE_VALUE) {
			MODULEENTRY32 modEntry;
			modEntry.dwSize = sizeof(modEntry);
			if (Module32First(hSnap, &modEntry)) {
				do {
					if (!_wcsicmp(modEntry.szModule, modName)) {
						modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
						break;
					}
				} while (Module32Next(hSnap, &modEntry));
			}
		}
		CloseHandle(hSnap);
		return modBaseAddr;
	}
public:
	int baseAddress = 0;
	int baseOffset = 0x3222D0;
	LPCSTR windowName = "";
	HANDLE wHand;
	const wchar_t* moduleName = L"";

	std::vector<long double> configVectors;
	std::vector < std::string> configTooltips;

	//Grabs the baseaddress
	void GetBaseAddress() {
		//Set base address
		CloseHandle(wHand);
		HWND hWnd = FindWindowA(0, windowName);
		GetWindowThreadProcessId(hWnd, &pid);
		wHand = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

		modBaseAddr = GetBaseAddr(pid, moduleName);
		ReadProcessMemory(wHand, (LPVOID)(modBaseAddr + baseOffset), &baseAddress, sizeof(baseAddress), NULL);
	}

	//Returns the address based on the baseAddress + offsets
	int GetAddressByOffset(std::vector<DWORD> offsets) {
		HANDLE wHand = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
		pointAddr = baseAddress;
		for (int i = 0; i < offsets.size() - 1; i++) {
			ReadProcessMemory(wHand, (LPVOID)(pointAddr += offsets[i]), &pointAddr, sizeof(pointAddr), NULL);
		}
		pointAddr += offsets[offsets.size() - 1];
		CloseHandle(wHand);
		return pointAddr;
	}

	//Returns the value of the address (value must  already initialized)
	template <typename t>
	t ReadAddress(int addr, t val) {
		t value = val;
		ReadProcessMemory(wHand, (LPVOID)addr, &value, sizeof(value), NULL);
		return value;
	}

	std::string ReadAddress(int addr, std::string val) {
		std::string tmp;
		for (int i = 0; i < 50; i++) {
			char cvar[128];
			ReadProcessMemory(wHand, (LPVOID)addr, &cvar, sizeof(cvar), NULL);
			if (cvar[i] != '\0' && cvar[i] != 'Ì') {
				tmp += cvar[i];
			}
			else {
				break;
			}
		}
		return tmp;
	}

	//Write to address
	template <typename t>
	void WriteAddress(int addr, t val) {
		t value = val;
		WriteProcessMemory(wHand, (LPVOID)addr, &value, sizeof(value), NULL);
	}

	//Send key input, 0 = DOWN, 1 = UP
	void Input(SHORT key, int value) {
		input.type = INPUT_KEYBOARD;
		input.ki.wScan = MapVirtualKey(LOBYTE(key), 0);
		input.ki.dwFlags = KEYEVENTF_SCANCODE;

		if (value == 0) {
			input.ki.dwFlags = 0;
			SendInput(1, &input, sizeof(input));
		}
		else {
			input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(input));
		}
	}

	//Returns 1 if file exists or 0 if not
	int FileExists(std::string filename) {
		std::fstream file;
		file.open(filename);
		if (file.fail()) {
			return 0;
		}
		return 1;
	}

	//Save vector data into a file
	template <typename t>
	void SaveVectors(std::vector<t> vectors, std::string name) {
		if (FileExists(name)) {
			remove(name.c_str());
		}

		std::ofstream file(name);
		for (int i = 0; i < vectors.size(); i++) {
			file << std::setprecision(10000) << vectors[i] << std::endl;
		}
	}

	//Load vector data from a file
	std::vector<long double> LoadVectors(std::string name) {
		std::vector<long double> tmp;
		std::ifstream file(name);
		if (file.is_open()) {
			std::string line;
			int i = 0;
			while (getline(file, line)) {
				tmp.resize(i + 1);
				tmp[i] = stod(line);
				i++;
			}
		}
		return tmp;
	}

	//Save config file with vectors, tooltips, filename
	void SaveConfig(std::string filename) {
		if (FileExists(filename)) {
			remove(filename.c_str());
		}

		std::ofstream file(filename);
		int i = 0;
		for (const auto& e : configVectors) {
			file << std::setprecision(10000) << configVectors[i] << "; : " << configTooltips[i] << std::endl;
			i++;
		}
	}

	//Return the config values from a file
	std::vector<long double> LoadConfig(std::string name) {
		std::vector<long double> tmp;
		std::ifstream file(name);
		if (file.is_open()) {
			std::string line;
			std::string tmpString;
			int i = 0;
			while (getline(file, line)) {
				tmp.resize(i + 1);

				for (char& c : line) {
					if (c == ';') {
						break;
					}
					tmpString += c;
				}

				tmp[i] = stod(tmpString);
				tmpString = "";
				i++;
			}
		}
		return tmp;
	}

	//Find text inside of a file
	int FindTextInFile(std::string text, std::string filename) {
		std::ifstream file(filename);
		std::string line, tmp, target = text;
		while (std::getline(file, line)) {
			const char* targetc = target.c_str();
			int i = 0;
			for (char& c : line) {
				if (c == targetc[i]) {
					tmp += c;
					i++;
				}
				else {
					tmp = "";
					i = 0;
				}
			}
		}
		if (tmp == target) {
			return 1;
		}
		return 0;
	}

	//Returns the xor value
	std::string Xor(std::string str, int key) {
		std::string tmp;
		for (char c : str) {
			tmp += c ^ key;
		}
		return tmp;
	}
};