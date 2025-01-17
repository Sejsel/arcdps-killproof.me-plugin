﻿#include "KillproofUI.h"

#include <mutex>
#include <Windows.h>
#include <future>

#include "resource.h"
#include "global.h"
#include "Player.h"
#include "Settings.h"
#include "extension//Icon.h"
#include "Lang.h"
#include "imgui/imgui_internal.h"
#include "extension/Widgets.h"
#include "WindowSettingsUI.h"

void KillproofUI::openInBrowser(const char* username) {
	char buf[128];
	snprintf(buf, 128, "https://killproof.me/proof/%s", username);
	std::async(std::launch::async, [&buf]() {
		ShellExecuteA(nullptr, nullptr, buf, nullptr, nullptr, SW_SHOW);
	});
}

void KillproofUI::draw(bool* p_open, ImGuiWindowFlags flags) {
	// ImGui::SetNextWindowSizeConstraints(ImVec2(150, 50), ImVec2(windowWidth, windowsHeight));
	std::string title = lang.translate(LangKey::KpWindowName);
	title.append("##Killproof.me");

	flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	if (!settings.getShowHeader()) {
		flags |= ImGuiWindowFlags_NoTitleBar;
	}

	if (settings.getPosition() != Position::Manual) {
		flags |= ImGuiWindowFlags_NoMove;
	}

	ImGui::Begin(title.c_str(), p_open, flags);

	/**
	 * Settings UI
	 */
	ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
	if (ImGuiEx::BeginPopupContextWindow(nullptr, 1, ImGuiHoveredFlags_ChildWindows)) {
		windowSettingsUI.draw(table, currentWindow);

		ImGui::EndPopup();
	}

	// lock the mutexes, before we access sensible data
	std::scoped_lock<std::mutex, std::mutex> lock(trackedPlayersMutex, cachedPlayersMutex);


	/**
	* ERROR MESSAGES
	*/
	for (std::string trackedPlayer : trackedPlayers) {
		const Player& player = cachedPlayers.at(trackedPlayer);
		if (player.status == LoadingStatus::KpMeError) {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", player.errorMessage.c_str());
		}
	}

	/**
	* Controls
	*/
	if (!settings.getHideControls()) {
		bool addPlayer = false;
		if (ImGui::InputText("##useradd", userAddBuf, sizeof userAddBuf, ImGuiInputTextFlags_EnterReturnsTrue)) {
			addPlayer = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(lang.translate(LangKey::AddPlayerTooltip).c_str());
		ImGui::SameLine();
		if (ImGui::Button(lang.translate(LangKey::AddPlayerText).c_str())) {
			addPlayer = true;
		}

		if (addPlayer) {
			std::string username(userAddBuf);

			// only run when username is not empty
			if (!username.empty()) {
				// only add to tracking, if not already there
				if (std::find(trackedPlayers.begin(), trackedPlayers.end(), username) == trackedPlayers.end()) {
					trackedPlayers.emplace_back(username);

					const auto& tryEmplace = cachedPlayers.try_emplace(username, username, "", 0, true);
					if (!tryEmplace.second && tryEmplace.first->second.status == LoadingStatus::NotLoaded) {
						tryEmplace.first->second.manuallyAdded = true;
					}
					loadKillproofs(tryEmplace.first->second);
				}
				userAddBuf[0] = '\0';
			}
		}

		ImGui::SameLine();
		if (ImGui::Button(lang.translate(LangKey::ClearText).c_str())) {
			const auto end = std::remove_if(trackedPlayers.begin(), trackedPlayers.end(), [](const std::string& playerName) {
				const auto& player = cachedPlayers.find(playerName);
				if (player == cachedPlayers.end()) {
					return false;
				}
				return player->second.manuallyAdded;
			});
			trackedPlayers.erase(end, trackedPlayers.end());
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(lang.translate(LangKey::ClearTooltip).c_str());
		}

		// get own player
		const auto& playerIt = cachedPlayers.find(selfAccountName);
		if (playerIt != cachedPlayers.end()) {
			const Player& player = playerIt->second;
			if (player.status == LoadingStatus::Loaded) {
				ImGui::SameLine();

				if (ImGui::Button(lang.translate(LangKey::CopyKpIdText).c_str())) {
					// create string to copy
					std::string str;
					str.append("Killproof.me: ");
					str.append(player.killproofId);

					// copy ID to clipboard
					//put your text in source
					if (OpenClipboard(NULL)) {
						HGLOBAL clipbuffer;
						char* buffer;
						EmptyClipboard();
						clipbuffer = GlobalAlloc(GMEM_DDESHARE, str.size() + 1);
						buffer = (char*)GlobalLock(clipbuffer);
						memset(buffer, 0, str.size() + 1);
						str.copy(buffer, str.size());
						GlobalUnlock(clipbuffer);
						SetClipboardData(CF_TEXT, clipbuffer);
						CloseClipboard();
					}
				}
			}
		}
	}

	/**
	* TABLE
	*/
	Alignment alignment = settings.getAlignment();
	const int columnCount = static_cast<int>(Killproof::FINAL_ENTRY) + 2;

	if (ImGui::BeginTable("kp.me", columnCount,
	                      ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable |
	                      ImGuiTableFlags_Reorderable | ImGuiTableFlags_RowBg)) {
		table = GImGui->CurrentTable;
		ImU32 accountNameId = static_cast<ImU32>(Killproof::FINAL_ENTRY) + 1;
		ImU32 characterNameId = static_cast<ImU32>(Killproof::FINAL_ENTRY) + 2;

		std::string accountName = lang.translate(LangKey::Accountname);
		std::string charName = lang.translate(LangKey::Charactername);

		/**
		 * HEADER
		 */
		ImGui::TableSetupColumn(accountName.c_str(), ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_PreferSortDescending, 0, accountNameId);
		ImGui::TableSetupColumn(charName.c_str(), ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_PreferSortDescending, 0, characterNameId);

		for (int i = 0; i < static_cast<int>(Killproof::FINAL_ENTRY); ++i) {
			Killproof kp = static_cast<Killproof>(i);
			int columnFlags = ImGuiTableColumnFlags_PreferSortDescending;

			if (defaultHidden(kp)) {
				columnFlags |= ImGuiTableColumnFlags_DefaultHide;
			}

			ImGui::TableSetupColumn(toString(kp).c_str(), columnFlags, 0.f, static_cast<ImU32>(kp));
		}

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

		// accountname header
		if (ImGui::TableNextColumn()) {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3.f);
			ImGuiEx::TableHeader(accountName.c_str(), true, nullptr);
		}

		// charname header
		if (ImGui::TableNextColumn())
			ImGuiEx::TableHeader(charName.c_str(), true, nullptr);

		// header for killproofs
		for (int i = 0; i < static_cast<int>(Killproof::FINAL_ENTRY); ++i) {
			if (ImGui::TableNextColumn()) {
				Killproof kp = static_cast<Killproof>(i);
				std::string columnName = toString(kp);
				if (settings.getShowHeaderText()) {
					ImGuiEx::TableHeader(columnName.c_str(), true, nullptr, alignment);
				} else {
					ImGuiEx::TableHeader(columnName.c_str(), false, iconLoader.getTexture(icons.at(kp)), alignment);
				}
			}
		}

		/**
		 * SORTING
		 */
		if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
			// Sort our data if sort specs have been changed!
			if (sorts_specs->SpecsDirty)
				needSort = true;

			bool expected = true;
			if (needSort.compare_exchange_strong(expected, false)) {
				const bool descend = sorts_specs->Specs->SortDirection == ImGuiSortDirection_Descending;

				if (sorts_specs->Specs->ColumnUserID == accountNameId) {
					// sort by account name. Account name is the value we used in trackedPlayers, so nothing more to do
					std::sort(trackedPlayers.begin(), trackedPlayers.end(), [descend](std::string playerAName, std::string playerBName) {
						std::transform(playerAName.begin(), playerAName.end(), playerAName.begin(), [](unsigned char c) { return std::tolower(c); });
						std::transform(playerBName.begin(), playerBName.end(), playerBName.begin(), [](unsigned char c) { return std::tolower(c); });

						if (descend) {
							return playerAName < playerBName;
						} else {
							return playerAName > playerBName;
						}
					});
				} else if (sorts_specs->Specs->ColumnUserID == characterNameId) {
					// sort by character name
					std::sort(trackedPlayers.begin(), trackedPlayers.end(), [descend](std::string playerAName, std::string playerBName) {
						std::string playerAChar = cachedPlayers.at(playerAName).characterName;
						std::string playerBChar = cachedPlayers.at(playerBName).characterName;

						std::transform(playerAChar.begin(), playerAChar.end(), playerAChar.begin(), [](unsigned char c) { return std::tolower(c); });
						std::transform(playerBChar.begin(), playerBChar.end(), playerBChar.begin(), [](unsigned char c) { return std::tolower(c); });

						if (descend) {
							return playerAChar < playerBChar;
						} else {
							return playerAChar > playerBChar;
						}
					});
				} else {
					// sort by any amount of KP
					std::sort(trackedPlayers.begin(), trackedPlayers.end(), [sorts_specs, descend](std::string playerAName, std::string playerBName) {
						// get player object of name
						const Player& playerA = cachedPlayers.at(playerAName);
						const Player& playerB = cachedPlayers.at(playerBName);

						const amountVal amountA = playerA.killproofs.getAmountFromEnum(static_cast<const Killproof>(sorts_specs->Specs->ColumnUserID));
						const amountVal amountB = playerB.killproofs.getAmountFromEnum(static_cast<const Killproof>(sorts_specs->Specs->ColumnUserID));

						// descending: the more the higher up it gets
						if (descend) {
							return amountA > amountB;
						} else {
							return amountA < amountB;
						}
					});
				}
				sorts_specs->SpecsDirty = false;
			}
		}

		// List of all players
		for (const std::string& trackedPlayer : trackedPlayers) {
			const Player& player = cachedPlayers.at(trackedPlayer);

			// hide player without data, when setting is active
			if (!(settings.getHidePrivateAccount() && player.status == LoadingStatus::NoDataAvailable)) {
				bool open = drawRow(alignment, player.username.c_str(), player.characterName.c_str(), player.status, player.killproofs, player.coffers,
				                    player.linkedTotalKillproofs.has_value(), player.commander);
				if (open) {
					for (std::string linkedAccount : player.linkedAccounts) {
						Player& linkedPlayer = cachedPlayers.at(linkedAccount);
						drawRow(alignment, linkedPlayer.username.c_str(), linkedPlayer.characterName.c_str(), LoadingStatus::Loaded, linkedPlayer.killproofs,
						        linkedPlayer.coffers, false, false);
					}
					drawRow(alignment, lang.translate(LangKey::Overall).c_str(), "--->", player.status, player.linkedTotalKillproofs.value(), player.linkedTotalCoffers.value(), false, false);

					ImGui::TreePop();
				}
			}
		}

		ImGui::EndTable();
	}

	/**
	 * Reposition Window
	 */
	ImGuiEx::WindowReposition(settings.getPosition(), settings.getCornerVector(), settings.getCornerPosition(), settings.getFromWindowID(),
	                          settings.getAnchorPanelCornerPosition(), settings.getSelfPanelCornerPosition());

	ImGui::End();
}

