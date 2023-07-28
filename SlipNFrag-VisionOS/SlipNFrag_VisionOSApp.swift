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
	
	var body: some Scene {
		WindowGroup(id: "WindowGroup") {
			PlayView()
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
				let renderThread = Thread {
					let url = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
					NSLog(url.path)
					if (url.startAccessingSecurityScopedResource())
					{
						let args = [
							"SlipNFrag-VisionOS",
							"-basedir",
							url.path
						]
						Task {
							dismissWindow(id: "WindowGroup")
						}
						let engine = Engine()
						engine.startGame(args, layerRenderer: layerRenderer)
						url.stopAccessingSecurityScopedResource()
						Task {
							await dismissImmersiveSpace()
						}
						if (engine.stopGameMessage != nil)
						{
							stopGameMessage = engine.stopGameMessage
							stopGame = true
						}
					}
					else
					{
						stopGameMessage = "Unable to find game assets."
						stopGame = true
					}
				}
				renderThread.name = "Render Thread"
				renderThread.start()
			}
		}
	}
}
