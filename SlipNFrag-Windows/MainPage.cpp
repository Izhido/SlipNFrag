#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include <Windows.UI.Xaml.Media.DXInterop.h>
#include "vid_uwp.h"
#include "sys_uwp.h"
#include "virtualkeymap.h"
#include "in_uwp.h"
#include "snd_uwp.h"

extern qboolean snd_forceclear;

#pragma comment(lib, "User32.lib")

struct __declspec(uuid("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")) __declspec(novtable) IMemoryBufferByteAccess : ::IUnknown
{
	virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};

using namespace winrt;
using namespace DirectX;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Devices::Input;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Gaming::Input;
using namespace Windows::Graphics::Display;
using namespace Windows::Media;
using namespace Windows::Media::Audio;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Render;
using namespace Windows::Security::Cryptography;
using namespace Windows::Storage;
using namespace Windows::Storage::AccessCache;
using namespace Windows::Storage::FileProperties;
using namespace Windows::Storage::Streams;
using namespace Windows::System;
using namespace Windows::System::Threading;
using namespace Windows::UI;
using namespace Windows::UI::Input;
using namespace Windows::UI::Core;
using namespace Windows::UI::Popups;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;

struct VertexShaderInput
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 texCoords;
};

static const int alignedConstantBufferSize = (sizeof(ModelViewProjectionConstantBuffer) + 255) & ~255;

static const DirectX::XMFLOAT4X4 Rotation0
(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
);

static const DirectX::XMFLOAT4X4 Rotation90
(
	0.0f, 1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
);

static const DirectX::XMFLOAT4X4 Rotation180
(
	-1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, -1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
);

static const DirectX::XMFLOAT4X4 Rotation270
(
	0.0f, -1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
);

namespace winrt::SlipNFrag_Windows::implementation
{
	MainPage::MainPage() : 
		defaultHeapProperties { D3D12_HEAP_TYPE_DEFAULT }, 
		uploadHeapProperties { D3D12_HEAP_TYPE_UPLOAD },
		screenTextureDesc { D3D12_RESOURCE_DIMENSION_TEXTURE2D },
		paletteTextureDesc { D3D12_RESOURCE_DIMENSION_TEXTURE1D },
		screenSrvDesc { },
		paletteSrvDesc { },
		screenLocation { },
		screenUploadLocation { },
		paletteLocation { },
		paletteUploadLocation { }
	{
		InitializeComponent();
		defaultHeapProperties.CreationNodeMask = 1;
		defaultHeapProperties.VisibleNodeMask = 1;
		uploadHeapProperties.CreationNodeMask = 1;
		uploadHeapProperties.VisibleNodeMask = 1;
		screenTextureDesc.MipLevels = 1;
		screenTextureDesc.Format = DXGI_FORMAT_R8_UNORM;
		screenTextureDesc.DepthOrArraySize = 1;
		screenTextureDesc.SampleDesc.Count = 1;
		consoleTextureDesc = screenTextureDesc;
		paletteTextureDesc.MipLevels = 1;
		paletteTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		paletteTextureDesc.Width = 256;
		paletteTextureDesc.Height = 1;
		paletteTextureDesc.DepthOrArraySize = 1;
		paletteTextureDesc.SampleDesc.Count = 1;
		screenSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		screenSrvDesc.Format = screenTextureDesc.Format;
		screenSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		screenSrvDesc.Texture2D.MipLevels = screenTextureDesc.MipLevels;
		consoleSrvDesc = screenSrvDesc;
		paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		paletteSrvDesc.Format = paletteTextureDesc.Format;
		paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		paletteSrvDesc.Texture1D.MipLevels = paletteTextureDesc.MipLevels;
		screenLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		consoleLocation = screenLocation;
		screenUploadLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		consoleUploadLocation = screenUploadLocation;
		paletteLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		paletteUploadLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		auto titleBar = CoreApplication::GetCurrentView().TitleBar();
		titleBar.ExtendViewIntoTitleBar(true);
		UpdateTitleBarLayout(titleBar);
		Window::Current().SetTitleBar(appTitleBar());
		titleBar.LayoutMetricsChanged([this, titleBar](CoreApplicationViewTitleBar const&, IInspectable const&)
			{
				UpdateTitleBarLayout(titleBar);
			});
		titleBar.IsVisibleChanged([this, titleBar](CoreApplicationViewTitleBar const&, IInspectable const&)
			{
				if (titleBar.IsVisible())
				{
					appTitleBar().Visibility(Visibility::Visible);
					fullscreenButton().Visibility(Visibility::Visible);
				}
				else
				{
					appTitleBar().Visibility(Visibility::Collapsed);
					fullscreenButton().Visibility(Visibility::Collapsed);
				}
			});
		Uri playBitmapImageUri(L"ms-appx:///Assets/play.png");
		BitmapImage playBitmapImage(playBitmapImageUri);
		Image playImage;
		playImage.Source(playBitmapImage);
		auto transparent = ColorHelper::FromArgb(0, 0, 0, 0);
		SolidColorBrush transparentBrush(transparent);
		Button playButton;
		playButton.Background(transparentBrush);
		playButton.HorizontalAlignment(HorizontalAlignment::Center);
		playButton.VerticalAlignment(VerticalAlignment::Center);
		playButton.Width(150);
		playButton.Height(150);
		playButton.Content(playImage);
		swapChainPanel().Children().Append(playButton);
		SymbolIcon settingsIcon;
		settingsIcon.Symbol(Symbol::Setting);
		Thickness settingsPadding;
		settingsPadding.Left = 12;
		settingsPadding.Top = 12;
		settingsPadding.Right = 12;
		settingsPadding.Bottom = 12;
		Button settingsButton;
		settingsButton.Background(transparentBrush);
		settingsButton.HorizontalAlignment(HorizontalAlignment::Right);
		settingsButton.VerticalAlignment(VerticalAlignment::Bottom);
		settingsButton.Width(42);
		settingsButton.Height(42);
		settingsButton.Content(settingsIcon);
		settingsButton.Margin(settingsPadding);
		swapChainPanel().Children().Append(settingsButton);
		playButton.Click([this, playButton, settingsButton](IInspectable const&, RoutedEventArgs const&)
			{
				uint32_t index;
				if (swapChainPanel().Children().IndexOf(playButton, index))
				{
					swapChainPanel().Children().RemoveAt(index);
				}
				if (swapChainPanel().Children().IndexOf(settingsButton, index))
				{
					swapChainPanel().Children().RemoveAt(index);
				}
				auto displayInformation = DisplayInformation::GetForCurrentView();
				displayInformation.DpiChanged([this](DisplayInformation const& sender, IInspectable const& e)
					{
						DpiChanged(sender, e);
					});
				displayInformation.OrientationChanged([this](DisplayInformation const& sender, IInspectable const& e)
					{
						OrientationChanged(sender, e);
					});
				displayInformation.DisplayContentsInvalidated([this](DisplayInformation const& sender, IInspectable const& e)
					{
						DisplayContentsInvalidated(sender, e);
					});
				auto folder = Package::Current().InstalledLocation();
				OpenVertexShaderFile(folder);
			});
		settingsButton.Click([this, playButton, settingsButton](IInspectable const&, RoutedEventArgs const&)
			{
				SettingsContentDialog settings;
				settings.ShowAsync(ContentDialogPlacement::InPlace);
			});
	}

	void MainPage::UpdateTitleBarLayout(CoreApplicationViewTitleBar const& titleBar)
	{
		auto leftInset = titleBar.SystemOverlayLeftInset();
		auto rightInset = titleBar.SystemOverlayRightInset();
		appTitleBarLeftPaddingColumn().Width(GridLengthHelper::FromPixels(leftInset));
		appTitleBarRightPaddingColumn().Width(GridLengthHelper::FromPixels(rightInset));
		fullscreenButton().Margin(ThicknessHelper::FromLengths(0, 0, rightInset, 0));
		appTitleBar().Height(titleBar.Height());
	}

	void MainPage::OpenVertexShaderFile(StorageFolder const& folder)
	{
		auto task = folder.GetFileAsync(L"VertexShader.cso");
		task.Completed([=](IAsyncOperation<StorageFile> const& operation, AsyncStatus const)
			{
				StorageFile file = operation.get();
				ReadVertexShaderFile(file, folder);
			});
	}

	void MainPage::ReadVertexShaderFile(StorageFile const& file, StorageFolder const& folder)
	{
		auto task = FileIO::ReadBufferAsync(file);
		task.Completed([=](IAsyncOperation<IBuffer> const& operation, AsyncStatus const)
			{
				auto vertexShaderFileBuffer = operation.get();
				vertexShaderBuffer.resize(vertexShaderFileBuffer.Length());
				DataReader::FromBuffer(vertexShaderFileBuffer).ReadBytes(vertexShaderBuffer);
				OpenPixelShaderFile(folder);
			});
	}

	void MainPage::OpenPixelShaderFile(StorageFolder const& folder)
	{
		auto task = folder.GetFileAsync(L"PixelShader.cso");
		task.Completed([=](IAsyncOperation<StorageFile> const& operation, AsyncStatus const)
			{
				StorageFile file = operation.get();
				ReadPixelShaderFile(file);
			});
	}

	void MainPage::ReadPixelShaderFile(StorageFile const& file)
	{
		auto pixelShaderReadTask = FileIO::ReadBufferAsync(file);
		pixelShaderReadTask.Completed([=](IAsyncOperation<IBuffer> const& operation, AsyncStatus const)
			{
				auto pixelShaderFileBuffer = operation.get();
				pixelShaderBuffer.resize(pixelShaderFileBuffer.Length());
				DataReader::FromBuffer(pixelShaderFileBuffer).ReadBytes(pixelShaderBuffer);
				Start();
			});
	}

