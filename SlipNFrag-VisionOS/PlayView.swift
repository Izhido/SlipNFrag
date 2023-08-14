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
	@Environment(\.dismiss) private var dismiss
	@Environment(\.openWindow) private var openWindow
	@Environment(\.dismissWindow) private var dismissWindow
	@Environment(\.openImmersiveSpace) private var openImmersiveSpace

	@Binding var stopEngine: Bool
	@Binding var stopEngineMessage: String
	@Binding var stopEngineExit: Bool
	@Binding var exitUponBackground: Bool

	@State var size: CGSize
	@State private var hidePlayButton = false
	@State private var startPlanarEngine = false
	@State private var engineStarted = false
	@State private var engineStop = EngineStop()
	@State private var engineMode = EngineMode.immersive
	
	var body: some View {
		ZStack {
			if !$hidePlayButton.wrappedValue {
				Button(action: {
					if engineMode == .planar {
						hidePlayButton = true
						startPlanarEngine = true
					} else {
						Task {
							exitUponBackground = false
							await openImmersiveSpace(id: "MainImmersiveSpace")
						}
					}
				}, label: {
					Image("play")
				})

				VStack {
					Spacer()

					Picker("Mode", selection: $engineMode) {
						Image(systemName:"cube")
						.tag(EngineMode.immersive)
						Image(systemName:"tv")
						.tag(EngineMode.planar)
					}
					.pickerStyle(.segmented)
					.frame(width: 100, height: 100)
					.scaledToFit()
					.scaleEffect(CGSize(width: 2, height: 2))
				}

				VStack {
					Spacer()

					HStack {
						Spacer()
						
						Button(action: {
							if SettingsView.openCount > 0 {
								dismissWindow(id: "SettingsWindowGroup")
								SettingsView.openCount = 0
							}
							openWindow(id: "SettingsWindowGroup")
							SettingsView.openCount = 1
						}, label: {
							Image(systemName: "gearshape")
							.frame(width: 30, height: 30)
							.scaledToFit()
							.scaleEffect(CGSize(width: 2, height: 2))
						})
						.padding(.horizontal, 30)
						.padding(.vertical, 30)
					}
				}
			}
			if $startPlanarEngine.wrappedValue {
				TimelineView(.animation) { timelineContext in
					Canvas { context, size in
						let date = timelineContext.date
						if engineStarted && !engineStop.stopEngine {
							context.withCGContext { cgContext in
								Renderer.renderFrame(cgContext, size: size, date: date)
							}
						} else {
							context.fill(Path(CGRect(x: 0, y: 0, width: size.width, height: size.height)), with: .color(.black))
						}
					}
				}.onAppear {
					let engineThread = Thread {
						var available = false
						var url : URL?

						SettingsView.gameAssetsAvailable(
							keepSecurityScopedResourceOpen: true,
							securityScopedResourceAvailable: &available,
							urlToAssets: &url
						)
						if !available {

							stopEngineMessage = "Permission denied to access the folder with the game assets."
							stopEngineExit = true
							stopEngine = true

						} else if url == nil {

							stopEngineMessage = "No game assets folder 'id1' with a file 'pak0.pak' was found - the game will not start.\n\nEnsure that all required files are present before starting the game."
							stopEngineExit = true
							stopEngine = true

						} else {
							let args = SettingsView.buildCommandLine(url: url!)

							let engine = Engine()
							engine.start(args, size:size, engineStop: engineStop)
							if engineStop.stopEngine {

								stopEngineMessage = engineStop.stopEngineMessage
								stopEngineExit = true
								stopEngine = engineStop.stopEngine

							} else {
								engineStarted = true
								engine.loopEngine(engineStop)
								if engineStop.stopEngineMessage != nil {

									stopEngineMessage = engineStop.stopEngineMessage
									stopEngineExit = true
									stopEngine = true

								}
							}

							url!.stopAccessingSecurityScopedResource()
							
							if (engineStop.stopEngine && engineStop.stopEngineMessage == nil) {
								dismiss()
							}
						}
					}

					engineThread.name = "Engine Thread"
					engineThread.start()
				}
			}
		}
	}
}
