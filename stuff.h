#pragma once
#include "SkadooshAPI.h"
#include "stuff.h"
#include <iostream>
#include <thread>
#include <chrono>
#define cube 0
#define ship 1
#define ufo 256
#define ball 65536
#define wave 16777216
#define robot 4294967296
#define spider 1099511627776
#define upsideDown 281474976710656
#define dead 72057594037927936

typedef std::chrono::high_resolution_clock highResClock;
auto startTime = highResClock::now();

// Variables

SkadooshAPI sapi;

std::vector<DWORD> xposOffset = { 0x164, 0x224, 0x67C };
std::vector<DWORD> yposOffset = { 0x164, 0x224, 0x680 };
std::vector<DWORD> gmOffset = { 0x164, 0x224, 0x638 };
std::vector<DWORD> percentageOffset = { 0x164, 0x124, 0xEC, 0x2A4, 0x12C };
std::vector<DWORD> attemptsOffset = { 0x164, 0xe8, 0x8, 0x4a8 };

std::vector<long double> defaultConfigVectors { 50, 20, 10, 25, 12, 100, 0, 15, 30, 10 };
std::vector<std::string> configTooltips {
	"The default chance value of a new position.",
	"The distance behind the furthest position the ai should place a checkpoint.",
	"The amount of times to die at the same place to mutate.",
	"The amount the ai should reward the action it did at the current position - rewardDistance.",
	"The distance the ai should reward the action it did this current position - this.",
	"The max amount the chance can have.",
	"The min amount the chance can have.",
	"The amount of times to die at the same place to remove a checkpoint.",
	"The size of a block (normal sized block is 30).",
	"The amount to mutate the ai on same place death mutations."
};

std::string percentageStr;
int xpos = 0, lxpos, ypos = 0, lypos, furthestXpos = 0, percentage, levelCompleteTimes;
int blockSize = 30;
unsigned long long currentGM = 0;

int cDeathAmount, lDeathPos, vlDeathPos, attempts;
bool isAir, isDead, checkedDead, checkedLevelComplete = false;
bool aiDisabled, allowCheckpoints = true, guideMode = false, allowRestart = true;

int xposAddress = sapi.GetAddressByOffset(xposOffset);
int yposAddress = sapi.GetAddressByOffset(yposOffset);
int currentGMAddress = sapi.GetAddressByOffset(gmOffset);
int percentageAddress = sapi.GetAddressByOffset(percentageOffset);
int attemptsAddress = sapi.GetAddressByOffset(attemptsOffset);

class Ai {
public:
	int reward, rewardMin, rewardMax, rewardDistance, checkpointDistance, baseChance, deathAmountToMut, deathAmountToRemoveCheckpoint, deathMutateFail;
	std::vector<float> chance;
	std::vector<int> actions;
	std::vector<int> checkpoints;

	void Play() {
		//Resize stuff
		while (xpos >= chance.size() || xpos >= actions.size()) {
			chance.resize(chance.size() + 1);
			actions.resize(actions.size() + 1);
			checkpoints.resize(actions.size() + 1);

			chance[chance.size() - 1] = baseChance;
		}

		if (!aiDisabled) {
			//Reward
			if (xpos - rewardDistance >= 0) {
				if (actions[xpos - rewardDistance] == 0) {
					chance[xpos - rewardDistance] += reward;
					if (chance[xpos - rewardDistance] > rewardMax) {
						chance[xpos - rewardDistance] = rewardMax;
					}
				}
				else {
					chance[xpos - rewardDistance] -= reward;
					if (chance[xpos - rewardDistance] < rewardMin) {
						chance[xpos - rewardDistance] = rewardMin;
					}
				}
			}

			//Input
			if (rand() % 100 <= chance[xpos]) {
				actions[xpos] = 0;
				sapi.Input(VK_SPACE, 0);
			}
			else {
				actions[xpos] = 1;
				sapi.Input(VK_SPACE, 1);
			}

			//Set checkpoint
			if (xpos == furthestXpos - checkpointDistance && allowCheckpoints) {
				if (checkpoints[xpos] == 0 && !isAir && currentGM != ship && currentGM != ship + upsideDown) {
					if (chance[xpos] >= rewardMax || chance[xpos] <= rewardMin) {
						checkpoints[xpos] = 1;
						sapi.Input(VkKeyScan('z'), 0);
						sapi.Input(VkKeyScan('z'), 1);
					}
				}
			}
		}
		else if (guideMode) {
			if (GetAsyncKeyState(VK_RBUTTON)) {
				actions[xpos] = 0;
				chance[xpos] += reward;
				sapi.Input(VK_SPACE, 0);
			}
			else {
				actions[xpos] = 1;
				chance[xpos] -= reward;
				sapi.Input(VK_SPACE, 1);
			}
		}
	}
};