	void MainPage::Start()
	{
		if (!StorageApplicationPermissions::FutureAccessList().ContainsItem(L"basedir_text"))
		{
			sys_errormessage = "FutureAccessList does not contain an entry for basedir_text";
			sys_nogamedata = 1;
			DisplaySysError();
			return;
		}
		auto task = StorageApplicationPermissions::FutureAccessList().GetFolderAsync(L"basedir_text");
		task.Completed([this](IAsyncOperation<StorageFolder> const& operation, AsyncStatus const status)
			{
				std::string basedir_text;
				if (status == AsyncStatus::Completed)
				{
					auto folder = operation.GetResults();
					auto path = folder.Path();
					for (auto c : path)
					{
						basedir_text.push_back((char)c);
					}
				}
				std::vector<std::string> arguments;
				arguments.emplace_back("SlipNFrag.exe");
				if (basedir_text.length() > 0)
				{
					arguments.emplace_back("-basedir");
					arguments.push_back(basedir_text);
				}
				auto values = ApplicationData::Current().LocalSettings().Values();
				if (values.HasKey(L"hipnotic_radio"))
				{
					auto value = values.Lookup(L"hipnotic_radio");
					if (unbox_value<bool>(value))
					{
						arguments.emplace_back("-hipnotic");
					}
				}
				if (values.HasKey(L"rogue_radio"))
				{
					auto value = values.Lookup(L"rogue_radio");
					if (unbox_value<bool>(value))
					{
						arguments.emplace_back("-rogue");
					}
				}
				if (values.HasKey(L"game_radio"))
				{
					if (values.HasKey(L"game_text"))
					{
						auto value = values.Lookup(L"game_radio");
						if (unbox_value<bool>(value))
						{
							value = values.Lookup(L"game_text");
							auto value_as_string = unbox_value<hstring>(value);
							std::string game_text;
							for (auto c : value_as_string)
							{
								game_text.push_back((char)c);
							}
							arguments.emplace_back("-game");
							arguments.push_back(game_text);
						}
					}
				}
				auto joystick_check = false;
				if (values.HasKey(L"joystick_check"))
				{
					auto value = values.Lookup(L"joystick_check");
					joystick_check = unbox_value<bool>(value);
				}
				if (joystick_check)
				{
					arguments.emplace_back("+joystick");
					arguments.emplace_back("1");
					auto joy_standard_radio = false;
					auto joy_advanced_radio = false;
					if (values.HasKey(L"joy_standard_radio"))
					{
						auto value = values.Lookup(L"joy_standard_radio");
						joy_standard_radio = unbox_value<bool>(value);
					}
					if (values.HasKey(L"joy_advanced_radio"))
					{
						auto value = values.Lookup(L"joy_advanced_radio");
						joy_advanced_radio = unbox_value<bool>(value);
					}
					joyCommands.clear();
					joyCommands.resize(5);
					joyCommands.push_back("+attack");
					joyCommands.push_back("impulse 10");
					joyCommands.push_back("+jump");
					joyCommands.push_back("+forward");
					joyCommands.push_back("+back");
					joyCommands.push_back("+left");
					joyCommands.push_back("+right");
					joyCommands.push_back("+speed");
					joyCommands.push_back("+moveleft");
					joyCommands.push_back("+moveright");
					joyCommands.push_back("+strafe");
					joyCommands.push_back("+lookup");
					joyCommands.push_back("+lookdown");
					joyCommands.push_back("centerview");
					joyCommands.push_back("+mlook");
					joyCommands.push_back("+klook");
					joyCommands.push_back("+moveup");
					joyCommands.push_back("+movedown");
					joyCommands.push_back("K_ENTER");
					joyCommands.push_back("K_ESCAPE");
					joyCommands.push_back("K_UPARROW");
					joyCommands.push_back("K_LEFTARROW");
					joyCommands.push_back("K_RIGHTARROW");
					joyCommands.push_back("K_DOWNARROW");
					joyCommands.push_back("impulse 1");
					joyCommands.push_back("impulse 2");
					joyCommands.push_back("impulse 3");
					joyCommands.push_back("impulse 4");
					joyCommands.push_back("impulse 5");
					joyCommands.push_back("impulse 6");
					joyCommands.push_back("impulse 7");
					joyCommands.push_back("impulse 8");
					joyAxesAsButtons.clear();
					joyAxesAsButtonsOnRelease.clear();
					if (joy_standard_radio)
					{
						joyAxesAsButtons.resize(4);
						joyAxesAsButtons.push_back(joyCommands[6]);
						joyAxesAsButtons.push_back(joyCommands[5]);
						joyAxesAsButtonsOnRelease.resize(5);
						joyAxesAsButtonsOnRelease.push_back(std::string("-") + joyCommands[5].substr(1));
						joyAxesAsKeys.resize(6);
						joyAxesInverted.resize(6);
						for (auto i = 0; i < (int)joyAxesInverted.size(); i++)
						{
							joyAxesInverted[i] = 1;
						}
					}
					else if (joy_advanced_radio)
					{
						arguments.emplace_back("+joyadvanced");
						arguments.emplace_back("1");
						AddJoystickAxis(values, L"joy_axis_x", "+joyadvaxisx", arguments);
						AddJoystickAxis(values, L"joy_axis_y", "+joyadvaxisy", arguments);
						AddJoystickAxis(values, L"joy_axis_z", "+joyadvaxisz", arguments);
						AddJoystickAxis(values, L"joy_axis_r", "+joyadvaxisr", arguments);
						AddJoystickAxis(values, L"joy_axis_u", "+joyadvaxisu", arguments);
						AddJoystickAxis(values, L"joy_axis_v", "+joyadvaxisv", arguments);
					}
					joyButtonsAsKeys.clear();
					joyButtonsAsCommands.clear();
					for (auto i = 0; i < 36; i++)
					{
						auto keyFound = false;
						auto commandFound = false;
						hstring name;
						if (i < 4)
						{
							name = L"k_joy" + to_hstring(i + 1) + L"_combo";
						}
						else
						{
							name = L"k_aux" + to_hstring(i - 3) + L"_combo";
						}
						if (values.HasKey(name))
						{
							auto value = values.Lookup(name);
							std::wstring text(unbox_value<hstring>(value));
							if (text.size() > 0)
							{
								try
								{
									auto index = std::stoi(text);
									if (index >= 1 && index < (int)joyCommands.size() - 5)
									{
										auto command = joyCommands[index + 4];
										if (command == "K_ENTER")
										{
											joyButtonsAsKeys.push_back(13);
											keyFound = true;
										}
										else if (command == "K_ESCAPE")
										{
											joyButtonsAsKeys.push_back(27);
											keyFound = true;
										}
										else if (command == "K_UPARROW")
										{
											joyButtonsAsKeys.push_back(128);
											keyFound = true;
										}
										else if (command == "K_LEFTARROW")
										{
											joyButtonsAsKeys.push_back(130);
											keyFound = true;
										}
										else if (command == "K_RIGHTARROW")
										{
											joyButtonsAsKeys.push_back(131);
											keyFound = true;
										}
										else if (command == "K_DOWNARROW")
										{
											joyButtonsAsKeys.push_back(129);
											keyFound = true;
										}
										else
										{
											joyButtonsAsCommands.push_back(command);
											commandFound = true;
										}
									}
								}
								catch (...)
								{
									// Do nothing. The key will be set automatically to -1.
								}
							}
						}
						if (!keyFound)
						{
							joyButtonsAsKeys.push_back(-1);
						}
						if (!commandFound)
						{
							joyButtonsAsCommands.emplace_back();
						}
					}
					previousGamepadButtonsWereRead = false;
					previousJoyAxesAsButtonValuesWereRead = false;
					previousJoyAxesAsButtonValues.resize(joyAxesAsButtons.size());
					mlook_enabled = false;
					if (values.HasKey(L"mlook_check"))
					{
						auto value = values.Lookup(L"mlook_check");
						if (unbox_value<bool>(value))
						{
							mlook_enabled = true;
						}
					}
				}
				if (values.HasKey(L"command_line_text"))
				{
					auto value = values.Lookup(L"command_line_text");
					auto value_as_string = unbox_value<hstring>(value);
					std::string one_command;
					auto within_quotes = false;
					for (auto c : value_as_string)
					{
						if (c == ' ' && !within_quotes)
						{
							if (one_command.length() > 0)
							{
								arguments.push_back(one_command);
								one_command.clear();
							}
						}
						else
						{
							if (c == '\"')
							{
								within_quotes = !within_quotes;
							}
							one_command.push_back((char)c);
						}
					}
					if (one_command.length() > 0)
					{
						arguments.push_back(one_command);
					}
				}
				sys_argc = (int)arguments.size();
				sys_argv = new char* [sys_argc];
				for (auto i = 0; i < arguments.size(); i++)
				{
					sys_argv[i] = new char[arguments[i].length() + 1];
					strcpy(sys_argv[i], arguments[i].c_str());
				}
				sys_version = "UWP 1.0.6";
				swapChainPanel().Dispatcher().RunAsync(CoreDispatcherPriority::High, [=]()
					{
						newScreenWidth = (float)swapChainPanel().ActualWidth();
						newScreenHeight = (float)swapChainPanel().ActualHeight();
						CalculateConsoleDimensions();
						try
						{
							CreateDevice();
						}
						catch (...)
						{
							sys_errormessage = "DirectX 12, Feature Level 11.0 is not currently available in your system.";
							DisplaySysError();
							return;
						}
						std::thread filesThread([this]()
							{
								ProcessFiles();
							});
						filesThread.detach();
						vid_width = (int)newScreenWidth;
						vid_height = (int)newScreenHeight;
						vid_rowbytes = (int)newScreenRowbytes;
						con_width = (int)newConsoleWidth;
						con_height = (int)newConsoleHeight;
						con_rowbytes = (int)newConsoleRowbytes;
						Sys_Init(sys_argc, sys_argv);
						if (sys_errorcalled || sys_quitcalled)
						{
							if (sys_errormessage.length() > 0)
							{
								DisplaySysError();
							}
							return;
						}
						if (joy_initialized)
						{
							for (auto i = 0; i < joyButtonsAsCommands.size(); i++)
							{
								auto& command = joyButtonsAsCommands[i];
								if (command.size() > 0)
								{
									std::string text = "bind ";
									if (i < 4)
									{
										text += "JOY" + std::to_string(i + 1);
									}
									else
									{
										text += "AUX" + std::to_string(i - 3);
									}
									text += " \"" + command + "\"\n";
									Cbuf_AddText(text.c_str());
								}
							}
						}
						if (mlook_enabled)
						{
							Cbuf_AddText("+mlook");
						}
						if (snd_initialized)
						{
							CreateAudioGraph();
						}
						if (mouseinitialized)
						{
							if (key_dest == key_game)
							{
								RegisterMouseMoved();
							}
							key_dest_was_game = (key_dest == key_game);
						}
						Window::Current().CoreWindow().KeyDown([this](IInspectable const&, KeyEventArgs const& e)
							{
								HandleKey((int)e.VirtualKey(), true);
							});
						Window::Current().CoreWindow().KeyUp([this](IInspectable const&, KeyEventArgs const& e)
							{
								HandleKey((int)e.VirtualKey(), false);
							});
						if (mouseinitialized)
						{
							Window::Current().CoreWindow().PointerPressed([this](IInspectable const&, PointerEventArgs const& e)
								{
									auto properties = e.CurrentPoint().Properties();
									try
									{
										std::lock_guard lock(renderLoopMutex);
										if (properties.IsLeftButtonPressed())
										{
											Key_Event(K_MOUSE1, true);
										}
										if (properties.IsMiddleButtonPressed())
										{
											Key_Event(K_MOUSE3, true);
										}
										if (properties.IsRightButtonPressed())
										{
											Key_Event(K_MOUSE2, true);
										}
									}
									catch (...)
									{
										// Do nothing - error messages (and Sys_Quit) will already be handled before getting here
									}
								});
							Window::Current().CoreWindow().PointerReleased([this](IInspectable const&, PointerEventArgs const& e)
								{
									auto properties = e.CurrentPoint().Properties();
									try
									{
										std::lock_guard lock(renderLoopMutex);
										if (!properties.IsLeftButtonPressed())
										{
											Key_Event(K_MOUSE1, false);
										}
										if (!properties.IsMiddleButtonPressed())
										{
											Key_Event(K_MOUSE3, false);
										}
										if (!properties.IsRightButtonPressed())
										{
											Key_Event(K_MOUSE2, false);
										}
									}
									catch (...)
									{
										// Do nothing - error messages (and Sys_Quit) will already be handled before getting here
									}
								});
						}
						if (joy_initialized)
						{
							if (Gamepad::Gamepads().Size() > 0)
							{
								gamepad = Gamepad::Gamepads().GetAt(0);
								joy_avail = true;
							}
							if (!joy_avail)
							{
								for (auto const& controller : RawGameController::RawGameControllers())
								{
									if (controller.AxisCount() >= 2 && controller.ButtonCount() >= 4)
									{
										joystick = controller;
										joy_avail = true;
										break;
									}
								}
							}
							Gamepad::GamepadAdded([this](IInspectable const&, Gamepad const& e)
								{
									if (gamepad.get() == nullptr && joystick.get() == nullptr)
									{
										gamepad = e;
										joy_avail = true;
									}
								});
							Gamepad::GamepadRemoved([this](IInspectable const&, Gamepad const& e)
								{
									if (gamepad.get() != nullptr && gamepad.get() == e)
									{
										in_forwardmove = 0.0;
										in_sidestepmove = 0.0;
										in_rollangle = 0.0;
										in_pitchangle = 0.0;
										gamepad = nullptr;
										joy_avail = false;
									}
								});
							RawGameController::RawGameControllerAdded([this](IInspectable const&, RawGameController const& e)
								{
									if (gamepad.get() == nullptr && joystick.get() == nullptr && e.AxisCount() >= 2 && e.ButtonCount() >= 4)
									{
										joystick = e;
										joy_avail = true;
									}
								});
							RawGameController::RawGameControllerRemoved([this](IInspectable const&, RawGameController const& e)
								{
									if (joystick.get() != nullptr && joystick.get().NonRoamableId() == e.NonRoamableId())
									{
										in_forwardmove = 0.0;
										in_sidestepmove = 0.0;
										in_rollangle = 0.0;
										in_pitchangle = 0.0;
										joystick = nullptr;
										joy_avail = false;
										delete[] previousJoystickButtons;
										previousJoystickButtonsLength = 0;
									}
								});
						}
						auto fullscreen_check = true;
						if (values.HasKey(L"fullscreen_check"))
						{
							auto value = values.Lookup(L"fullscreen_check");
							fullscreen_check = unbox_value<bool>(value);
						}
						if (fullscreen_check)
						{
							ApplicationView::GetForCurrentView().TryEnterFullScreenMode();
						}
						renderLoopWorker = ThreadPool::RunAsync([=](IAsyncAction const& action)
							{
								RenderLoop(action);
							}, WorkItemPriority::High, WorkItemOptions::TimeSliced);
					});
			});
	}

