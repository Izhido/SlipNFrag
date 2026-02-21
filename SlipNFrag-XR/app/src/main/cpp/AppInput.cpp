#include "AppInput.h"
#include "AppState_xr.h"
#include "in_xr.h"
#include "Utils.h"
#include "Locks.h"
#include <fstream>
#include <minizip/unzip.h>
#include <lhasa.h>
#include <Logger.h>
#include <sys/stat.h>

extern m_state_t m_state;

std::vector<AppInput> AppInput::inputQueue(8);
int AppInput::lastInputQueueItem = -1;

void AppInput::AddKeyInput(int key, int down)
{
	std::lock_guard<std::mutex> lock(Locks::InputMutex);

	lastInputQueueItem++;
	if (lastInputQueueItem >= inputQueue.size())
	{
		inputQueue.emplace_back();
	}
	auto& entry = inputQueue[lastInputQueueItem];
	entry.key = key;
	entry.down = down;
	entry.command.clear();
}

void AppInput::AddCommandInput(const char* command)
{
	std::lock_guard<std::mutex> lock(Locks::InputMutex);

	lastInputQueueItem++;
	if (lastInputQueueItem >= inputQueue.size())
	{
		inputQueue.emplace_back();
	}
	auto& entry = inputQueue[lastInputQueueItem];
	entry.key = 0;
	entry.down = false;
	entry.command = command;
}

