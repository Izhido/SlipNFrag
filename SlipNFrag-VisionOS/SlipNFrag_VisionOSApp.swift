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
	@State var stopEngine = false
	@State var stopEngineMessage = ""

	var size = CGSize(width: 960, height: 600)
	
	var body: some Scene {
		WindowGroup(id: "WindowGroup") {
			PlayView(
				stopEngine: $stopEngine,
				stopEngineMessage: $stopEngineMessage,
				size: size
			).alert("Slip & Frag", isPresented: $stopEngine) {
				Button("Close") {
					exit(0)
				}
			} message: {
				Text("\n" + stopEngineMessage + "\n")
			}
		}.defaultSize(size)
	}
}
