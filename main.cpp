#include "stuff.h"
#include "defines.h"

void Input(int in) {
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = KEYEVENTF_SCANCODE;
	input.ki.wScan = mappedKey;

	if (in == CLICK) {
		input.ki.dwFlags = 0;
		SendInput(1, &input, sizeof(input));
	}
	else {
		input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(input));
	}
}

void SetAddressValues() {
	HANDLE wHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	//Set xpos value
	float value = 0;
	ReadProcessMemory(wHandle, (LPVOID)pointsAddress, &value, sizeof(value), NULL);
	xpos = value / blockSize;

	//Set gamemode value
	long long value2 = 0;
	ReadProcessMemory(wHandle, (LPVOID)pointsAddressGM, &value2, sizeof(value2), NULL);
	currentGamemode = value2;

	CloseHandle(wHandle);
}

int Start() {
	if (FileExist("config.txt")) {
		//Load configs
		ifstream configFile("config.txt");
		if (configFile.is_open()) {
			string line;
			int i = 0;
			while (getline(configFile, line)) {
				switch (i) {
				case 0:
					reward = stof(line);
					break;
				case 1:
					rewardMax = stof(line);
					break;
				case 2: 
					rewardMin = stof(line);
					break;
				case 3:
					rewardRange = stof(line);
					break;
				case 4: 
					baseChance = stof(line); 
					break;
				}
				i++;
			}
		}
	}
	else {
		ofstream configFile("config.txt");
		if (configFile.is_open()) {
			configFile << reward << endl;
			configFile << rewardMax << endl;
			configFile << rewardMin << endl;
			configFile << rewardRange << endl;
			configFile << baseChance << endl << endl;
			configFile << "-reward\n-rewardMax\n-rewardMin\n-rewardRange\n-baseChance";
		}
	}
	cout << "Controls: \n-'g' to reget the address (use if ai isn't doing anything)\n-'l' disables/enables ai\n-'k' disables/enables guide mode (use right click instead of left)\n-'r' reset ai data\n-'left' to save and quit\n-'right' to quit without saving\n\n";
	if (FileExist("vectors.txt") && FileExist("states.txt")) {
		string input;
		cout << "Would you like to load current data? (curent dir) y/n: ";
		cin >> input;

		if (input == "y") {
			//Load clickChance vectors
			ifstream vectorsFile("vectors.txt");
			if (vectorsFile.is_open()) {
				string line;
				int i = 0;
				while (getline(vectorsFile, line)) {
					clickChance.resize(i + 1);
					clickChance[i] = stof(line);
					i++;
				}
			}

			//Load states
			ifstream statesFile("states.txt");
			if (statesFile.is_open()) {
				string line;
				int i = 0;
				while (getline(statesFile, line)) {
					states.resize(i + 1);
					if (stoi(line) == NONE) {
						states[i] = NONE;
					}
					else {
						states[i] = CLICK;
					}
					i++;
				}
			}
		}
		else if (input == "n") {
			startingNew = true;
			cout << "Do you want to preload data? (faster performance, more space) y/n: ";
			cin >> input;

			if (input == "y") {
				for (int i = 0; i < preLoadSize; i++) {
					clickChance.resize(i + 1);
					clickChance[i] = baseChance;
					states.resize(i + 1);
					states[i] = NONE;
				}
			}
			else if (input != "n") {
				cout << "invalid parameter";
				this_thread::sleep_for(500ms);
				return -1;
			}
		}
		else {
			cout << "invalid parameter";
			this_thread::sleep_for(500ms);
			return -1;
		}
	}
	else {
		startingNew = true;
	}
	GetAddressData();
}