void AppInput::Handle(AppState_xr& appState, bool keyPressHandled, const char* basedir)
{
	XrActionStateGetInfo actionGetInfo { XR_TYPE_ACTION_STATE_GET_INFO };
	XrActionStateBoolean booleanActionState { XR_TYPE_ACTION_STATE_BOOLEAN };
	XrActionStateFloat floatActionState { XR_TYPE_ACTION_STATE_FLOAT };

	auto hand = Cvar_VariableString ("dominant_hand");
	auto isLeftHanded = (Q_strncmp(hand, "left", 4) == 0);

	const char* sharewarePath = "quake106.zip";

	if (appState.Mode == AppStartupMode)
	{
		if (!appState.StartupButtonsPressed)
		{
			auto count = 0;
			actionGetInfo.action = appState.Actions.Play1;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			actionGetInfo.action = appState.Actions.Play2;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			if (count > 1)
			{
				appState.StartupButtonsPressed = true;
				appState.SharewareExists = appState.FileLoader->Exists(sharewarePath);
				if (appState.SharewareExists)
				{
					auto isEmpty = true;
					auto isDamaged = true;
					unsigned short pak0DirCRC = 0;
					const auto pak0Path = std::string(basedir) + "/id1/pak0.pak";
					std::ifstream pak0File(pak0Path, std::ios::binary);
					if (pak0File)
					{
						isEmpty = false;
						pak0File.seekg(0, std::ios::end);
						const auto pak0Size = pak0File.tellg();
						if (pak0Size > 12)
						{
							std::vector<unsigned char> contents;
							contents.resize(12);
							pak0File.seekg(0, std::ios::beg);
							pak0File.read((char*)contents.data(), 12);
							if (contents[0] == 'P' && contents[1] == 'A' && contents[2] == 'C' && contents[3] == 'K')
							{
								int dirofs = ((int)contents[4]) | (((int)contents[5]) << 8) | (((int)contents[6]) << 16) | (((int)contents[7]) << 24);
								int dirlen = ((int)contents[8]) | (((int)contents[9]) << 8) | (((int)contents[10]) << 16) | (((int)contents[11]) << 24);
								if (dirofs >= 12 && dirlen > 0 && (dirlen % 64) == 0 && (dirofs + dirlen) <= pak0Size)
								{
									contents.resize(dirlen);
									pak0File.seekg(dirofs, std::ios::beg);
									pak0File.read((char*)contents.data(), dirlen);
									pak0File.close();
									CRC_Init (&pak0DirCRC);
									for (auto b : contents)
									{
										CRC_ProcessByte (&pak0DirCRC, b);
									}
									const auto pak1Path = std::string(basedir) + "/id1/pak1.pak";
									std::ifstream pak1File(pak1Path, std::ios::binary);
									if (pak1File)
									{
										pak1File.seekg(0, std::ios::end);
										const auto pak1Size = pak1File.tellg();
										if (pak1Size > 12)
										{
											std::vector<unsigned char> pak1Contents;
											pak1Contents.resize(pak1Size);
											pak1File.seekg(0, std::ios::beg);
											pak1File.read((char*)pak1Contents.data(), pak1Size);
											pak1File.close();
											if (pak1Contents[0] == 'P' && pak1Contents[1] == 'A' && pak1Contents[2] == 'C' && pak1Contents[3] == 'K')
											{
												dirofs = ((int)pak1Contents[4]) | (((int)pak1Contents[5]) << 8) | (((int)pak1Contents[6]) << 16) | (((int)pak1Contents[7]) << 24);
												dirlen = ((int)pak1Contents[8]) | (((int)pak1Contents[9]) << 8) | (((int)pak1Contents[10]) << 16) | (((int)pak1Contents[11]) << 24);
												if (dirofs >= 12 && dirlen > 0 && (dirlen % 64) == 0 && (dirofs + dirlen) <= pak1Size)
												{
													auto dirlast = dirofs + dirlen;
													auto i = dirofs;
													char name[57];
													name[56] = 0;
													int popofs = -1;
													int poplen = -1;
													do
													{
														std::copy(pak1Contents.begin() + i, pak1Contents.begin() + i + 56, name);
														if (strncmp(name, "gfx/pop.lmp", 11) == 0)
														{
															popofs = ((int)pak1Contents[i + 56]) | (((int)pak1Contents[i + 57]) << 8) | (((int)pak1Contents[i + 58]) << 16) | (((int)pak1Contents[i + 59]) << 24);
															poplen = ((int)pak1Contents[i + 60]) | (((int)pak1Contents[i + 61]) << 8) | (((int)pak1Contents[i + 62]) << 16) | (((int)pak1Contents[i + 63]) << 24);
															break;
														}
														i += 64;
													} while (i < dirlast);
													if (popofs < 0 && poplen < 0)
													{
														isDamaged = false;
													}
													else if (popofs >= 12 && poplen > 0 && (popofs + poplen) <= pak1Size)
													{
														isDamaged = false;
														if (poplen == 256)
														{
															extern unsigned short pop[];
															int j = 0;
															for (i = 0; i < 256; i += 2)
															{
																auto p = (((unsigned short)pak1Contents[popofs + i]) << 8) | ((unsigned short)pak1Contents[popofs + i + 1]);
																if (p != pop[j])
																{
																	break;
																}
																j++;
															}
															if (j == 128)
															{
																appState.IsRegistered = true;
															}
														}
													}
												}
											}
										}
									}
									else
									{
										isDamaged = false;
									}
								}
							}
						}
					}
					if (appState.IsRegistered)
					{
						appState.DestroyImageSources();
						appState.Mode = AppWorldMode;
					}
					else if (!isEmpty && !isDamaged)
					{
						if (pak0DirCRC == 32981)
						{
							appState.Mode = AppSharewareGameDataMode;
						}
						else
						{
							appState.Mode = AppInvalidGameDataUncompressMode;
						}
					}
					else if (isEmpty)
					{
						appState.Mode = AppNoGameDataUncompressMode;
					}
					else
					{
						appState.Mode = AppInvalidGameDataUncompressMode;
					}
				}
				else
				{
					appState.Mode = AppWorldMode;
				}
			}
		}
	}
	else if (appState.Mode == AppSharewareGameDataMode)
	{
		if (!appState.SharewareGameDataButtonsPressed)
		{
			auto count = 0;
			actionGetInfo.action = appState.Actions.Play1;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			actionGetInfo.action = appState.Actions.Play2;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			if (count > 1)
			{
				if (!appState.StartupButtonsPressed)
				{
					appState.SharewareGameDataButtonsPressed = true;
					appState.DestroyImageSources();
					appState.Mode = AppWorldMode;
				}
			}
			else
			{
				appState.StartupButtonsPressed = false;
			}
		}
	}
	else if (appState.Mode == AppNoGameDataUncompressMode || appState.Mode == AppInvalidGameDataUncompressMode)
	{
		if (!appState.UncompressButtonsPressed)
		{
			auto count = 0;
			actionGetInfo.action = appState.Actions.Play1;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			actionGetInfo.action = appState.Actions.Play2;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			if (count > 1)
			{
				if (!appState.StartupButtonsPressed)
				{
					appState.UncompressButtonsPressed = true;

					std::vector<unsigned char> sharewareContents;
					appState.FileLoader->Load(sharewarePath, sharewareContents);
					std::string sharewareLocalPath = std::string(basedir) + "/" + sharewarePath;
					std::ofstream sharewareFile(sharewareLocalPath, std::ios::binary);
					sharewareFile.write((char*)sharewareContents.data(), sharewareContents.size());
					sharewareFile.close();
					unzFile sharewareZipFile = unzOpen(sharewareLocalPath.c_str());
					unz_global_info sharewareGlobalInfo;
					unzGetGlobalInfo(sharewareZipFile, &sharewareGlobalInfo);
					for (uLong i = 0; i < sharewareGlobalInfo.number_entry; i++)
					{
						unz_file_info localFileInfo;
						unzGetCurrentFileInfo(sharewareZipFile, &localFileInfo, NULL, 0, NULL, 0, NULL, 0);
						std::vector<char> localFilename(localFileInfo.size_filename);
						unzGetCurrentFileInfo(sharewareZipFile, &localFileInfo, localFilename.data(), localFileInfo.size_filename, NULL, 0, NULL, 0);
						if (localFilename.back() == '/')
						{
							continue;
						}
						unzOpenCurrentFile(sharewareZipFile);
						std::ofstream localFile(std::string(basedir) + "/" + std::string(localFilename.data(), localFilename.size()), std::ios::binary);
						std::vector<char> localFileContents(1024 * 1024);
						int localFileReadBytes;
						do
						{
							localFileReadBytes = unzReadCurrentFile(sharewareZipFile, localFileContents.data(), localFileContents.size());
							if (localFileReadBytes > 0)
							{
								localFile.write(localFileContents.data(), localFileReadBytes);
							}
						} while (localFileReadBytes > 0);
						unzCloseCurrentFile(sharewareZipFile);
						if (i < sharewareGlobalInfo.number_entry - 1)
						{
							unzGoToNextFile(sharewareZipFile);
						}
					}
					unzClose(sharewareZipFile);

					std::string resource1Path = std::string(basedir) + "/resource.1";
					auto resource1File = fopen(resource1Path.c_str(), "rb");
					auto resource1Stream = lha_input_stream_from_FILE(resource1File);
					auto resource1Reader = lha_reader_new(resource1Stream);
					auto resource1Header = lha_reader_next_file(resource1Reader);
					while (resource1Header != NULL)
					{
						std::string localSubdirectory;
						if (resource1Header->path != NULL)
						{
							localSubdirectory = std::string("/") + resource1Header->path;
							mkdir((std::string(basedir) + localSubdirectory).c_str(), 0770);
							if (localSubdirectory.back() == '/')
							{
								localSubdirectory.pop_back();
							}
						}
						std::string localPath = std::string(basedir) + localSubdirectory + "/" + resource1Header->filename;
						lha_reader_extract(resource1Reader, (char*)localPath.c_str(), NULL, NULL);
						resource1Header = lha_reader_next_file(resource1Reader);
					}
					lha_reader_free(resource1Reader);
					lha_input_stream_free(resource1Stream);
					fclose(resource1File);

					appState.DestroyImageSources();
					appState.Mode = AppWorldMode;
				}
			}
			else
			{
				appState.StartupButtonsPressed = false;
			}
		}
	}
	else if (appState.Mode == AppScreenMode)
	{
		if (host_initialized)
		{
			if (m_state == m_quit)
			{
				actionGetInfo.action = appState.Actions.EscapeNonY;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						AddKeyInput(K_ESCAPE, true);
					}
					else
					{
						AddKeyInput(K_ESCAPE, false);
					}
				}
				actionGetInfo.action = appState.Actions.Quit;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						AddKeyInput('y', true);
					}
					else
					{
						AddKeyInput('y', false);
					}
				}
			}
			else
			{
				actionGetInfo.action = appState.Actions.EscapeY;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						AddKeyInput(K_ESCAPE, true);
					}
					else
					{
						AddKeyInput(K_ESCAPE, false);
					}
				}
			}
			auto escapeDown = false;
			auto escapeUp = false;
			actionGetInfo.action = appState.Actions.Menu;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
			{
				if (booleanActionState.currentState)
				{
					escapeDown = true;
				}
				else
				{
					escapeUp = true;
				}
			}
			if (isLeftHanded)
			{
				actionGetInfo.action = appState.Actions.MenuLeftHanded;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						escapeDown = true;
					}
					else
					{
						escapeUp = true;
					}
				}
			}
			else
			{
				actionGetInfo.action = appState.Actions.MenuRightHanded;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						escapeDown = true;
					}
					else
					{
						escapeUp = true;
					}
				}
			}
			if (escapeDown)
			{
				AddKeyInput(K_ESCAPE, true);
			}
			if (escapeUp)
			{
				AddKeyInput(K_ESCAPE, false);
			}
			auto thumbstick = appState.PreviousThumbstick;
			actionGetInfo.action = appState.Actions.MoveX;
			CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
			if (floatActionState.isActive && floatActionState.changedSinceLastSync)
			{
				thumbstick.x = -floatActionState.currentState;
			}
			actionGetInfo.action = appState.Actions.MoveY;
			CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
			if (floatActionState.isActive && floatActionState.changedSinceLastSync)
			{
				thumbstick.y = -floatActionState.currentState;
			}
			if (thumbstick.y > 0.5 && appState.PreviousThumbstick.y <= 0.5)
			{
				AddKeyInput(K_DOWNARROW, true);
			}
			if (thumbstick.y <= 0.5 && appState.PreviousThumbstick.y > 0.5)
			{
				AddKeyInput(K_DOWNARROW, false);
			}
			if (thumbstick.y < -0.5 && appState.PreviousThumbstick.y >= -0.5)
			{
				AddKeyInput(K_UPARROW, true);
			}
			if (thumbstick.y >= -0.5 && appState.PreviousThumbstick.y < -0.5)
			{
				AddKeyInput(K_UPARROW, false);
			}
			if (thumbstick.x > 0.5 && appState.PreviousThumbstick.x <= 0.5)
			{
				AddKeyInput(K_LEFTARROW, true);
			}
			if (thumbstick.x <= 0.5 && appState.PreviousThumbstick.x > 0.5)
			{
				AddKeyInput(K_LEFTARROW, false);
			}
			if (thumbstick.x < -0.5 && appState.PreviousThumbstick.x >= -0.5)
			{
				AddKeyInput(K_RIGHTARROW, true);
			}
			if (thumbstick.x >= -0.5 && appState.PreviousThumbstick.x < -0.5)
			{
				AddKeyInput(K_RIGHTARROW, false);
			}
			appState.PreviousThumbstick = thumbstick;
			auto enterDown = false;
			auto enterUp = false;
			if (!keyPressHandled)
			{
				actionGetInfo.action = appState.Actions.EnterTrigger;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						enterDown = true;
					}
					else
					{
						enterUp = true;
					}
				}
			}
			if (isLeftHanded)
			{
				actionGetInfo.action = appState.Actions.EnterNonTriggerLeftHanded;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						enterDown = true;
					}
					else
					{
						enterUp = true;
					}
				}
			}
			else
			{
				actionGetInfo.action = appState.Actions.EnterNonTriggerRightHanded;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						enterDown = true;
					}
					else
					{
						enterUp = true;
					}
				}
			}
			if (enterDown)
			{
				AddKeyInput(K_ENTER, true);
			}
			if (enterUp)
			{
				AddKeyInput(K_ENTER, false);
			}
			// The following actions are performed only to reset any in-game actions
			// after a transition occurs - this is why only currentState == false is checked:
			if (!keyPressHandled)
			{
				actionGetInfo.action = appState.Actions.Fire;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && !booleanActionState.currentState)
				{
					AddCommandInput("-attack");
				}
			}
			actionGetInfo.action = appState.Actions.Run;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && !booleanActionState.currentState)
			{
				AddCommandInput("-speed");
			}
		}
	}
	else if (appState.Mode == AppWorldMode)
	{
		if (host_initialized)
		{
			if (key_dest == key_game)
			{
				if (m_state == m_quit)
				{
					actionGetInfo.action = appState.Actions.Quit;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddKeyInput('y', true);
						}
						else
						{
							AddKeyInput('y', false);
						}
					}
				}
				else if (isLeftHanded)
				{
					actionGetInfo.action = appState.Actions.JumpLeftHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddCommandInput("+jump");
						}
						else
						{
							AddCommandInput("-jump");
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.Actions.JumpRightHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddCommandInput("+jump");
						}
						else
						{
							AddCommandInput("-jump");
						}
					}
				}
				auto escapeDown = false;
				auto escapeUp = false;
				actionGetInfo.action = appState.Actions.Menu;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						escapeDown = true;
					}
					else
					{
						escapeUp = true;
					}
				}
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.Actions.MenuLeftHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							escapeDown = true;
						}
						else
						{
							escapeUp = true;
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.Actions.MenuRightHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							escapeDown = true;
						}
						else
						{
							escapeUp = true;
						}
					}
				}
				if (escapeDown)
				{
					AddKeyInput(K_ESCAPE, true);
				}
				if (escapeUp)
				{
					AddKeyInput(K_ESCAPE, false);
				}
				if (joy_initialized)
				{
					joy_avail = true;
					actionGetInfo.action = appState.Actions.MoveX;
					CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
					if (floatActionState.isActive && floatActionState.changedSinceLastSync)
					{
						pdwRawValue[JOY_AXIS_X] = -floatActionState.currentState;
					}
					actionGetInfo.action = appState.Actions.MoveY;
					CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
					if (floatActionState.isActive && floatActionState.changedSinceLastSync)
					{
						pdwRawValue[JOY_AXIS_Y] = -floatActionState.currentState;
					}
				}
				if (!keyPressHandled)
				{
					actionGetInfo.action = appState.Actions.Fire;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddCommandInput("+attack");
						}
						else
						{
							AddCommandInput("-attack");
						}
					}
				}
				actionGetInfo.action = appState.Actions.Run;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						AddCommandInput("+speed");
					}
					else
					{
						AddCommandInput("-speed");
					}
				}
				actionGetInfo.action = appState.Actions.SwitchWeapon;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && booleanActionState.currentState)
				{
					AddCommandInput("impulse 10");
				}
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.Actions.SwimDownLeftHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddCommandInput("+movedown");
						}
						else
						{
							AddCommandInput("-movedown");
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.Actions.SwimDownRightHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddCommandInput("+movedown");
						}
						else
						{
							AddCommandInput("-movedown");
						}
					}
				}
			}
			else
			{
				if (m_state == m_quit)
				{
					actionGetInfo.action = appState.Actions.EscapeNonY;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddKeyInput(K_ESCAPE, true);
						}
						else
						{
							AddKeyInput(K_ESCAPE, false);
						}
					}
					actionGetInfo.action = appState.Actions.Quit;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddKeyInput('y', true);
						}
						else
						{
							AddKeyInput('y', false);
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.Actions.EscapeY;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddKeyInput(K_ESCAPE, true);
						}
						else
						{
							AddKeyInput(K_ESCAPE, false);
						}
					}
				}
				auto escapeDown = false;
				auto escapeUp = false;
				actionGetInfo.action = appState.Actions.Menu;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						escapeDown = true;
					}
					else
					{
						escapeUp = true;
					}
				}
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.Actions.MenuLeftHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							escapeDown = true;
						}
						else
						{
							escapeUp = true;
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.Actions.MenuRightHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							escapeDown = true;
						}
						else
						{
							escapeUp = true;
						}
					}
				}
				if (escapeDown)
				{
					AddKeyInput(K_ESCAPE, true);
				}
				if (escapeUp)
				{
					AddKeyInput(K_ESCAPE, false);
				}
				auto thumbstick = appState.PreviousThumbstick;
				actionGetInfo.action = appState.Actions.MoveX;
				CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
				if (floatActionState.isActive && floatActionState.changedSinceLastSync)
				{
					thumbstick.x = -floatActionState.currentState;
				}
				actionGetInfo.action = appState.Actions.MoveY;
				CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
				if (floatActionState.isActive && floatActionState.changedSinceLastSync)
				{
					thumbstick.y = -floatActionState.currentState;
				}
				if (thumbstick.y > 0.5 && appState.PreviousThumbstick.y <= 0.5)
				{
					AddKeyInput(K_DOWNARROW, true);
				}
				if (thumbstick.y <= 0.5 && appState.PreviousThumbstick.y > 0.5)
				{
					AddKeyInput(K_DOWNARROW, false);
				}
				if (thumbstick.y < -0.5 && appState.PreviousThumbstick.y >= -0.5)
				{
					AddKeyInput(K_UPARROW, true);
				}
				if (thumbstick.y >= -0.5 && appState.PreviousThumbstick.y < -0.5)
				{
					AddKeyInput(K_UPARROW, false);
				}
				if (thumbstick.x > 0.5 && appState.PreviousThumbstick.x <= 0.5)
				{
					AddKeyInput(K_LEFTARROW, true);
				}
				if (thumbstick.x <= 0.5 && appState.PreviousThumbstick.x > 0.5)
				{
					AddKeyInput(K_LEFTARROW, false);
				}
				if (thumbstick.x < -0.5 && appState.PreviousThumbstick.x >= -0.5)
				{
					AddKeyInput(K_RIGHTARROW, true);
				}
				if (thumbstick.x >= -0.5 && appState.PreviousThumbstick.x < -0.5)
				{
					AddKeyInput(K_RIGHTARROW, false);
				}
				appState.PreviousThumbstick = thumbstick;
				auto enterDown = false;
				auto enterUp = false;
				if (!keyPressHandled)
				{
					actionGetInfo.action = appState.Actions.EnterTrigger;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							enterDown = true;
						}
						else
						{
							enterUp = true;
						}
					}
				}
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.Actions.EnterNonTriggerLeftHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							enterDown = true;
						}
						else
						{
							enterUp = true;
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.Actions.EnterNonTriggerRightHanded;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							enterDown = true;
						}
						else
						{
							enterUp = true;
						}
					}
				}
				if (enterDown)
				{
					AddKeyInput(K_ENTER, true);
				}
				if (enterUp)
				{
					AddKeyInput(K_ENTER, false);
				}
			}
		}
	}
}