bool KillproofUI::drawRow(const Alignment& alignment, const char* username, const char* characterName, const std::atomic<LoadingStatus>& status,
                          const Killproofs& killproofs, const Coffers& coffers, bool treeNode, bool isCommander) {
	ImGui::TableNextRow();

	bool open = false;
	// username
	if (ImGui::TableNextColumn()) {
		if (isCommander && settings.getShowCommander()) {
			auto* icon = iconLoader.getTexture(ID_Commander_White);
			if (icon) {
				float size = ImGui::GetFontSize();
				ImGui::Image(icon, ImVec2(size, size));
				ImGui::SameLine();
			}
		}

		if (treeNode) {
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));

			ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;
			if (settings.getShowOverallByDefault()) {
				treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
			}

			open = ImGui::TreeNodeEx(username, treeNodeFlags);
			ImGui::PopStyleVar();
		} else {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3.f);
			ImGui::TextUnformatted(username);
			if (status == LoadingStatus::Loaded && ImGui::IsItemClicked()) {
				// Open users kp.me in the browser
				openInBrowser(username);
			}
		}
	}

	// charactername
	if (ImGui::TableNextColumn()) {
		ImGui::Text(characterName);
		if (status == LoadingStatus::Loaded && ImGui::IsItemClicked()) {
			// Open users kp.me in the browser
			openInBrowser(username);
		}
	}

	for (int i = 0; i < static_cast<int>(Killproof::FINAL_ENTRY); ++i) {
		if (ImGui::TableNextColumn()) {
			Killproof kp = static_cast<Killproof>(i);
			amountVal killproofAmount = killproofs.getAmountFromEnum(kp);
			amountVal cofferAmount = coffers.getAmount(kp);
			amountVal totalAmount = killproofAmount;
			if (kp == Killproof::li || kp == Killproof::ld || kp == Killproof::liLd) {
				totalAmount += cofferAmount;
			} else {
				totalAmount += cofferAmount * settings.getCofferValue();
			}
			if (status == LoadingStatus::LoadingById || status == LoadingStatus::LoadingByChar) {
				ImGuiEx::SpinnerAligned("loadingSpinner", ImGui::GetTextLineHeight() / 4.f, 1.f, ImGui::GetColorU32(ImGuiCol_Text), settings.getAlignment());
			} else if (totalAmount == -1 || status != LoadingStatus::Loaded) {
				ImGuiEx::AlignedTextColumn(alignment, "%s", settings.getBlockedDataText().c_str());
			} else {
				ImGuiEx::AlignedTextColumn(alignment, "%i", totalAmount);

				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					std::string kpText = std::format("{}: {}", lang.translate(LangKey::Killproofs), killproofAmount);
					ImGui::TextUnformatted(kpText.c_str());
					std::string cofferText = std::format("{}: {}", lang.translate(LangKey::Coffers), cofferAmount);
					ImGui::TextUnformatted(cofferText.c_str());
					ImGui::EndTooltip();
				}
			}
		}
	}

	return open;
}