void Ai() {
	//auto t1 = high_resolution_clock::now();

	//Resize arrays
	while (xpos >= clickChance.size()) {
		clickChance.resize(clickChance.size() + 1);
		clickChance[clickChance.size() - 1] = baseChance;
	}
	while (xpos >= states.size()) {
		states.resize(xpos + 1);
	}

	//When dead
	if (currentGamemode >= dead && !checkIsDead) {
		checkIsDead = true;
		vLastDeathPos = lastDeathPos;
		lastDeathPos = xpos;

		if (lastDeathPos == vLastDeathPos) {
			dieSPTimes++;
			if (dieSPTimes >= amountOfSameDeathToMut) {
				for (int i = 0; i <= deathFailRange; i++) {
					if (lastDeathPos - i >= 0) {
						if (rand() % 100 >= 50) {
							clickChance[lastDeathPos - i] += deathFail;
						}
						else {
							clickChance[lastDeathPos - i] -= deathFail;
						}
					}
				}
			}
		}
		else {
			dieSPTimes = 0;
		}
	}
	else if (currentGamemode < dead) {
		checkIsDead = false;
	}

	//Update every new block
	if (xpos != lastxpos) {
		if (!aiDisabled) {
			//Change state based on xpos chance ya ba da ba doo
			if (xpos - 1 >= 0) {
				if (rand() % 100 < clickChance[xpos]) {
					Input(CLICK);
					states[xpos] = CLICK;
				}
				else {
					Input(NONE);
					states[xpos] = NONE;
				}
			}

			//Reward ai
			if (xpos - rewardRange >= 0) {
				if (states[xpos - rewardRange] == CLICK) {
					clickChance[xpos - rewardRange] += reward;
				}
				else {
					clickChance[xpos - rewardRange] -= reward;
				}
			}
		}
		else {
			if (guideMode) {
				if (GetAsyncKeyState(VK_RBUTTON)) {
					Input(CLICK);
					clickChance[xpos] += reward;
					states[xpos] = CLICK;
				}
				else {
					Input(NONE);
					clickChance[xpos] -= reward;
					states[xpos] = NONE;
				}
			}
		}
		//Round up clickChance
		if (xpos - rewardRange >= 0) {
			if (clickChance[xpos - rewardRange] > rewardMax) {
				clickChance[xpos - rewardRange] = rewardMax;
			}
			else if (clickChance[xpos - rewardRange] < rewardMin) {
				clickChance[xpos - rewardRange] = rewardMin;
			}
		}
	}

	vlastxpos = lastxpos;
	lastxpos = xpos;

	//auto t2 = high_resolution_clock::now();
	//system("cls");
	//cout << "Duration: " << duration_cast<microseconds>(t2 - t1).count() << "us";
}

int OtherIn() {
	while (true) {
		this_thread::sleep_for(2ms);
		if ((GetAsyncKeyState(VkKeyScan('l')) & 0x0001) != 0) {
			if (checkedIfAiActive) {
				aiDisabled = true;
				cout << "Ai Disabled!" << endl;
				checkedIfAiActive = false;
			}
			else {
				aiDisabled = false;
				guideMode = false;
				checkedIfAiActive = true;
				cout << "Ai Enabled!" << endl;
			}
		}
		if ((GetAsyncKeyState(VkKeyScan('k')) & 0x0001) != 0) {
			if (checkedGuideMode) {
				guideMode = false;
				cout << "GuideMode Disabled!" << endl;
				checkedGuideMode = false;
			}
			else {
				guideMode = true;
				aiDisabled = true;
				cout << "GuideMode Enabled!" << endl;
				checkedGuideMode = true;
			}
		}
		if ((GetAsyncKeyState(VkKeyScan('g')) & 0x0001) != 0) {
			cout << "Regetting address...";
			GetAddressData();
			cout << "Done!\n";
		}
		if ((GetAsyncKeyState(VkKeyScan('r')) & 0x0001) != 0) {
			for (int i = 0; i < clickChance.size(); i++) {
				clickChance[i] = baseChance;
				states[i] = NONE;
			}
			cout << "Reset very nice!\n";
		}
	}
}

int main() {
	Start();
	this_thread::sleep_for(1s);
	thread otherin(OtherIn);

	cout << "Started!\n";
	//Main loop
	while (true) {
		//auto t1 = high_resolution_clock::now();
		//Save or quit
		if ((GetAsyncKeyState(VK_LEFT) & 0x0001) != 0) {
			remove("vectors.txt");
			remove("states.txt");

			//Save clickChance vectors
			ofstream vectorsFile("vectors.txt");
			if (vectorsFile.is_open()) {
				for (int i = 0; i < clickChance.size(); i++) {
					vectorsFile << clickChance[i] << endl;
				}
			}

			//Save states
			ofstream statesFile("states.txt");
			if (statesFile.is_open()) {
				for (int i = 0; i < states.size(); i++) {
					statesFile << states[i] << endl;
				}
			}
			cout << endl << "Saved!";
			otherin.detach();

			this_thread::sleep_for(50ms);
			return 0;
		}
		if ((GetAsyncKeyState(VK_RIGHT) & 0x0001) != 0) {
			cout << endl << "Exit!";

			otherin.detach();
			this_thread::sleep_for(50ms);
			return 0;
		}

		SetAddressValues();
		Ai();
		//auto t2 = high_resolution_clock::now();
		//system("cls");
		//cout << "Duration: " << duration_cast<microseconds>(t2 - t1).count() << "us";
	}
}