	void MainPage::CreateDevice()
	{
		UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
		com_ptr<ID3D12Debug> debugController0;
		com_ptr<ID3D12Debug1> debugController1;
		check_hresult(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController0)));
		debugController0->EnableDebugLayer();
		debugController1 = debugController0.as<ID3D12Debug1>();
		debugController1->SetEnableGPUBasedValidation(true);
		debugController1->SetEnableSynchronizedCommandQueueValidation(true);
		dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
		check_hresult(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
		com_ptr<IDXGIAdapter1> adapter = nullptr;
		for (auto i = 0; dxgiFactory->EnumAdapters1(i, adapter.put()) != DXGI_ERROR_NOT_FOUND; i++)
		{
			DXGI_ADAPTER_DESC1 descriptor;
			adapter->GetDesc1(&descriptor);
			if ((descriptor.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			{
				continue;
			}
			if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
		if (FAILED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice))))
		{
			com_ptr<IDXGIAdapter> warpAdapter;
			check_hresult(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
			check_hresult(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice)));
		}
		D3D12_COMMAND_QUEUE_DESC queueDesc { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
		check_hresult(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
		commandQueue->SetName(L"commandQueue");
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, frameCount };
		check_hresult(d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
		rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1 };
		check_hresult(d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
		D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameCount * 4, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
		check_hresult(d3dDevice->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&cbvSrvHeap)));
		cbvSrvHeap->SetName(L"cbvSrvHeap");
		for (auto i = 0; i < frameCount; i++)
		{
			check_hresult(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
		}
		check_hresult(d3dDevice->CreateFence(fenceValues[0], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		fenceValues[0]++;
		fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		auto displayInformation = DisplayInformation::GetForCurrentView();
		Size outputSize;
		CreateOutputSize(displayInformation, outputSize);
		DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
		CreateRotation(displayInformation, rotation);
		Size renderTargetSize;
		CreateRenderTargetSize(rotation, outputSize, renderTargetSize);
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc { };
		swapChainDesc.Width = lround(renderTargetSize.Width);
		swapChainDesc.Height = lround(renderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = frameCount;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		com_ptr<IDXGISwapChain1> newSwapChain;
		check_hresult(dxgiFactory->CreateSwapChainForComposition(commandQueue.get(), &swapChainDesc, nullptr, newSwapChain.put()));
		newSwapChain.as(swapChain);
		com_ptr<ISwapChainPanelNative> panelNative;
		swapChainPanel().as(panelNative);
		check_hresult(panelNative->SetSwapChain(swapChain.get()));
		XMFLOAT4X4 orientationTransform3D;
		CreateOrientationTransform3D(rotation, orientationTransform3D);
		UpdateSwapChainTransform(rotation, swapChainPanel().CompositionScaleX(), swapChainPanel().CompositionScaleY());
		currentFrame = swapChain->GetCurrentBackBufferIndex();
		CreateRenderTargetViews();
		CreateDepthStencilBuffer(swapChainDesc.Width, swapChainDesc.Height);
		viewport = { 0, 0, renderTargetSize.Width, renderTargetSize.Height, 0, 1 };
		D3D12_DESCRIPTOR_RANGE1 ranges[2] { };
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[0].NumDescriptors = 1;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[1].NumDescriptors = 3;
		ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
		D3D12_ROOT_PARAMETER1 parameters[2] { };
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		parameters[0].DescriptorTable.NumDescriptorRanges = 1;
		parameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
		parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		parameters[1].DescriptorTable.NumDescriptorRanges = 1;
		parameters[1].DescriptorTable.pDescriptorRanges = &ranges[1];
		D3D12_STATIC_SAMPLER_DESC samplers[3] { };
		samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		samplers[1] = samplers[0];
		samplers[1].ShaderRegister = 1;
		samplers[2] = samplers[0];
		samplers[2].ShaderRegister = 2;
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC descRootSignature { };
		descRootSignature.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		descRootSignature.Desc_1_1.NumParameters = 2;
		descRootSignature.Desc_1_1.pParameters = &parameters[0];
		descRootSignature.Desc_1_1.NumStaticSamplers = 3;
		descRootSignature.Desc_1_1.pStaticSamplers = &samplers[0];
		descRootSignature.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		com_ptr<ID3DBlob> signature;
		com_ptr<ID3DBlob> error;
		check_hresult(D3D12SerializeVersionedRootSignature(&descRootSignature, signature.put(), error.put()));
		check_hresult(d3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
		rootSignature->SetName(L"rootSignature");
		static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC state { };
		state.InputLayout = { inputLayout, _countof(inputLayout) };
		state.pRootSignature = rootSignature.get();
		state.VS.BytecodeLength = vertexShaderBuffer.size();
		state.VS.pShaderBytecode = vertexShaderBuffer.data();
		state.PS.BytecodeLength = pixelShaderBuffer.size();
		state.PS.pShaderBytecode = pixelShaderBuffer.data();
		state.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		state.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		state.RasterizerState.DepthClipEnable = true;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (auto i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		{
			state.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
		}
		state.DepthStencilState.DepthEnable = true;
		state.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		state.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		state.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{
			D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP,
			D3D12_COMPARISON_FUNC_ALWAYS
		};
		state.DepthStencilState.FrontFace = defaultStencilOp;
		state.DepthStencilState.BackFace = defaultStencilOp;
		state.SampleMask = UINT_MAX;
		state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		state.NumRenderTargets = 1;
		state.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
		state.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		state.SampleDesc.Count = 1;
		check_hresult(d3dDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&pipelineState)));
		pipelineState->SetName(L"pipelineState");
		check_hresult(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].get(), pipelineState.get(), IID_PPV_ARGS(&commandList)));
		commandList->SetName(L"commandList");
		VertexShaderInput screenPlaneVertices[] =
		{
			{ XMFLOAT3(-1, 1, -0.5f), XMFLOAT2(0, 0) },
			{ XMFLOAT3(-1, -1, -0.5f), XMFLOAT2(0, 1) },
			{ XMFLOAT3(1, 1, -0.5f), XMFLOAT2(1, 0) },
			{ XMFLOAT3(1, -1, -0.5f), XMFLOAT2(1, 1) }
		};
		auto vertexBufferSize = sizeof(screenPlaneVertices);
		D3D12_RESOURCE_DESC vertexBufferDesc { D3D12_RESOURCE_DIMENSION_BUFFER };
		vertexBufferDesc.Width = vertexBufferSize;
		vertexBufferDesc.Height = 1;
		vertexBufferDesc.DepthOrArraySize = 1;
		vertexBufferDesc.MipLevels = 1;
		vertexBufferDesc.SampleDesc.Count = 1;
		vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		check_hresult(d3dDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer)));
		vertexBuffer->SetName(L"vertexBuffer");
		com_ptr<ID3D12Resource> vertexBufferUpload;
		check_hresult(d3dDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBufferUpload)));
		vertexBufferUpload->SetName(L"vertexBufferUpload");
		D3D12_SUBRESOURCE_DATA vertexData;
		vertexData.pData = screenPlaneVertices;
		vertexData.RowPitch = vertexBufferSize;
		vertexData.SlicePitch = vertexData.RowPitch;
		UploadBufferToGPU(vertexData, vertexBuffer, vertexBufferUpload, vertexBufferDesc);
		D3D12_RESOURCE_DESC constantBufferDesc { D3D12_RESOURCE_DIMENSION_BUFFER };
		constantBufferDesc.Width = frameCount * alignedConstantBufferSize;
		constantBufferDesc.Height = 1;
		constantBufferDesc.DepthOrArraySize = 1;
		constantBufferDesc.MipLevels = 1;
		constantBufferDesc.SampleDesc.Count = 1;
		constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		check_hresult(d3dDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer)));
		constantBuffer->SetName(L"constantBuffer");
		check_hresult(d3dDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &paletteTextureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&palette)));
		palette->SetName(L"palette");
		paletteLocation.pResource = palette.get();
		UINT64 uploadBufferSize = 0;
		d3dDevice->GetCopyableFootprints(&paletteTextureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
		D3D12_HEAP_PROPERTIES paletteUploadHeapProperties { D3D12_HEAP_TYPE_UPLOAD };
		paletteUploadHeapProperties.CreationNodeMask = 1;
		paletteUploadHeapProperties.VisibleNodeMask = 1;
		D3D12_RESOURCE_DESC paletteUploadBufferDesc { D3D12_RESOURCE_DIMENSION_BUFFER };
		paletteUploadBufferDesc.Width = uploadBufferSize;
		paletteUploadBufferDesc.Height = 1;
		paletteUploadBufferDesc.DepthOrArraySize = 1;
		paletteUploadBufferDesc.MipLevels = 1;
		paletteUploadBufferDesc.SampleDesc.Count = 1;
		paletteUploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		check_hresult(d3dDevice->CreateCommittedResource(&paletteUploadHeapProperties, D3D12_HEAP_FLAG_NONE, &paletteUploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&paletteUpload)));
		paletteUpload->SetName(L"paletteUpload");
		paletteUploadLocation.pResource = paletteUpload.get();
		CreateScreen();
		auto address = constantBuffer->GetGPUVirtualAddress();
		auto handle = cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
		auto increment = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		for (auto i = 0; i < frameCount; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc { };
			desc.BufferLocation = address;
			desc.SizeInBytes = alignedConstantBufferSize;
			d3dDevice->CreateConstantBufferView(&desc, handle);
			address += desc.SizeInBytes;
			handle.ptr += increment;
			d3dDevice->CreateShaderResourceView(screen.get(), &screenSrvDesc, handle);
			handle.ptr += increment;
			d3dDevice->CreateShaderResourceView(console.get(), &consoleSrvDesc, handle);
			handle.ptr += increment;
			d3dDevice->CreateShaderResourceView(palette.get(), &paletteSrvDesc, handle);
			handle.ptr += increment;
		}
		D3D12_RANGE readRange { };
		check_hresult(constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedConstantBuffer)));
		memset(mappedConstantBuffer, 0, frameCount * alignedConstantBufferSize);
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(VertexShaderInput);
		vertexBufferView.SizeInBytes = (UINT)vertexBufferSize;
		check_hresult(commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { commandList.get() };
		commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		WaitForGPU(0, "WaitForGPU failed while creating device");
		UpdateModelViewProjectionMatrix(orientationTransform3D);
		deviceRemoved = false;
		firstResize = true;
	}

	void MainPage::RemoveDevice()
	{
		deviceRemoved = true;
		swapChainPanel().Dispatcher().RunAsync(CoreDispatcherPriority::High, [this]()
			{
				com_ptr<ISwapChainPanelNative> panelNative;
				swapChainPanel().as(panelNative);
				check_hresult(panelNative->SetSwapChain(nullptr));
			});
	}

	void MainPage::swapChainPanel_SizeChanged(IInspectable const&, SizeChangedEventArgs const& e)
	{
		if (ApplicationView::GetForCurrentView().IsFullScreenMode())
		{
			fullscreenButton().Visibility(Visibility::Collapsed);
		}
		else
		{
			auto titleBar = CoreApplication::GetCurrentView().TitleBar();
			if (titleBar.IsVisible())
			{
				fullscreenButton().Visibility(Visibility::Visible);
			}
			else
			{
				fullscreenButton().Visibility(Visibility::Collapsed);
			}
		}
		if (deviceRemoved)
		{
			return;
		}
		std::lock_guard lock(renderLoopMutex);
		WaitForGPU(currentFrame, "WaitForGPU failed while resizing swap chain panel");
		CleanupFrameData();
		auto displayInformation = DisplayInformation::GetForCurrentView();
		newScreenWidth = e.NewSize().Width;
		newScreenHeight = e.NewSize().Height;
		CalculateConsoleDimensions();
		Size outputSize;
		CreateOutputSize(displayInformation, outputSize);
		DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
		CreateRotation(displayInformation, rotation);
		Size renderTargetSize;
		CreateRenderTargetSize(rotation, outputSize, renderTargetSize);
		auto result = swapChain->ResizeBuffers(frameCount, lround(renderTargetSize.Width), lround(renderTargetSize.Height), DXGI_FORMAT_B8G8R8A8_UNORM, 0);
		if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
		{
			RemoveDevice();
			return;
		}
		check_hresult(result);
		XMFLOAT4X4 orientationTransform3D;
		CreateOrientationTransform3D(rotation, orientationTransform3D);
		UpdateSwapChainTransform(rotation, swapChainPanel().CompositionScaleX(), swapChainPanel().CompositionScaleY());
		currentFrame = swapChain->GetCurrentBackBufferIndex();
		CreateRenderTargetViews();
		depthStencil = nullptr;
		CreateDepthStencilBuffer(lround(renderTargetSize.Width), lround(renderTargetSize.Height));
		viewport = { 0, 0, renderTargetSize.Width, renderTargetSize.Height, 0, 1 };
		screen = nullptr;
		console = nullptr;
		screenUpload = nullptr;
		consoleUpload = nullptr;
		CreateScreen();
		auto handle = cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
		auto increment = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		handle.ptr += increment;
		for (auto i = 0; i < frameCount; i++)
		{
			d3dDevice->CreateShaderResourceView(screen.get(), &screenSrvDesc, handle);
			handle.ptr += increment;
			d3dDevice->CreateShaderResourceView(console.get(), &consoleSrvDesc, handle);
			handle.ptr += increment * 3;
		}
		UpdateModelViewProjectionMatrix(orientationTransform3D);
	}

	void MainPage::swapChainPanel_CompositionScaleChanged(SwapChainPanel const& sender, IInspectable const&)
	{
		if (deviceRemoved)
		{
			return;
		}
		std::lock_guard lock(renderLoopMutex);
		WaitForGPU(currentFrame, "WaitForGPU failed while setting composition scale");
		CleanupFrameData();
		auto displayInformation = DisplayInformation::GetForCurrentView();
		newScreenWidth = (float)swapChainPanel().ActualWidth();
		newScreenHeight = (float)swapChainPanel().ActualHeight();
		CalculateConsoleDimensions();
		Size outputSize;
		CreateOutputSize(displayInformation, outputSize);
		DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
		CreateRotation(displayInformation, rotation);
		Size renderTargetSize;
		CreateRenderTargetSize(rotation, outputSize, renderTargetSize);
		auto result = swapChain->ResizeBuffers(frameCount, lround(renderTargetSize.Width), lround(renderTargetSize.Height), DXGI_FORMAT_B8G8R8A8_UNORM, 0);
		if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
		{
			RemoveDevice();
			return;
		}
		check_hresult(result);
		XMFLOAT4X4 orientationTransform3D;
		CreateOrientationTransform3D(rotation, orientationTransform3D);
		UpdateSwapChainTransform(rotation, sender.CompositionScaleX(), sender.CompositionScaleY());
		currentFrame = swapChain->GetCurrentBackBufferIndex();
		CreateRenderTargetViews();
		depthStencil = nullptr;
		CreateDepthStencilBuffer(lround(renderTargetSize.Width), lround(renderTargetSize.Height));
		viewport = { 0, 0, renderTargetSize.Width, renderTargetSize.Height, 0, 1 };
		screen = nullptr;
		console = nullptr;
		screenUpload = nullptr;
		consoleUpload = nullptr;
		CreateScreen();
		auto handle = cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
		auto increment = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		handle.ptr += increment;
		for (auto i = 0; i < frameCount; i++)
		{
			d3dDevice->CreateShaderResourceView(screen.get(), &screenSrvDesc, handle);
			handle.ptr += increment;
			d3dDevice->CreateShaderResourceView(console.get(), &consoleSrvDesc, handle);
			handle.ptr += increment * 3;
		}
		UpdateModelViewProjectionMatrix(orientationTransform3D);
	}

	void MainPage::DpiChanged(Windows::Graphics::Display::DisplayInformation const& sender, IInspectable const&)
	{
		if (deviceRemoved)
		{
			return;
		}
		std::lock_guard lock(renderLoopMutex);
		WaitForGPU(currentFrame, "WaitForGPU failed while setting screen DPI");
		CleanupFrameData();
		newScreenWidth = (float)swapChainPanel().ActualWidth();
		newScreenHeight = (float)swapChainPanel().ActualHeight();
		CalculateConsoleDimensions();
		Size outputSize;
		CreateOutputSize(sender, outputSize);
		DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
		CreateRotation(sender, rotation);
		Size renderTargetSize;
		CreateRenderTargetSize(rotation, outputSize, renderTargetSize);
		auto result = swapChain->ResizeBuffers(frameCount, lround(renderTargetSize.Width), lround(renderTargetSize.Height), DXGI_FORMAT_B8G8R8A8_UNORM, 0);
		if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
		{
			RemoveDevice();
			return;
		}
		check_hresult(result);
		XMFLOAT4X4 orientationTransform3D;
		CreateOrientationTransform3D(rotation, orientationTransform3D);
		UpdateSwapChainTransform(rotation, swapChainPanel().CompositionScaleX(), swapChainPanel().CompositionScaleX());
		currentFrame = swapChain->GetCurrentBackBufferIndex();
		CreateRenderTargetViews();
		depthStencil = nullptr;
		CreateDepthStencilBuffer(lround(renderTargetSize.Width), lround(renderTargetSize.Height));
		viewport = { 0, 0, renderTargetSize.Width, renderTargetSize.Height, 0, 1 };
		screen = nullptr;
		console = nullptr;
		screenUpload = nullptr;
		consoleUpload = nullptr;
		CreateScreen();
		auto handle = cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
		auto increment = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		handle.ptr += increment;
		for (auto i = 0; i < frameCount; i++)
		{
			d3dDevice->CreateShaderResourceView(screen.get(), &screenSrvDesc, handle);
			handle.ptr += increment;
			d3dDevice->CreateShaderResourceView(console.get(), &consoleSrvDesc, handle);
			handle.ptr += increment * 3;
		}
		UpdateModelViewProjectionMatrix(orientationTransform3D);
	}

	void MainPage::OrientationChanged(Windows::Graphics::Display::DisplayInformation const& sender, IInspectable const&)
	{
		if (deviceRemoved)
		{
			return;
		}
		std::lock_guard lock(renderLoopMutex);
		WaitForGPU(currentFrame, "WaitForGPU failed while setting screen orientation");
		CleanupFrameData();
		newScreenWidth = (float)swapChainPanel().ActualWidth();
		newScreenHeight = (float)swapChainPanel().ActualHeight();
		CalculateConsoleDimensions();
		Size outputSize;
		CreateOutputSize(sender, outputSize);
		DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
		CreateRotation(sender, rotation);
		Size renderTargetSize;
		CreateRenderTargetSize(rotation, outputSize, renderTargetSize);
		auto result = swapChain->ResizeBuffers(frameCount, lround(renderTargetSize.Width), lround(renderTargetSize.Height), DXGI_FORMAT_B8G8R8A8_UNORM, 0);
		if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
		{
			RemoveDevice();
			return;
		}
		check_hresult(result);
		XMFLOAT4X4 orientationTransform3D;
		CreateOrientationTransform3D(rotation, orientationTransform3D);
		UpdateSwapChainTransform(rotation, swapChainPanel().CompositionScaleX(), swapChainPanel().CompositionScaleX());
		currentFrame = swapChain->GetCurrentBackBufferIndex();
		CreateRenderTargetViews();
		depthStencil = nullptr;
		CreateDepthStencilBuffer(lround(renderTargetSize.Width), lround(renderTargetSize.Height));
		viewport = { 0, 0, renderTargetSize.Width, renderTargetSize.Height, 0, 1 };
		screen = nullptr;
		console = nullptr;
		screenUpload = nullptr;
		consoleUpload = nullptr;
		CreateScreen();
		auto handle = cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
		auto increment = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		handle.ptr += increment;
		for (auto i = 0; i < frameCount; i++)
		{
			d3dDevice->CreateShaderResourceView(screen.get(), &screenSrvDesc, handle);
			handle.ptr += increment;
			d3dDevice->CreateShaderResourceView(console.get(), &consoleSrvDesc, handle);
			handle.ptr += increment * 3;
		}
		UpdateModelViewProjectionMatrix(orientationTransform3D);
	}

	void MainPage::DisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation const&, IInspectable const&)
	{
		if (deviceRemoved)
		{
			return;
		}
		std::lock_guard lock(renderLoopMutex);
		if (FAILED(d3dDevice->GetDeviceRemovedReason()))
		{
			RemoveDevice();
			return;
		}
		DXGI_ADAPTER_DESC previousDesc;
		com_ptr<IDXGIAdapter1> previousDefaultAdapter;
		check_hresult(dxgiFactory->EnumAdapters1(0, previousDefaultAdapter.put()));
		check_hresult(previousDefaultAdapter->GetDesc(&previousDesc));
		DXGI_ADAPTER_DESC currentDesc;
		com_ptr<IDXGIFactory4> currentDxgiFactory;
		check_hresult(CreateDXGIFactory1(IID_PPV_ARGS(&currentDxgiFactory)));
		com_ptr<IDXGIAdapter1> currentDefaultAdapter;
		check_hresult(currentDxgiFactory->EnumAdapters1(0, currentDefaultAdapter.put()));
		check_hresult(currentDefaultAdapter->GetDesc(&currentDesc));
		if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart || previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart)
		{
			RemoveDevice();
		}
	}

	void MainPage::RenderLoop(IAsyncAction const& action)
	{
		if (action.Status() != AsyncStatus::Started)
		{
			return;
		}
		if (deviceRemoved)
		{
			try
			{
				constantBuffer->Unmap(0, nullptr);
				screen = nullptr;
				console = nullptr;
				palette = nullptr;
				depthStencil = nullptr;
				CreateDevice();
#if defined(_DEBUG)
				com_ptr<IDXGIDebug1> dxgiDebug;
				if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
				{
					dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
				}
#endif
			}
			catch (...)
			{
				sys_errormessage = "DirectX 12, Feature Level 11.0 is not currently available in your system.";
				DisplaySysError();
				return;
			}
		}
		else
		{
			std::lock_guard lock(renderLoopMutex);
			PIXBeginEvent(commandQueue.get(), 0, L"Update");
			auto updated = Update();
			PIXEndEvent(commandQueue.get());
			if (updated)
			{
				PIXBeginEvent(commandQueue.get(), 0, L"Render");
				Render();
				PIXEndEvent(commandQueue.get());
			}
		}
		if (sys_quitcalled)
		{
			Application::Current().Exit();
			return;
		}
		if (sys_errorcalled)
		{
			return;
		}
		renderLoopWorker = ThreadPool::RunAsync([=](IAsyncAction const& action)
			{
				RenderLoop(action);
			}, WorkItemPriority::High, WorkItemOptions::TimeSliced);
	}

	bool MainPage::Update()
	{
		if (deviceRemoved)
		{
			return false;
		}
		if (previousTime.QuadPart == 0)
		{
			QueryPerformanceCounter(&previousTime);
			return false;
		}
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		auto elapsed = (float)(time.QuadPart - previousTime.QuadPart) / (float)frequency.QuadPart;
		UINT8* destination = mappedConstantBuffer + currentFrame * alignedConstantBufferSize;
		memcpy(destination, &constantBufferData, sizeof(constantBufferData));
		if (firstResize || vid_width != (int)newScreenWidth || vid_height != (int)newScreenHeight)
		{
			vid_width = (int)newScreenWidth;
			vid_height = (int)newScreenHeight;
			vid_rowbytes = (int)newScreenRowbytes;
			con_width = (int)newConsoleWidth;
			con_height = (int)newConsoleHeight;
			con_rowbytes = (int)newConsoleRowbytes;
			VID_Resize();
			firstResize = false;
		}
		if (r_cache_thrash)
		{
			VID_ReallocSurfCache();
		}
		if (mouseinitialized)
		{
			if (key_dest_was_game && key_dest != key_game)
			{
				swapChainPanel().Dispatcher().RunAsync(CoreDispatcherPriority::High, [this]()
					{
						UnregisterMouseMoved();
					});
			}
			else if (!key_dest_was_game && key_dest == key_game)
			{
				swapChainPanel().Dispatcher().RunAsync(CoreDispatcherPriority::High, [this]()
					{
						RegisterMouseMoved();
					});
			}
			key_dest_was_game = (key_dest == key_game);
		}
		if (joy_initialized && joy_avail && joyAxesInverted.size() > 0)
		{
			try
			{
				if (gamepad.get() != nullptr)
				{
					auto reading = gamepad.get().GetCurrentReading();
					pdwRawValue[JOY_AXIS_X] = (float)reading.LeftThumbstickX * joyAxesInverted[0];
					pdwRawValue[JOY_AXIS_Y] = (float)-reading.LeftThumbstickY * joyAxesInverted[1];
					pdwRawValue[JOY_AXIS_Z] = (float)reading.RightThumbstickX * joyAxesInverted[2];
					pdwRawValue[JOY_AXIS_R] = (float)-reading.RightThumbstickY * joyAxesInverted[3];
					pdwRawValue[JOY_AXIS_U] = (float)reading.LeftTrigger * joyAxesInverted[4];
					pdwRawValue[JOY_AXIS_V] = (float)reading.RightTrigger * joyAxesInverted[5];
					if (previousJoyAxesAsButtonValuesWereRead)
					{
						for (auto i = 0; i < joyAxesAsButtons.size(); i++)
						{
							auto previous = fabs(previousJoyAxesAsButtonValues[i]);
							auto current = fabs(pdwRawValue[i]);
							if (previous < 0.15 && current >= 0.15)
							{
								if (joyAxesAsButtons[i].size() > 0)
								{
									Cbuf_AddText(joyAxesAsButtons[i].c_str());
								}
								else if (joyAxesAsKeys[i] > 0)
								{
									Key_Event(joyAxesAsKeys[i], true);
								}
							}
							else if (previous >= 0.15 && current < 0.15)
							{
								if (joyAxesAsButtons[i].size() > 0)
								{
									Cbuf_AddText(joyAxesAsButtonsOnRelease[i].c_str());
								}
								else if (joyAxesAsKeys[i] > 0)
								{
									Key_Event(joyAxesAsKeys[i], false);
								}
							}
						}
					}
					for (auto i = 0; i < joyAxesAsButtons.size(); i++)
					{
						previousJoyAxesAsButtonValues[i] = pdwRawValue[i];
					}
					previousJoyAxesAsButtonValuesWereRead = true;
					auto buttons = (unsigned int)reading.Buttons;
					if (previousGamepadButtonsWereRead)
					{
						auto key = 203;
						unsigned int mask = 1;
						for (auto i = 0; i < 30; i++)
						{
							if ((buttons & mask) == mask && (previousGamepadButtons & mask) != mask)
							{
								if (key_dest == key_game)
								{
									if (joyButtonsAsKeys[i] >= 0)
									{
										Key_Event(joyButtonsAsKeys[i], true);
									}
									else
									{
										Key_Event(key, true);
									}
								}
								else if ((GamepadButtons)mask == GamepadButtons::A)
								{
									Key_Event(13, true);
								}
								else if ((GamepadButtons)mask == GamepadButtons::B)
								{
									Key_Event(27, true);
								}
								else if ((GamepadButtons)mask == GamepadButtons::DPadUp)
								{
									Key_Event(128, true);
								}
								else if ((GamepadButtons)mask == GamepadButtons::DPadDown)
								{
									Key_Event(129, true);
								}
								else if ((GamepadButtons)mask == GamepadButtons::DPadLeft)
								{
									Key_Event(130, true);
								}
								else if ((GamepadButtons)mask == GamepadButtons::DPadRight)
								{
									Key_Event(131, true);
								}
								else
								{
									if (joyButtonsAsKeys[i] >= 0)
									{
										Key_Event(joyButtonsAsKeys[i], true);
									}
									else
									{
										Key_Event(key, true);
									}
								}
							}
							else if ((buttons & mask) != mask && (previousGamepadButtons & mask) == mask)
							{
								if (key_dest == key_game)
								{
									if (joyButtonsAsKeys[i] >= 0)
									{
										Key_Event(joyButtonsAsKeys[i], false);
									}
									else
									{
										Key_Event(key, false);
									}
								}
								else if ((GamepadButtons)mask == GamepadButtons::A)
								{
									Key_Event(13, false);
								}
								else if ((GamepadButtons)mask == GamepadButtons::B)
								{
									Key_Event(27, false);
								}
								else if ((GamepadButtons)mask == GamepadButtons::DPadUp)
								{
									Key_Event(128, false);
								}
								else if ((GamepadButtons)mask == GamepadButtons::DPadDown)
								{
									Key_Event(129, false);
								}
								else if ((GamepadButtons)mask == GamepadButtons::DPadLeft)
								{
									Key_Event(130, false);
								}
								else if ((GamepadButtons)mask == GamepadButtons::DPadRight)
								{
									Key_Event(131, false);
								}
								else
								{
									if (joyButtonsAsKeys[i] >= 0)
									{
										Key_Event(joyButtonsAsKeys[i], false);
									}
									else
									{
										Key_Event(key, false);
									}
								}
							}
							key++;
							mask <<= 1;
						}
					}
					previousGamepadButtons = buttons;
					previousGamepadButtonsWereRead = true;
				}
				if (joystick.get() != nullptr)
				{
					auto buttons = new bool[joystick.get().ButtonCount()];
					std::vector<GameControllerSwitchPosition> switches(joystick.get().SwitchCount());
					std::vector<double> axes(joystick.get().AxisCount());
					joystick.get().GetCurrentReading(array_view(buttons, buttons + joystick.get().ButtonCount()), switches, axes);
					for (auto i = 0; i < JOY_MAX_AXES; i++)
					{
						if (i < axes.size())
						{
							pdwRawValue[i] = (float)(2 * (axes[i] - 0.5)) * joyAxesInverted[i];
						}
						else
						{
							pdwRawValue[i] = 0;
						}
					}
					if (previousJoyAxesAsButtonValuesWereRead)
					{
						for (auto i = 0; i < joyAxesAsButtons.size(); i++)
						{
							auto previous = previousJoyAxesAsButtonValues[i];
							float current;
							if (i < axes.size())
							{
								current = (float)axes[i];
							}
							else
							{
								current = 0;
							}
							if (previous < 0.15 && current >= 0.15)
							{
								if (joyAxesAsButtons[i].size() > 0)
								{
									Cbuf_AddText(joyAxesAsButtons[i].c_str());
								}
								else if (joyAxesAsKeys[i] > 0)
								{
									Key_Event(joyAxesAsKeys[i], true);
								}
							}
							else if (previous >= 0.15 && current < 0.15)
							{
								if (joyAxesAsButtons[i].size() > 0)
								{
									Cbuf_AddText(joyAxesAsButtonsOnRelease[i].c_str());
								}
								else if (joyAxesAsKeys[i] > 0)
								{
									Key_Event(joyAxesAsKeys[i], false);
								}
							}
						}
					}
					for (auto i = 0; i < joyAxesAsButtons.size(); i++)
					{
						if (i < axes.size())
						{
							previousJoyAxesAsButtonValues[i] = (float)axes[i];
						}
						else
						{
							previousJoyAxesAsButtonValues[i] = 0;
						}
					}
					previousJoyAxesAsButtonValuesWereRead = true;
					if (previousJoystickButtonsLength == joystick.get().ButtonCount())
					{
						auto key = 203;
						for (auto i = 0; i < 36; i++)
						{
							if (i >= joystick.get().ButtonCount())
							{
								break;
							}
							if (buttons[i] && !previousJoystickButtons[i])
							{
								if (joyButtonsAsKeys[i] >= 0)
								{
									Key_Event(joyButtonsAsKeys[i], true);
								}
								else
								{
									Key_Event(key, true);
								}
							}
							else if (!buttons[i] && previousJoystickButtons[i])
							{
								if (joyButtonsAsKeys[i] >= 0)
								{
									Key_Event(joyButtonsAsKeys[i], false);
								}
								else
								{
									Key_Event(key, false);
								}
							}
							key++;
						}
					}
					delete[] previousJoystickButtons;
					previousJoystickButtons = buttons;
					previousJoystickButtonsLength = joystick.get().ButtonCount();
				}
			}
			catch (...)
			{
				// Do nothing - error messages (and Sys_Quit) will already be handled before getting here
			}
			if (sys_errorcalled || sys_quitcalled)
			{
				if (sys_errormessage.length() > 0)
				{
					DisplaySysError();
				}
				return false;
			}
		}
		Sys_Frame(elapsed);
		if (sys_errorcalled || sys_quitcalled)
		{
			if (sys_errormessage.length() > 0)
			{
				DisplaySysError();
			}
			return false;
		}
		byte* data = nullptr;
		check_hresult(screenUpload->Map(0, nullptr, reinterpret_cast<void**>(&data)));
		memcpy(data, vid_buffer.data(), vid_buffer.size());
		screenUpload->Unmap(0, nullptr);
		check_hresult(consoleUpload->Map(0, nullptr, reinterpret_cast<void**>(&data)));
		memcpy(data, con_buffer.data(), con_buffer.size());
		consoleUpload->Unmap(0, nullptr);
		previousTime = time;
		return true;
	}

	void MainPage::Render()
	{
		check_hresult(commandAllocators[currentFrame]->Reset());
		check_hresult(commandList->Reset(commandAllocators[currentFrame].get(), pipelineState.get()));
		PIXBeginEvent(commandList.get(), 0, L"Draw screen contents");
		commandList->SetGraphicsRootSignature(rootSignature.get());
		ID3D12DescriptorHeap* ppHeaps[] = { cbvSrvHeap.get() };
		commandList->SetDescriptorHeaps(1, ppHeaps);
		D3D12_GPU_DESCRIPTOR_HANDLE handle = cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
		auto increment = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		handle.ptr += currentFrame * 4 * increment;
		commandList->SetGraphicsRootDescriptorTable(0, handle);
		handle.ptr += increment;
		commandList->SetGraphicsRootDescriptorTable(1, handle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		D3D12_RESOURCE_BARRIER renderTargetResourceBarrier { };
		renderTargetResourceBarrier.Transition.pResource = renderTargets[currentFrame].get();
		renderTargetResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		renderTargetResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		renderTargetResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &renderTargetResourceBarrier);
		D3D12_RESOURCE_BARRIER transition { };
		transition.Transition.pResource = screen.get();
		transition.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		transition.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		transition.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &transition);
		d3dDevice->GetCopyableFootprints(&screenTextureDesc, 0, 1, 0, &screenUploadLocation.PlacedFootprint, nullptr, nullptr, nullptr);
		commandList->CopyTextureRegion(&screenLocation, 0, 0, 0, &screenUploadLocation, nullptr);
		transition.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		transition.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &transition);
		transition.Transition.pResource = console.get();
		transition.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		transition.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		commandList->ResourceBarrier(1, &transition);
		d3dDevice->GetCopyableFootprints(&consoleTextureDesc, 0, 1, 0, &consoleUploadLocation.PlacedFootprint, nullptr, nullptr, nullptr);
		commandList->CopyTextureRegion(&consoleLocation, 0, 0, 0, &consoleUploadLocation, nullptr);
		transition.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		transition.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &transition);
		if (vid_palettechanged)
		{
			transition.Transition.pResource = palette.get();
			transition.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			transition.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			commandList->ResourceBarrier(1, &transition);
			d3dDevice->GetCopyableFootprints(&paletteTextureDesc, 0, 1, 0, &paletteUploadLocation.PlacedFootprint, nullptr, nullptr, nullptr);
			BYTE* data = nullptr;
			check_hresult(paletteUpload->Map(0, nullptr, reinterpret_cast<void**>(&data)));
			memcpy(data, d_8to24table, 1024);
			paletteUpload->Unmap(0, nullptr);
			commandList->CopyTextureRegion(&paletteLocation, 0, 0, 0, &paletteUploadLocation, nullptr);
			transition.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			transition.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			commandList->ResourceBarrier(1, &transition);
			vid_palettechanged = false;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView { rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + currentFrame * rtvDescriptorSize };
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView { dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr };
		commandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		commandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->DrawInstanced(4, 1, 0, 0);
		D3D12_RESOURCE_BARRIER presentResourceBarrier { };
		presentResourceBarrier.Transition.pResource = renderTargets[currentFrame].get();
		presentResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		presentResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		presentResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &presentResourceBarrier);
		PIXEndEvent(commandList.get());
		check_hresult(commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { commandList.get() };
		commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		auto result = swapChain->Present(1, 0);
		if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
		{
			RemoveDevice();
			return;
		}
		check_hresult(result);
		auto currentFenceValue = fenceValues[currentFrame];
		check_hresult(commandQueue->Signal(fence.get(), currentFenceValue));
		currentFrame = swapChain->GetCurrentBackBufferIndex();
		if (fence->GetCompletedValue() < fenceValues[currentFrame])
		{
			check_hresult(fence->SetEventOnCompletion(fenceValues[currentFrame], fenceEvent));
			if (WaitForSingleObjectEx(fenceEvent, 5000, FALSE) != WAIT_OBJECT_0)
			{
				throw std::runtime_error("WaitForSingleObjectEx failed at rendering stage.");
			}
		}
		fenceValues[currentFrame] = currentFenceValue + 1;
	}

	void MainPage::WaitForGPU(int frame, const char* timeoutError)
	{
		check_hresult(commandQueue->Signal(fence.get(), fenceValues[frame]));
		check_hresult(fence->SetEventOnCompletion(fenceValues[frame], fenceEvent));
		if (WaitForSingleObjectEx(fenceEvent, 5000, FALSE) != WAIT_OBJECT_0)
		{
			throw std::runtime_error(timeoutError);
		}
		fenceValues[frame]++;
	}

	void MainPage::UploadBufferToGPU(D3D12_SUBRESOURCE_DATA data, com_ptr<ID3D12Resource> const& buffer, com_ptr<ID3D12Resource> const& bufferUpload, D3D12_RESOURCE_DESC const& descriptor)
	{
		UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64));
		void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
		if (pMem == NULL)
		{
			throw std::bad_alloc();
		}
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
		auto pRowSizesInBytes = (UINT64*)(pLayouts + 1);
		auto pNumRows = (UINT*)(pRowSizesInBytes + 1);
		UINT64 RequiredSize = 0;
		d3dDevice->GetCopyableFootprints(&descriptor, 0, 1, 0, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
		BYTE* pData = nullptr;
		check_hresult(bufferUpload->Map(0, NULL, reinterpret_cast<void**>(&pData)));
		for (auto i = 0; i < 1; i++)
		{
			D3D12_MEMCPY_DEST DestData { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
			for (UINT j = 0; j < pLayouts[i].Footprint.Depth; j++)
			{
				BYTE* pDestSlice = reinterpret_cast<BYTE*>(DestData.pData) + DestData.SlicePitch * j;
				const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(data.pData) + data.SlicePitch * j;
				for (UINT k = 0; k < pNumRows[i]; k++)
				{
					memcpy(pDestSlice + DestData.RowPitch * k, pSrcSlice + data.RowPitch * k, pRowSizesInBytes[i]);
				}
			}
		}
		bufferUpload->Unmap(0, NULL);
		commandList->CopyBufferRegion(buffer.get(), 0, bufferUpload.get(), pLayouts[0].Offset, pLayouts[0].Footprint.Width);
		HeapFree(GetProcessHeap(), 0, pMem);
		D3D12_RESOURCE_BARRIER transition { };
		transition.Transition.pResource = buffer.get();
		transition.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		transition.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		transition.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &transition);
	}

	void MainPage::CleanupFrameData()
	{
		for (auto i = 0; i < frameCount; i++)
		{
			renderTargets[i] = nullptr;
			fenceValues[i] = fenceValues[currentFrame];
		}
	}

	void MainPage::CreateOutputSize(DisplayInformation const& displayInformation, Size& outputSize)
	{
		auto width = max(1, floor(newScreenWidth * displayInformation.LogicalDpi() / 96 + 0.5f));
		auto height = max(1, floor(newScreenHeight * displayInformation.LogicalDpi() / 96 + 0.5f));
		outputSize.Width = width;
		outputSize.Height = height;
	}

	void MainPage::CreateRotation(DisplayInformation const& displayInformation, DXGI_MODE_ROTATION& rotation)
	{
		switch (displayInformation.NativeOrientation())
		{
		case DisplayOrientations::Landscape:
			switch (displayInformation.CurrentOrientation())
			{
			case DisplayOrientations::Landscape:
				rotation = DXGI_MODE_ROTATION_IDENTITY;
				break;

			case DisplayOrientations::Portrait:
				rotation = DXGI_MODE_ROTATION_ROTATE270;
				break;

			case DisplayOrientations::LandscapeFlipped:
				rotation = DXGI_MODE_ROTATION_ROTATE180;
				break;

			case DisplayOrientations::PortraitFlipped:
				rotation = DXGI_MODE_ROTATION_ROTATE90;
				break;
			}
			break;
		case DisplayOrientations::Portrait:
			switch (displayInformation.CurrentOrientation())
			{
			case DisplayOrientations::Landscape:
				rotation = DXGI_MODE_ROTATION_ROTATE90;
				break;

			case DisplayOrientations::Portrait:
				rotation = DXGI_MODE_ROTATION_IDENTITY;
				break;

			case DisplayOrientations::LandscapeFlipped:
				rotation = DXGI_MODE_ROTATION_ROTATE270;
				break;

			case DisplayOrientations::PortraitFlipped:
				rotation = DXGI_MODE_ROTATION_ROTATE180;
				break;
			}
			break;
		}
	}

	void MainPage::CreateRenderTargetSize(DXGI_MODE_ROTATION& rotation, Size const& size, Size& renderTargetSize)
	{
		if (rotation == DXGI_MODE_ROTATION_ROTATE90 || rotation == DXGI_MODE_ROTATION_ROTATE270)
		{
			renderTargetSize.Width = size.Height;
			renderTargetSize.Height = size.Width;
		}
		else
		{
			renderTargetSize.Width = size.Width;
			renderTargetSize.Height = size.Height;
		}
	}

	void MainPage::CreateOrientationTransform3D(DXGI_MODE_ROTATION rotation, XMFLOAT4X4& orientationTransform3D)
	{
		switch (rotation)
		{
		case DXGI_MODE_ROTATION_IDENTITY:
			orientationTransform3D = Rotation0;
			break;

		case DXGI_MODE_ROTATION_ROTATE90:
			orientationTransform3D = Rotation270;
			break;

		case DXGI_MODE_ROTATION_ROTATE180:
			orientationTransform3D = Rotation180;
			break;

		case DXGI_MODE_ROTATION_ROTATE270:
			orientationTransform3D = Rotation90;
			break;

		default:
			throw std::runtime_error("Unknown rotation mode.");
		}
	}

	void MainPage::UpdateSwapChainTransform(DXGI_MODE_ROTATION rotation, float compositionScaleX, float compositionScaleY)
	{
		check_hresult(swapChain->SetRotation(rotation));
		DXGI_MATRIX_3X2_F inverseScale { };
		inverseScale._11 = 1.0f / compositionScaleX;
		inverseScale._22 = 1.0f / compositionScaleY;
		check_hresult(swapChain->SetMatrixTransform(&inverseScale));
	}

	void MainPage::CreateRenderTargetViews()
	{
		auto handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (auto i = 0; i < frameCount; i++)
		{
			check_hresult(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
			d3dDevice->CreateRenderTargetView(renderTargets[i].get(), nullptr, handle);
			handle.ptr += rtvDescriptorSize;
			renderTargets[i]->SetName(std::wstring(L"renderTarget" + std::to_wstring(i)).c_str());
		}
	}

	void MainPage::CreateDepthStencilBuffer(UINT width, UINT height)
	{
		D3D12_HEAP_PROPERTIES depthHeapProperties { D3D12_HEAP_TYPE_DEFAULT };
		depthHeapProperties.CreationNodeMask = 1;
		depthHeapProperties.VisibleNodeMask = 1;
		D3D12_RESOURCE_DESC depthResourceDesc { D3D12_RESOURCE_DIMENSION_TEXTURE2D };
		depthResourceDesc.Width = width;
		depthResourceDesc.Height = height;
		depthResourceDesc.DepthOrArraySize = 1;
		depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthResourceDesc.SampleDesc.Count = 1;
		depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		D3D12_CLEAR_VALUE depthOptimizedClearValue { DXGI_FORMAT_D32_FLOAT };
		depthOptimizedClearValue.DepthStencil.Depth = 1;
		check_hresult(d3dDevice->CreateCommittedResource(&depthHeapProperties, D3D12_HEAP_FLAG_NONE, &depthResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, IID_PPV_ARGS(&depthStencil)));
		depthStencil->SetName(L"depthStencil");
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc { DXGI_FORMAT_D32_FLOAT, D3D12_DSV_DIMENSION_TEXTURE2D };
		d3dDevice->CreateDepthStencilView(depthStencil.get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void MainPage::CreateScreen()
	{
		screenTextureDesc.Width = lround(newScreenWidth);
		screenTextureDesc.Height = lround(newScreenHeight);
		check_hresult(d3dDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &screenTextureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&screen)));
		screen->SetName(L"screen");
		screenLocation.pResource = screen.get();
		UINT64 uploadBufferSize = 0;
		d3dDevice->GetCopyableFootprints(&screenTextureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
		D3D12_RESOURCE_DESC uploadBufferDesc { D3D12_RESOURCE_DIMENSION_BUFFER };
		uploadBufferDesc.Width = uploadBufferSize;
		uploadBufferDesc.Height = 1;
		uploadBufferDesc.DepthOrArraySize = 1;
		uploadBufferDesc.MipLevels = 1;
		uploadBufferDesc.SampleDesc.Count = 1;
		uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		check_hresult(d3dDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&screenUpload)));
		screenUpload->SetName(L"screenUpload");
		screenUploadLocation.pResource = screenUpload.get();
		d3dDevice->GetCopyableFootprints(&screenTextureDesc, 0, 1, 0, &screenLocation.PlacedFootprint, nullptr, nullptr, nullptr);
		newScreenRowbytes = (float)screenLocation.PlacedFootprint.Footprint.RowPitch;
		consoleTextureDesc.Width = lround(newConsoleWidth);
		consoleTextureDesc.Height = lround(newConsoleHeight);
		check_hresult(d3dDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &consoleTextureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&console)));
		console->SetName(L"console");
		consoleLocation.pResource = console.get();
		uploadBufferSize = 0;
		d3dDevice->GetCopyableFootprints(&consoleTextureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
		uploadBufferDesc.Width = uploadBufferSize;
		check_hresult(d3dDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&consoleUpload)));
		consoleUpload->SetName(L"consoleUpload");
		consoleUploadLocation.pResource = consoleUpload.get();
		d3dDevice->GetCopyableFootprints(&consoleTextureDesc, 0, 1, 0, &consoleLocation.PlacedFootprint, nullptr, nullptr, nullptr);
		newConsoleRowbytes = (float)consoleLocation.PlacedFootprint.Footprint.RowPitch;
	}

	void MainPage::UpdateModelViewProjectionMatrix(XMFLOAT4X4 const& orientationTransform3D)
	{
		scissorRect = { 0, 0, (LONG)viewport.Width, (LONG)viewport.Height };
		float matrix[16];
		memset(matrix, 0, sizeof(matrix));
		matrix[0] = 1;
		matrix[5] = 1;
		matrix[10] = -1;
		matrix[15] = 1;
		XMMATRIX perspectiveMatrix(matrix);
		auto orientationMatrix = XMLoadFloat4x4(&orientationTransform3D);
		XMStoreFloat4x4(&constantBufferData.modelViewProjection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
	}

	void MainPage::DisplaySysError()
	{
		if (sys_nogamedata)
		{
			swapChainPanel().Dispatcher().RunAsync(CoreDispatcherPriority::High, [this]()
				{
					ContentDialog dialog;
					dialog.Title(box_value(L"Game data not found"));
					dialog.Content(box_value(L"Go to Preferences and set Game directory (-basedir) to your copy of the game files."));
					dialog.CloseButtonText(L"Go to Preferences");
					dialog.PrimaryButtonText(L"Where to buy the game");
					dialog.SecondaryButtonText(L"Where to get shareware episode");
					dialog.Closing([](ContentDialog const&, ContentDialogClosingEventArgs const& args)
						{
							auto result = args.Result();
							if (result == ContentDialogResult::Primary)
							{
								Launcher::LaunchUriAsync(Uri(L"https://www.google.com/search?q=buy+quake+1+game"));
							}
							else if (result == ContentDialogResult::Secondary)
							{
								Launcher::LaunchUriAsync(Uri(L"https://www.google.com/search?q=download+quake+1+shareware+episode"));
							}
							args.Cancel(result == ContentDialogResult::Primary || result == ContentDialogResult::Secondary);
						});
					auto task = dialog.ShowAsync();
					task.Completed([this](IAsyncOperation<ContentDialogResult> const& operation, AsyncStatus const&)
						{
							auto result = operation.GetResults();
							if (result != ContentDialogResult::Primary && result != ContentDialogResult::Secondary)
							{
								SettingsContentDialog settings;
								auto task = settings.ShowAsync(ContentDialogPlacement::InPlace);
								task.Completed([](IAsyncOperation<ContentDialogResult> const&, AsyncStatus const&)
									{
										ContentDialog exitDialog;
										exitDialog.Title(box_value(L"Preferences saved"));
										exitDialog.Content(box_value(L"Slip & Frag will now close. Run the application again to play with the new preferences."));
										exitDialog.CloseButtonText(L"Exit");
										auto task = exitDialog.ShowAsync();
										task.Completed([](IAsyncOperation<ContentDialogResult> const& operation, AsyncStatus const&)
											{
												Application::Current().Exit();
											});
									});
							}
						});
				});
		}
		else
		{
			swapChainPanel().Dispatcher().RunAsync(CoreDispatcherPriority::High, [this]()
				{
					std::wstring toDisplay;
					for (auto c : sys_errormessage)
					{
						toDisplay.push_back((wchar_t)c);
					}
					toDisplay += L"\n\nSlip & Frag will now close.";
					ContentDialog dialog;
					dialog.Title(box_value(L"Sys_Error"));
					dialog.Content(box_value(toDisplay));
					dialog.CloseButtonText(L"Close");
					auto task = dialog.ShowAsync();
					task.Completed([this](IAsyncOperation<ContentDialogResult> const&, AsyncStatus const&)
						{
							Application::Current().Exit();
						});
				});
		}
	}

	void MainPage::ProcessFiles()
	{
		auto keepRunning = false;
		do
		{
			std::this_thread::yield();
			fileoperation_t operation = fo_idle;
			{
				std::lock_guard lock(sys_iomutex);
				if (fileOperationRunning)
				{
					continue;
				}
				operation = sys_fileoperation;
			}
			if (operation == fo_openread)
			{
				OpenFileForRead();
			}
			else if (operation == fo_openwrite)
			{
				OpenFileForWrite();
			}
			else if (operation == fo_openappend)
			{
				OpenFileForAppend();
			}
			else if (operation == fo_read)
			{
				ReadFile();
			}
			else if (operation == fo_write)
			{
				WriteToFile();
			}
			else if (operation == fo_time)
			{
				GetFileTime();
			}
			{
				std::lock_guard lock(sys_errormutex);
				keepRunning = (!sys_errorcalled && !sys_quitcalled);
			}
		} while (keepRunning);
	}

	void MainPage::SignalFileOperationError(std::string errorMessage)
	{
		std::lock_guard lock(sys_iomutex);
		sys_fileoperationerror = errorMessage;
		sys_fileoperation = fo_error;
		fileOperationRunning = false;
	}

	void MainPage::OpenFileForRead()
	{
		if (!StorageApplicationPermissions::FutureAccessList().ContainsItem(L"basedir_text"))
		{
			sys_fileoperationerror = "basedir_text token not found in FutureAccessList";
			sys_fileoperation = fo_error;
			return;
		}
		fileOperationRunning = true;
		auto task = StorageApplicationPermissions::FutureAccessList().GetFolderAsync(L"basedir_text");
		task.Completed([this](IAsyncOperation<StorageFolder> const& operation, AsyncStatus const status)
			{
				if (status == AsyncStatus::Error)
				{
					SignalFileOperationError(std::to_string(operation.ErrorCode()));
					return;
				}
				auto folder = operation.GetResults();
				auto path = folder.Path();
				auto i = 0;
				for (; i < (int)sys_fileoperationname.length() && i < (int)path.size(); i++)
				{
					auto c = (wchar_t)sys_fileoperationname[i];
					if (c != path[i])
					{
						break;
					}
				}
				if (i != (int)path.size() || i >= (int)sys_fileoperationname.length())
				{
					SignalFileOperationError("Filename is not a subpath of specified folder");
					return;
				}
				if (sys_fileoperationname[i] == '/' || sys_fileoperationname[i] == '\\')
				{
					i++;
				}
				std::wstring remaining;
				for (; i < sys_fileoperationname.length(); i++)
				{
					auto c = (wchar_t)sys_fileoperationname[i];
					if (c == '/')
					{
						c = '\\';
					}
					remaining.push_back(c);
				}
				auto task = folder.GetFileAsync(remaining);
				task.Completed([this](IAsyncOperation<StorageFile> const& operation, AsyncStatus const status)
					{
						if (status == AsyncStatus::Error)
						{
							SignalFileOperationError(std::to_string(operation.ErrorCode()));
							return;
						}
						sys_files[sys_fileoperationindex].file = operation.GetResults();
						auto task = sys_files[sys_fileoperationindex].file.GetBasicPropertiesAsync();
						task.Completed([this](IAsyncOperation<BasicProperties> const& operation, AsyncStatus const status)
							{
								if (status == AsyncStatus::Error)
								{
									SignalFileOperationError(std::to_string(operation.ErrorCode()));
									return;
								}
								auto basicProperties = operation.GetResults();
								sys_fileoperationsize = (int)basicProperties.Size();
								auto task = sys_files[sys_fileoperationindex].file.OpenAsync(FileAccessMode::Read);
								task.Completed([this](IAsyncOperation<IRandomAccessStream> const& operation, AsyncStatus const status)
									{
										if (status == AsyncStatus::Error)
										{
											SignalFileOperationError(std::to_string(operation.ErrorCode()));
											return;
										}
										sys_files[sys_fileoperationindex].stream = operation.GetResults();
										{
											std::lock_guard lock(sys_iomutex);
											sys_fileoperation = fo_idle;
											fileOperationRunning = false;
										}
									});
							});
					});
			});
	}

	void MainPage::OpenFileForWrite()
	{
		if (!StorageApplicationPermissions::FutureAccessList().ContainsItem(L"basedir_text"))
		{
			sys_fileoperationerror = "basedir_text token not found in FutureAccessList";
			sys_fileoperation = fo_error;
			return;
		}
		fileOperationRunning = true;
		auto task = StorageApplicationPermissions::FutureAccessList().GetFolderAsync(L"basedir_text");
		task.Completed([this](IAsyncOperation<StorageFolder> const& operation, AsyncStatus const status)
			{
				if (status == AsyncStatus::Error)
				{
					SignalFileOperationError(std::to_string(operation.ErrorCode()));
					return;
				}
				auto folder = operation.GetResults();
				auto path = folder.Path();
				auto i = 0;
				for (; i < (int)sys_fileoperationname.length() && i < (int)path.size(); i++)
				{
					auto c = (wchar_t)sys_fileoperationname[i];
					if (c != path[i])
					{
						break;
					}
				}
				if (i != (int)path.size() || i >= (int)sys_fileoperationname.length())
				{
					SignalFileOperationError("Filename is not a subpath of specified folder");
					return;
				}
				if (sys_fileoperationname[i] == '/' || sys_fileoperationname[i] == '\\')
				{
					i++;
				}
				std::wstring remaining;
				for (; i < sys_fileoperationname.length(); i++)
				{
					auto c = (wchar_t)sys_fileoperationname[i];
					if (c == '/')
					{
						c = '\\';
					}
					remaining.push_back(c);
				}
				auto task = folder.CreateFileAsync(remaining, CreationCollisionOption::ReplaceExisting);
				task.Completed([this](IAsyncOperation<StorageFile> const& operation, AsyncStatus const status)
					{
						if (status == AsyncStatus::Error)
						{
							SignalFileOperationError(std::to_string(operation.ErrorCode()));
							return;
						}
						sys_files[sys_fileoperationindex].file = operation.GetResults();
						auto task = sys_files[sys_fileoperationindex].file.OpenAsync(FileAccessMode::ReadWrite);
						task.Completed([this](IAsyncOperation<IRandomAccessStream> const& operation, AsyncStatus const status)
							{
								if (status == AsyncStatus::Error)
								{
									SignalFileOperationError(std::to_string(operation.ErrorCode()));
									return;
								}
								sys_files[sys_fileoperationindex].stream = operation.GetResults();
								{
									std::lock_guard lock(sys_iomutex);
									sys_fileoperation = fo_idle;
									fileOperationRunning = false;
								}
							});
					});
			});
	}

	void MainPage::OpenFileForAppend()
	{
		if (!StorageApplicationPermissions::FutureAccessList().ContainsItem(L"basedir_text"))
		{
			sys_fileoperationerror = "basedir_text token not found in FutureAccessList";
			sys_fileoperation = fo_error;
			return;
		}
		fileOperationRunning = true;
		auto task = StorageApplicationPermissions::FutureAccessList().GetFolderAsync(L"basedir_text");
		task.Completed([this](IAsyncOperation<StorageFolder> const& operation, AsyncStatus const status)
			{
				if (status == AsyncStatus::Error)
				{
					SignalFileOperationError(std::to_string(operation.ErrorCode()));
					return;
				}
				auto folder = operation.GetResults();
				auto path = folder.Path();
				auto i = 0;
				for (; i < (int)sys_fileoperationname.length() && i < (int)path.size(); i++)
				{
					auto c = (wchar_t)sys_fileoperationname[i];
					if (c != path[i])
					{
						break;
					}
				}
				if (i != (int)path.size() || i >= (int)sys_fileoperationname.length())
				{
					SignalFileOperationError("Filename is not a subpath of specified folder");
					return;
				}
				if (sys_fileoperationname[i] == '/' || sys_fileoperationname[i] == '\\')
				{
					i++;
				}
				std::wstring remaining;
				for (; i < sys_fileoperationname.length(); i++)
				{
					auto c = (wchar_t)sys_fileoperationname[i];
					if (c == '/')
					{
						c = '\\';
					}
					remaining.push_back(c);
				}
				auto task = folder.GetFileAsync(remaining);
				task.Completed([this](IAsyncOperation<StorageFile> const& operation, AsyncStatus const status)
					{
						if (status == AsyncStatus::Error)
						{
							SignalFileOperationError(std::to_string(operation.ErrorCode()));
							return;
						}
						sys_files[sys_fileoperationindex].file = operation.GetResults();
						auto task = sys_files[sys_fileoperationindex].file.GetBasicPropertiesAsync();
						task.Completed([this](IAsyncOperation<BasicProperties> const& operation, AsyncStatus const status)
							{
								if (status == AsyncStatus::Error)
								{
									SignalFileOperationError(std::to_string(operation.ErrorCode()));
									return;
								}
								auto basicProperties = operation.GetResults();
								sys_fileoperationsize = (int)basicProperties.Size();
								auto task = sys_files[sys_fileoperationindex].file.OpenAsync(FileAccessMode::ReadWrite);
								task.Completed([this](IAsyncOperation<IRandomAccessStream> const& operation, AsyncStatus const status)
									{
										if (status == AsyncStatus::Error)
										{
											SignalFileOperationError(std::to_string(operation.ErrorCode()));
											return;
										}
										sys_files[sys_fileoperationindex].stream = operation.GetResults();
										sys_files[sys_fileoperationindex].stream.Seek(sys_fileoperationsize);
										{
											std::lock_guard lock(sys_iomutex);
											sys_fileoperation = fo_idle;
											fileOperationRunning = false;
										}
									});
							});
					});
			});
	}

	void MainPage::ReadFile()
	{
		Buffer buffer(sys_fileoperationsize);
		sys_fileoperationsize = 0;
		fileOperationRunning = true;
		auto task = sys_files[sys_fileoperationindex].stream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::None);
		task.Progress([this](IAsyncOperationWithProgress<IBuffer, uint32_t> const& operation, uint32_t const)
			{
				auto buffer = operation.GetResults();
				memcpy(sys_fileoperationbuffer + sys_fileoperationsize, buffer.data(), buffer.Length());
				sys_fileoperationsize += buffer.Length();
			});
		task.Completed([this](IAsyncOperationWithProgress<IBuffer, uint32_t> const& operation, AsyncStatus const status)
			{
				if (status == AsyncStatus::Error)
				{
					SignalFileOperationError(std::to_string(operation.ErrorCode()));
					return;
				}
				auto buffer = operation.GetResults();
				memcpy(sys_fileoperationbuffer + sys_fileoperationsize, buffer.data(), buffer.Length());
				sys_fileoperationsize += buffer.Length();
				{
					std::lock_guard lock(sys_iomutex);
					sys_fileoperation = fo_idle;
					fileOperationRunning = false;
				}
			});
	}

	void MainPage::WriteToFile()
	{
		auto buffer = CryptographicBuffer::CreateFromByteArray({ sys_fileoperationbuffer, sys_fileoperationbuffer + sys_fileoperationsize });
		fileOperationRunning = true;
		auto task = sys_files[sys_fileoperationindex].stream.WriteAsync(buffer);
		task.Completed([this](IAsyncOperationWithProgress<uint32_t, uint32_t> const& operation, AsyncStatus const status)
			{
				if (status == AsyncStatus::Error)
				{
					SignalFileOperationError(std::to_string(operation.ErrorCode()));
					return;
				}
				auto task = sys_files[sys_fileoperationindex].stream.FlushAsync();
				task.Completed([this](IAsyncOperation<bool> const& operation, AsyncStatus const status)
					{
						if (status == AsyncStatus::Error)
						{
							SignalFileOperationError(std::to_string(operation.ErrorCode()));
							return;
						}
						{
							std::lock_guard lock(sys_iomutex);
							sys_fileoperation = fo_idle;
							fileOperationRunning = false;
						}
					});
			});
	}

	void MainPage::GetFileTime()
	{
		if (!StorageApplicationPermissions::FutureAccessList().ContainsItem(L"basedir_text"))
		{
			sys_fileoperationerror = "basedir_text token not found in FutureAccessList";
			sys_fileoperation = fo_error;
			return;
		}
		fileOperationRunning = true;
		auto task = StorageApplicationPermissions::FutureAccessList().GetFolderAsync(L"basedir_text");
		task.Completed([this](IAsyncOperation<StorageFolder> const& operation, AsyncStatus const status)
			{
				if (status == AsyncStatus::Error)
				{
					SignalFileOperationError(std::to_string(operation.ErrorCode()));
					return;
				}
				auto folder = operation.GetResults();
				auto path = folder.Path();
				auto i = 0;
				for (; i < (int)sys_fileoperationname.length() && i < (int)path.size(); i++)
				{
					auto c = (wchar_t)sys_fileoperationname[i];
					if (c != path[i])
					{
						break;
					}
				}
				if (i != (int)path.size() || i >= (int)sys_fileoperationname.length())
				{
					SignalFileOperationError("Filename is not a subpath of specified folder");
					return;
				}
				if (sys_fileoperationname[i] == '/' || sys_fileoperationname[i] == '\\')
				{
					i++;
				}
				std::wstring remaining;
				for (; i < sys_fileoperationname.length(); i++)
				{
					auto c = (wchar_t)sys_fileoperationname[i];
					if (c == '/')
					{
						c = '\\';
					}
					remaining.push_back(c);
				}
				auto task = folder.GetFileAsync(remaining);
				task.Completed([this](IAsyncOperation<StorageFile> const& operation, AsyncStatus const status)
					{
						if (status == AsyncStatus::Error)
						{
							SignalFileOperationError(std::to_string(operation.ErrorCode()));
							return;
						}
						sys_files[sys_fileoperationindex].file = operation.GetResults();
						auto task = sys_files[sys_fileoperationindex].file.GetBasicPropertiesAsync();
						task.Completed([this](IAsyncOperation<BasicProperties> const& operation, AsyncStatus const status)
							{
								if (status == AsyncStatus::Error)
								{
									SignalFileOperationError(std::to_string(operation.ErrorCode()));
									return;
								}
								auto basicProperties = operation.GetResults();
								sys_fileoperationtime = (int)basicProperties.ItemDate().time_since_epoch().count();
								{
									std::lock_guard lock(sys_iomutex);
									sys_fileoperation = fo_idle;
									fileOperationRunning = false;
								}
							});
					});
			});
	}

	void MainPage::FullscreenButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		ApplicationView::GetForCurrentView().TryEnterFullScreenMode();
	}

	void MainPage::RegisterMouseMoved()
	{
		mouseMovedToken = MouseDevice::GetForCurrentView().MouseMoved([](IInspectable const&, MouseEventArgs const& e)
			{
				mx += e.MouseDelta().X;
				my += e.MouseDelta().Y;
			});
		Window::Current().CoreWindow().PointerCursor(nullptr);
	}

	void MainPage::UnregisterMouseMoved()
	{
		Window::Current().CoreWindow().PointerCursor(CoreCursor(CoreCursorType::Arrow, 0));
		MouseDevice::GetForCurrentView().MouseMoved(mouseMovedToken);
	}

	void MainPage::CreateAudioGraph()
	{
		AudioGraphSettings settings(AudioRenderCategory::Media);
		auto task = AudioGraph::CreateAsync(settings);
		task.Completed([=](IAsyncOperation<CreateAudioGraphResult> const& operation, AsyncStatus const status)
			{
				if (status != AsyncStatus::Completed || operation.GetResults().Status() != AudioGraphCreationStatus::Success)
				{
					return;
				}
				audioGraph = operation.GetResults().Graph();
				CreateAudioOutput();
			});
	}

	void MainPage::CreateAudioOutput()
	{
		auto task = audioGraph.CreateDeviceOutputNodeAsync();
		task.Completed([=](IAsyncOperation<CreateAudioDeviceOutputNodeResult> const& operation, AsyncStatus const status)
			{
				if (status != AsyncStatus::Completed || operation.GetResults().Status() != AudioDeviceNodeCreationStatus::Success)
				{
					return;
				}
				audioOutput = operation.GetResults().DeviceOutputNode();
				CreateAudioInput();
			});
	}

	void MainPage::CreateAudioInput()
	{
		AudioEncodingProperties properties = AudioEncodingProperties::CreatePcm(shm->speed, shm->channels, shm->samplebits);
		audioInput = audioGraph.CreateFrameInputNode(properties);
		audioInput.AddOutgoingConnection(audioOutput);
		audioInput.Stop();
		audioInput.QuantumStarted([this](AudioFrameInputNode const&, FrameInputNodeQuantumStartedEventArgs const& e)
			{
				auto samples = e.RequiredSamples();
				if (samples > 0 && shm != nullptr)
				{
					auto bufferSize = samples << 2;
					AudioFrame frame(bufferSize);
					{
						auto buffer = frame.LockBuffer(AudioBufferAccessMode::Write);
						auto reference = buffer.CreateReference();
						auto byteAccess = reference.as<IMemoryBufferByteAccess>();
						byte* data;
						unsigned int capacity;
						byteAccess->GetBuffer(&data, &capacity);
						if (shm != nullptr)
						{
							if (snd_forceclear)
							{
								memset(shm->buffer.data(), 0, shm->samples * shm->samplebits / 8);
								snd_forceclear = false;
							}
							memcpy(data, shm->buffer.data() + (snd_current_sample_pos << 1), capacity);
							snd_current_sample_pos += (samples << 1);
							if (snd_current_sample_pos >= shm->samples)
							{
								snd_current_sample_pos = 0;
							}
						}
					}
					audioInput.AddFrame(frame);
				}
			});
		audioGraph.Start();
		audioInput.Start();
	}

	void MainPage::AddJoystickAxis(IPropertySet const& values, hstring const& stickName, std::string const& axisName, std::vector<std::string>& arguments)
	{
		auto comboName = stickName + L"_combo";
		if (values.HasKey(comboName))
		{
			auto value = values.Lookup(comboName);
			std::wstring text(unbox_value<hstring>(value));
			if (text.size() > 0)
			{
				try
				{
					auto index = std::stoi(text);
					if (index < 5)
					{
						arguments.emplace_back(axisName);
						arguments.emplace_back(std::to_string(index));
						joyAxesAsButtons.emplace_back();
						joyAxesAsButtonsOnRelease.emplace_back();
						joyAxesAsKeys.push_back(-1);
					}
					else 
					{
						auto command = joyCommands[index];
						if (command == "K_ENTER")
						{
							joyAxesAsButtons.emplace_back();
							joyAxesAsButtonsOnRelease.emplace_back();
							joyAxesAsKeys.push_back(13);
						}
						else if (command == "K_ESCAPE")
						{
							joyAxesAsButtons.emplace_back();
							joyAxesAsButtonsOnRelease.emplace_back();
							joyAxesAsKeys.push_back(27);
						}
						else if (command == "K_UPARROW")
						{
							joyAxesAsButtons.emplace_back();
							joyAxesAsButtonsOnRelease.emplace_back();
							joyAxesAsKeys.push_back(128);
						}
						else if (command == "K_LEFTARROW")
						{
							joyAxesAsButtons.emplace_back();
							joyAxesAsButtonsOnRelease.emplace_back();
							joyAxesAsKeys.push_back(130);
						}
						else if (command == "K_RIGHTARROW")
						{
							joyAxesAsButtons.emplace_back();
							joyAxesAsButtonsOnRelease.emplace_back();
							joyAxesAsKeys.push_back(131);
						}
						else if (command == "K_DOWNARROW")
						{
							joyAxesAsButtons.emplace_back();
							joyAxesAsButtonsOnRelease.emplace_back();
							joyAxesAsKeys.push_back(129);
						}
						else
						{
							joyAxesAsButtons.push_back(command);
							if (command[0] == '+')
							{
								joyAxesAsButtonsOnRelease.push_back(std::string("-") + command.substr(1));
							}
							else
							{
								joyAxesAsButtonsOnRelease.emplace_back();
							}
							joyAxesAsKeys.push_back(-1);
						}
					}
				}
				catch (...)
				{
					joyAxesAsButtons.emplace_back();
					joyAxesAsButtonsOnRelease.emplace_back();
					joyAxesAsKeys.push_back(-1);
				}
			}
		}
		auto found = false;
		auto checkName = stickName + L"_check";
		if (values.HasKey(checkName))
		{
			auto value = values.Lookup(checkName);
			if (unbox_value<bool>(value))
			{
				joyAxesInverted.push_back(-1);
				found = true;
			}
		}
		if (!found)
		{
			joyAxesInverted.push_back(1);
		}
	}

	void MainPage::CalculateConsoleDimensions()
	{
		auto factor = newScreenWidth / 320;
		newConsoleWidth = 320;
		newConsoleHeight = ceil(newScreenHeight / factor);
		if (newConsoleHeight < 200)
		{
			factor = newScreenHeight / 200;
			newConsoleHeight = 200;
			newConsoleWidth = (float)(((int)ceil(newScreenWidth / factor) + 3) & ~3);
		}
	}

	void MainPage::HandleKey(int key, bool pressed)
	{
		auto mapped = 0;
		if (key >= 0 && key < sizeof(virtualkeymap) / sizeof(int))
		{
			mapped = virtualkeymap[key];
		}
		if (mapped == 0)
		{
			mapped = MapVirtualKeyA(key, 2); // == MAPVK_VK_TO_CHAR
		}
		capslock_down = ((Window::Current().CoreWindow().GetKeyState(VirtualKey::CapitalLock) & CoreVirtualKeyStates::Locked) == CoreVirtualKeyStates::Locked);
		try
		{
			std::lock_guard lock(renderLoopMutex);
			Key_Event(mapped, pressed);
		}
		catch (...)
		{
			// Do nothing - error messages (and Sys_Quit) will already be handled before getting here
		}
	}
}