Ai ai;

void OtherIn() {
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if ((GetAsyncKeyState(VkKeyScan('p')) & 0x0001) != 0) {
			if (aiDisabled) {
				aiDisabled = false;
				std::cout << "Ai Enabled!\n";
			}
			else {
				aiDisabled = true;
				std::cout << "Ai Disabled!\n";
			}
		}
		if ((GetAsyncKeyState(VkKeyScan('o')) & 0x0001) != 0) {
			if (allowCheckpoints) {
				allowCheckpoints = false;
				std::cout << "Checkpoints Disabled!\n";
			}
			else {
				allowCheckpoints = true;
				std::cout << "Checkpoints Enabled!\n";
			}
		}
		if ((GetAsyncKeyState(VkKeyScan('i')) & 0x0001) != 0) {
			std::cout << "Getting addresses...";
			sapi.GetBaseAddress();
			xposAddress = sapi.GetAddressByOffset(xposOffset);
			yposAddress = sapi.GetAddressByOffset(yposOffset);
			currentGMAddress = sapi.GetAddressByOffset(gmOffset);
			percentageAddress = sapi.GetAddressByOffset(percentageOffset);
			std::cout << "Done!\n";
		}
		if ((GetAsyncKeyState(VkKeyScan('u')) & 0x0001) != 0) {
			if (guideMode) {
				guideMode = false;
				std::cout << "Guidemode Disabled!\n";
			}
			else {
				guideMode = true;
				aiDisabled = true;
				std::cout << "Guidemode Enabled!\n";
			}
		}
		if ((GetAsyncKeyState(VkKeyScan('r')) & 0x0001) != 0) {
			std::cout << "Resetting...";
			for (int i = 0; i < ai.actions.size() - 1; i++) {
				ai.actions[i] = 0;
				ai.chance[i] = ai.baseChance;
				ai.checkpoints[i] = 0;
			}
			std::cout << "Done!\n";
		}
		if ((GetAsyncKeyState(VkKeyScan('y')) & 0x0001) != 0) {
			if (allowRestart) {
				allowRestart = false;
				std::cout << "Auto-Restart Disabled!\n";
			}
			else {
				allowRestart = true;
				std::cout << "Auto-Restart Enabled!\n";
			}
		}
	}
}


