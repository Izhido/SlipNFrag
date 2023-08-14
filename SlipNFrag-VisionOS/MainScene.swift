//
//  MainScene.swift
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 14/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

import SwiftUI
import CompositorServices

struct MainScene: Scene {
	@Environment(\.scenePhase) private var scenePhase
	@Environment(\.openWindow) private var openWindow
	@Environment(\.dismissWindow) private var dismissWindow
	@Environment(\.dismissImmersiveSpace) private var dismissImmersiveSpace

	@State private var stopEngine = false
	@State private var stopEngineMessage = ""
	@State private var stopEngineExit = true
	@State private var exitUponBackground = true

	private var size = CGSize(width: 1280, height: 800)
	
	var body: some Scene {
		WindowGroup(id: "MainWindowGroup") {
			PlayView(
				stopEngine: $stopEngine,
				stopEngineMessage: $stopEngineMessage,
				stopEngineExit: $stopEngineExit,
				exitUponBackground: $exitUponBackground,
				size: size
			).alert("Slip & Frag", isPresented: $stopEngine) {
				Button("Close") {
					if stopEngineExit {
						exit(0)
					}
				}
			} message: {
				Text("\n" + stopEngineMessage + "\n")
			}
		}
		.defaultSize(size)
		.onChange(of: scenePhase) {
			if scenePhase == .background {
				if (exitUponBackground) {
					exit(0)
				}
				exitUponBackground = true
			}
		}
		
		WindowGroup(id: "SettingsWindowGroup") {
			SettingsView()
		}
		.defaultSize(width: 500, height: 550)
		
		ImmersiveSpace(id: "MainImmersiveSpace") {
			CompositorLayer() { layerRenderer in
				let engineStop = EngineStop()

				let engineThread = Thread {
					dismissWindow(id: "MainWindowGroup")
					if SettingsView.openCount > 0 {
						dismissWindow(id: "SettingsWindowGroup")
						SettingsView.openCount = 0
					}

					var available = false
					var url : URL?

					SettingsView.gameAssetsAvailable(
						keepSecurityScopedResourceOpen: true,
						securityScopedResourceAvailable: &available,
						urlToAssets: &url
					)
					if !available {

						dismissAndAlert(message: "Permission denied to access the folder with the game assets.")

					} else if url == nil {

						dismissAndAlert(message: "No game assets folder 'id1' with a file 'pak0.pak' was found - the game will not start.\n\nEnsure that all required files are present before starting the game.")

					} else {
						let args = SettingsView.buildCommandLine(url: url!)

						let engine = Engine()
						engine.start(args, size:size, engineStop: engineStop)
						if engineStop.stopEngine {

							dismissAndAlert(message: engineStop.stopEngineMessage)

						} else {
							engine.loopEngine(engineStop)

							if engineStop.stopEngineMessage != nil {

								dismissAndAlert(message: engineStop.stopEngineMessage)

							} else if engineStop.stopEngine {
								Task {
									await dismissImmersiveSpace()
								}

								exit(0)
							}
						}

						url!.stopAccessingSecurityScopedResource()
					}
				}

				let rendererThread = Thread {
					let renderer = Renderer()
					renderer.start(layerRenderer, engineStop: engineStop)

					Task {
						await dismissImmersiveSpace()
					}

					if engineStop.stopEngineMessage != nil
					{
						Task {
							await dismissImmersiveSpace()
							exitUponBackground = false
							openWindow(id: "MainWindowGroup")
						}
						stopEngineMessage = engineStop.stopEngineMessage
						stopEngineExit = true
						stopEngine = true
					}
					else
					{
						exit(0)
					}
				}

				engineThread.name = "Engine Thread"
				engineThread.start()

				rendererThread.name = "Renderer Thread"
				rendererThread.start()
			}
		}
	}
	
	private func dismissAndAlert(message: String) {
		Task {
			await dismissImmersiveSpace()
			exitUponBackground = false
			openWindow(id: "MainWindowGroup")
		}
		stopEngineMessage = message
		stopEngineExit = true
		stopEngine = true
	}
}
