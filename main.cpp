#include "stuff.h"
#include <thread>

int main() {
	sapi.baseOffset = 0x3222D0;
	sapi.moduleName = L"GeometryDash.exe";
	sapi.windowName = "Geometry Dash";
	sapi.GetBaseAddress();

	xposAddress = sapi.GetAddressByOffset(xposOffset);
	yposAddress = sapi.GetAddressByOffset(yposOffset);
	currentGMAddress = sapi.GetAddressByOffset(gmOffset);
	percentageAddress = sapi.GetAddressByOffset(percentageOffset);
	attemptsAddress = sapi.GetAddressByOffset(attemptsOffset);

	Start();
	std::cout << "Started\n";
	std::thread oni(OtherIn);

	while (true) {
		//Exit with saving, or without
		if ((GetAsyncKeyState(VK_LEFT) & 0x0001) != 0) {
			if (ai.checkpoints.size() >= 1) {
				for (int i = 0; i < ai.checkpoints.size() - 1; i++) {
					ai.checkpoints[i] = 0;
				}
			}
			sapi.SaveVectors(ai.chance, "chances.txt");
			sapi.SaveVectors(ai.checkpoints, "checkpoints.txt");
			sapi.SaveVectors(ai.actions, "actions.txt");
			std::cout << "Saved!";
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			oni.detach();
			return 0;
		}
		if ((GetAsyncKeyState(VK_RIGHT) & 0x0001) != 0) {
			std::cout << "Exit";
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			oni.detach();
			return 0;
		}

		lxpos = xpos;
		xpos = sapi.ReadAddress(xposAddress, 3.4f) / blockSize;
		percentageStr = sapi.ReadAddress(percentageAddress, percentageStr);

		lypos = ypos;
		ypos = sapi.ReadAddress(yposAddress, ypos);
		currentGM = sapi.ReadAddress(currentGMAddress, currentGM);

		//Check if air and if dead
		isAir = (ypos != lypos);
		isDead = (currentGM >= dead);

		//Dead stuff
		if (isDead && !checkedDead) {
			checkedDead = true;
			vlDeathPos = lDeathPos;
			lDeathPos = xpos;

			if (lDeathPos == vlDeathPos) {
				cDeathAmount++;
			}
			else {
				cDeathAmount -= 3;
				if (cDeathAmount < 0) {
					cDeathAmount = 0;
				}
			}
			if (cDeathAmount >= ai.deathAmountToMut) {
				for (int i = 0; i < ai.rewardDistance; i++) {
					if (ai.chance[lDeathPos - i] <= 50) {
						ai.chance[lDeathPos - i] += ai.deathMutateFail;
						if (ai.chance[lDeathPos - i] > ai.rewardMax) {
							ai.chance[lDeathPos - i] = ai.rewardMax;
						}
					}
					else {
						ai.chance[lDeathPos - i] -= ai.deathMutateFail;
						if (ai.chance[lDeathPos - i] < ai.rewardMin) {
							ai.chance[lDeathPos - i] = ai.rewardMin;
						}
					}
				}
			}
			if (cDeathAmount >= ai.deathAmountToRemoveCheckpoint) {
				sapi.Input(VkKeyScan('x'), 0);
				sapi.Input(VkKeyScan('x'), 1);
			}
		}
		else if (!isDead && checkedDead) {
			checkedDead = false;
		}

		if (xpos >= furthestXpos) {
			furthestXpos = xpos;
		}

		std::string temp;
		if (percentageStr != "" && percentageStr.length() <= 4) {
			for (char c : percentageStr) {
				if (c != '%') {
					temp += c;
				}
			}
			percentage = std::stoi(temp);
		}

		if (percentage >= 100 && !checkedLevelComplete) {
			attempts = sapi.ReadAddress(attemptsAddress, attempts);

			checkedLevelComplete = true;
			levelCompleteTimes++;
			auto endTime = highResClock::now();
			if (ai.checkpoints.size() >= 1) {
				for (int i = 0; i < ai.checkpoints.size() - 1; i++) {
					ai.checkpoints[i] = 0;
				}
			}
			std::cout << "\nComplete!\nLevel complete times: " << levelCompleteTimes << std::endl;
			std::cout << "Attempts: " << attempts << "\n";
			std::string hours, mins, seconds;
			if (std::chrono::duration_cast<std::chrono::hours>(endTime - startTime).count() > 9) {
				hours = std::to_string(std::chrono::duration_cast<std::chrono::hours>(endTime - startTime).count());
			}
			else {
				hours = "0" + std::to_string(std::chrono::duration_cast<std::chrono::hours>(endTime - startTime).count());
			}
			if (std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime).count() > 9) {
				mins = std::to_string(std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime).count());
			}
			else {
				mins = "0" + std::to_string(std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime).count());
			}
			if (std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count() > 9) {
				seconds = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count());
			}
			else {
				seconds = "0" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count());
			}
			std::cout << "Time: " << hours << ":" << mins << ":" << seconds << "\n";
			if (allowRestart) {
				std::this_thread::sleep_for(std::chrono::seconds(6));
				sapi.Input(VK_SPACE, 0);
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
				sapi.Input(VK_SPACE, 1);
				sapi.Input(VK_SPACE, 0);
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
				sapi.Input(VK_SPACE, 1);
			}
			furthestXpos = 0;
			startTime = highResClock::now();
		}
		else if (percentage < 100 && checkedLevelComplete) {
			checkedLevelComplete = false;
		}

		//Update every new block
		if (xpos != lxpos) {
			ai.Play();
		}
	}
}