//
//  SlipNFrag_VisionOSApp.swift
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 22/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

import SwiftUI
import CompositorServices

@main
struct SlipNFrag_VisionOSApp: App {
	@State private var stopGame = false
	@State private var stopGameMessage = ""
	
	@Environment(\.dismissImmersiveSpace) var dismissImmersiveSpace
	@Environment(\.dismissWindow) private var dismissWindow
	@Environment(\.openWindow) private var openWindow

	var body: some Scene {
		WindowGroup(id: "WindowGroup") {
			PlayView(hidePlayButton: $stopGame)
				.alert("Slip & Frag", isPresented: $stopGame) {
					Button("Close") {
						exit(0)
					}
				} message: {
					Text("\n" + stopGameMessage + "\n")
				}
		}
		ImmersiveSpace(id: "ImmersiveSpace") {
			CompositorLayer() { layerRenderer in
				let engineStop = EngineStop()
				let engineThread = Thread {
					let url = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
					NSLog(url.path)
					if (url.startAccessingSecurityScopedResource())
					{
						let found = FileManager.default.fileExists(atPath: url.path + "/id1/pak0.pak")
						if (found)
						{
							let args = [
								"SlipNFrag-VisionOS",
								"-basedir",
								url.path
							]
							let engine = Engine()
							engine.start(args, engineStop: engineStop)
						}
						else
						{
							engineStop.stopEngineMessage = "The folder " + url.path + " does not contain a folder 'id1' with a file 'pak0.pak' - the game will not start.\n\nEnsure that all required files are present before starting the game."
							engineStop.stopEngine = true
						}
						url.stopAccessingSecurityScopedResource()
					}
					else
					{
						engineStop.stopEngineMessage = "Permission denied to access the folder " + url.path + " with the game assets."
						engineStop.stopEngine = true
					}
				}
				let rendererThread = Thread {
					Task {
						dismissWindow(id: "WindowGroup")
					}
					let renderer = Renderer()
					renderer.start(layerRenderer, engineStop: engineStop)
					Task {
						await dismissImmersiveSpace()
					}
					Task {
						openWindow(id: "WindowGroup")
					}
					if (engineStop.stopEngineMessage != nil)
					{
						stopGameMessage = engineStop.stopEngineMessage
						stopGame = true
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
}
