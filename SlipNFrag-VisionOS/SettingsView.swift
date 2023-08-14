//
//  SettingsView.swift
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 12/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

import SwiftUI

struct SettingsView: View {
	@State private var available = false
	@State private var url : URL? = nil
	@State private var mode : GameMode = .standard
	@State private var customGame : String = ""
	@State private var commandLine : String = ""

	static var openCount = 0

	var body: some View {
		TabView {
			VStack(alignment: .leading) {
				if !available {
					Text("Access to the game assets directory DENIED.")
					.padding(.vertical, 30)
				} else if url == nil {
					Text("Game assets WERE NOT FOUND. Transfer a copy of the \"id1\" folder containing the pak0.pak file in order to play.")
					.padding(.vertical, 30)
					.fixedSize(horizontal: false, vertical: true)
				} else {
					Text("Game assets are available.")
					.padding(.vertical, 30)
				}

				Button(action: {
					mode = .standard
				}, label: {
					HStack {
						ZStack {
							Circle()
							.strokeBorder(.primary, lineWidth: 1)
							.frame(width: 30, height: 30)

							if mode == .standard {
								Circle()
								.frame(width: 20, height: 20)
							}
						}

						Text("Core game")
					}
				})
				.buttonStyle(.plain)

				Button(action: {
					mode = .hipnotic
				}, label: {
					HStack {
						ZStack {
							Circle()
							.strokeBorder(.primary, lineWidth: 1)
							.frame(width: 30, height: 30)

							if mode == .hipnotic {
								Circle()
								.frame(width: 20, height: 20)
							}
						}

						Text("Mission Pack 1 (-hipnotic)")
					}
				})
				.buttonStyle(.plain)

				Button(action: {
					mode = .rogue
				}, label: {
					HStack {
						ZStack {
							Circle()
							.strokeBorder(.primary, lineWidth: 1)
							.frame(width: 30, height: 30)

							if mode == .rogue {
								Circle()
								.frame(width: 20, height: 20)
							}
						}

						Text("Mission Pack 2 (-rogue)")
					}
				})
				.buttonStyle(.plain)

				Button(action: {
					mode = .custom
				}, label: {
					HStack {
						ZStack {
							Circle()
							.strokeBorder(.primary, lineWidth: 1)
							.frame(width: 30, height: 30)

							if mode == .custom {
								Circle()
								.frame(width: 20, height: 20)
							}
						}

						Text("Other (-game):")
					}
				})
				.buttonStyle(.plain)

				TextField("", text: $customGame)
				.textInputAutocapitalization(.never)
				.disableAutocorrection(true)
				.border(.primary)

				Spacer()

				Text("Additional command-line arguments:")
					
				TextField("", text: $commandLine, axis: .vertical)
				.lineLimit(5, reservesSpace: true)
				.textInputAutocapitalization(.never)
				.disableAutocorrection(true)
				.border(.primary)
				.padding(.bottom, 30)
			}
			.padding(.horizontal, 30)
			.tabItem { Label("General", systemImage: "gearshape") }
		}
		.onAppear() {
			SettingsView.gameAssetsAvailable(
				keepSecurityScopedResourceOpen: false,
				securityScopedResourceAvailable: &available,
				urlToAssets: &url
			)
			mode = SettingsView.savedGameMode()
			customGame = SettingsView.savedCustomGame()
			commandLine = SettingsView.savedCommandLine()
		}
		.onChange(of: mode) {
			if mode == .standard {
				UserDefaults.standard.removeObject(forKey: "gameMode")
			} else {
				UserDefaults.standard.setValue(mode.rawValue, forKey: "gameMode")
			}
		}
		.onChange(of: customGame) {
			if customGame.trimmingCharacters(in: .whitespacesAndNewlines) == "" {
				UserDefaults.standard.removeObject(forKey: "customGame")
			} else {
				UserDefaults.standard.setValue(customGame, forKey: "customGame")
			}
		}
		.onChange(of: commandLine) {
			if commandLine.trimmingCharacters(in: .whitespacesAndNewlines) == "" {
				UserDefaults.standard.removeObject(forKey: "commandLine")
			} else {
				UserDefaults.standard.setValue(commandLine, forKey: "commandLine")
			}
		}
	}
	
	static func savedGameMode() -> GameMode {
		let value = UserDefaults.standard.string(forKey: "gameMode")
		if value == nil {
			return .standard
		}
		return GameMode(rawValue: value!)!
	}

	static func savedCustomGame() -> String {
		let value = UserDefaults.standard.string(forKey: "customGame")
		if value == nil {
			return ""
		}
		return value!
	}

	static func savedCommandLine() -> String {
		let value = UserDefaults.standard.string(forKey: "commandLine")
		if value == nil {
			return ""
		}
		return value!
	}

	static func gameAssetsAvailable(
		keepSecurityScopedResourceOpen: Bool,
		securityScopedResourceAvailable: inout Bool,
		urlToAssets: inout URL?
	) {
		let url = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
		print(url.path)
		securityScopedResourceAvailable = url.startAccessingSecurityScopedResource()
		if securityScopedResourceAvailable {
			let found = FileManager.default.fileExists(atPath: url.path + "/id1/pak0.pak")
			if found {
				urlToAssets = url
			}
			if !keepSecurityScopedResourceOpen {
				url.stopAccessingSecurityScopedResource()
			}
		}
	}
	
	static func buildCommandLine(url: URL) -> Array<String> {
		var args = [
			"SlipNFrag-VisionOS",
			"-basedir",
			url.path
		]

		let mode = savedGameMode()
		if mode == .hipnotic {
			args.append("-hipnotic")
		} else if mode == .rogue {
			args.append("-rogue")
		} else if mode == .custom {
			let custom = savedCustomGame()
			
			if custom != "" {
				args.append("-game")
				args.append(custom)
			}
		}
		
		let commandLine = savedCommandLine()
		if commandLine != "" {
			let components = commandLine.components(separatedBy: .whitespacesAndNewlines)
			
			for component in components {
				if component.trimmingCharacters(in: .whitespacesAndNewlines) != "" {
					args.append(component)
				}
			}
		}
		
		return args
	}
}