void Start() {
	startTime = highResClock::now();
	std::cout << "Controls:\n";
	std::cout << "-'p' enable/disable ai\n";
	std::cout << "-'o' enable/disable placing checkpoints\n";
	std::cout << "-'i' gets the base address (use if the ai isn't working)\n";
	std::cout << "-'u' enable/disable guidemode\n";
	std::cout << "-'r' reset data\n";
	std::cout << "-'y' enable/disable auto-restart\n\n";

	//Check if config is correct
	bool configTest = false;
	if (sapi.FindTextInFile(configTooltips[configTooltips.size()- 1], "config.txt")) {
		configTest = true;
	}

	//Create/load config
	if (sapi.FileExists("config.txt") && configTest) {
		std::vector<long double> configVec = sapi.LoadConfig("config.txt");

		ai.baseChance = configVec[0];
		ai.checkpointDistance = configVec[1];
		ai.deathAmountToMut = configVec[2];
		ai.reward = configVec[3];
		ai.rewardDistance = configVec[4];
		ai.rewardMax = configVec[5];
		ai.rewardMin = configVec[6];
		ai.deathAmountToRemoveCheckpoint = configVec[7];
		blockSize = configVec[8];
		ai.deathMutateFail = configVec[9];

		std::cout << "Config values:\n";
		std::cout << "ai.baseChance = " << ai.baseChance << "\n";
		std::cout << "ai.checkpointDistance = " << ai.checkpointDistance << "\n";
		std::cout << "ai.deathAmountToMut = " << ai.deathAmountToMut << "\n";
		std::cout << "ai.reward = " << ai.reward << "\n";
		std::cout << "ai.rewardDistance = " << ai.rewardDistance << "\n";
		std::cout << "ai.rewardMax = " << ai.rewardMax << "\n";
		std::cout << "ai.rewardMin = " << ai.rewardMin << "\n";
		std::cout << "ai.deathAmountToRemoveCheckpoint = " << ai.deathAmountToRemoveCheckpoint << "\n";
		std::cout << "blockSize = " << blockSize << "\n";
		std::cout << "ai.deathMutateFail = " << ai.deathMutateFail << "\n\n";
	}
	else {
		sapi.configVectors = defaultConfigVectors;
		sapi.configTooltips = configTooltips;
		sapi.SaveConfig("config.txt");

		std::vector<long double> configVec = sapi.LoadConfig("config.txt");

		ai.baseChance = configVec[0];
		ai.checkpointDistance = configVec[1];
		ai.deathAmountToMut = configVec[2];
		ai.reward = configVec[3];
		ai.rewardDistance = configVec[4];
		ai.rewardMax = configVec[5];
		ai.rewardMin = configVec[6];
		ai.deathAmountToRemoveCheckpoint = configVec[7];
		blockSize = configVec[8];
		ai.deathMutateFail = configVec[9];

		std::cout << "Config values:\n";
		std::cout << "ai.baseChance = " << ai.baseChance << "\n";
		std::cout << "ai.checkpointDistance = " << ai.checkpointDistance << "\n";
		std::cout << "ai.deathAmountToMut = " << ai.deathAmountToMut << "\n";
		std::cout << "ai.reward = " << ai.reward << "\n";
		std::cout << "ai.rewardDistance = " << ai.rewardDistance << "\n";
		std::cout << "ai.rewardMax = " << ai.rewardMax << "\n";
		std::cout << "ai.rewardMin = " << ai.rewardMin << "\n";
		std::cout << "ai.deathAmountToRemoveCheckpoint = " << ai.deathAmountToRemoveCheckpoint << "\n";
		std::cout << "blockSize = " << blockSize << "\n";
		std::cout << "ai.deathMutateFail = " << ai.deathMutateFail << "\n\n";
	}

	std::cout << "\nAi Enabled!\nCheckpoints Enabled!\nGuidemode Disabled!\nAuto-Restart Enabled!\n";
	if (sapi.FileExists("actions.txt") && sapi.FileExists("checkpoints.txt") && sapi.FileExists("chances.txt")) {
		char input;
		std::cout << "Do you want to load the current vectors? (current dir) y/n: ";
		std::cin >> input;

		if (input == 'y') {
			std::vector<long double> tmpAction = sapi.LoadVectors("actions.txt");
			std::vector<long double> tmpCheckpoint = sapi.LoadVectors("checkpoints.txt");
			std::vector<long double> tmpChance = sapi.LoadVectors("chances.txt");

			ai.chance =  std::vector<float>(tmpChance.begin(), tmpChance.end());
			ai.actions = std::vector<int> (tmpAction.begin(), tmpAction.end());
			ai.checkpoints = std::vector<int> (tmpCheckpoint.begin(), tmpCheckpoint.end());
		}
	}
}