//
//  PlayView.swift
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 22/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

import SwiftUI
import RealityKit

struct PlayView: View {
	@Binding var stopEngine: Bool
	@Binding var stopEngineMessage: String

	@State var size: CGSize
	@State var hidePlayButton = false
	@State var startEngine = false
	@State var engineStarted = false
	@State var engineStop = EngineStop()

	var body: some View {
		if !$hidePlayButton.wrappedValue {
			Button(action: {
				hidePlayButton = true
				startEngine = true
			}) { Image("play") }
		}
		if $startEngine.wrappedValue {
			TimelineView(.animation) { timelineContext in
				Canvas { context, size in
					let date = timelineContext.date
					if engineStarted {
						context.withCGContext { cgContext in
							Renderer.renderFrame(cgContext, size: size, date: date)
						}
					} else {
						context.fill(Path(CGRect(x: 0, y: 0, width: size.width, height: size.height)), with: .color(.black))
					}
				}
			}.onAppear {
				let engineThread = Thread {
					let url = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
					NSLog(url.path)
					if url.startAccessingSecurityScopedResource()
					{
						let found = FileManager.default.fileExists(atPath: url.path + "/id1/pak0.pak")
						if found
						{
							let args = [
								"SlipNFrag-VisionOS",
								"-basedir",
								url.path
							]
							let engine = Engine()
							engine.start(args, size:size, engineStop: engineStop)
							if engineStop.stopEngine {
								stopEngineMessage = engineStop.stopEngineMessage
								stopEngine = engineStop.stopEngine
							} else {
								engineStarted = true
								engine.loopEngine(engineStop)
								if engineStop.stopEngineMessage != nil {
									stopEngineMessage = engineStop.stopEngineMessage
									stopEngine = true
								} else if engineStop.stopEngine {
									exit(0)
								}
							}
						}
						else
						{
							stopEngineMessage = "The folder " + url.path + " does not contain a folder 'id1' with a file 'pak0.pak' - the game will not start.\n\nEnsure that all required files are present before starting the game."
							stopEngine = true
						}
						url.stopAccessingSecurityScopedResource()
					}
					else
					{
						stopEngineMessage = "Permission denied to access the folder " + url.path + " with the game assets."
						stopEngine = true
					}
				}
				engineThread.name = "Engine Thread"
				engineThread.start()
			}
		}
	}
}
