//
//  MainView.swift
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 26/4/26.
//  Copyright © 2026 Heriberto Delgado. All rights reserved.
//

import SwiftUI

struct MainView: View {
	@State private var isPlaying = false

	var body: some View {
		ZStack {
			if isPlaying {
				GameViewControllerRepresentable()
			} else {
				launchScreen
			}
		}
	}

	private var launchScreen: some View {
		ZStack(alignment: .bottomTrailing) {
			VStack {
				Spacer()
				Button(action: { isPlaying = true }) {
					Image("play")
						.resizable()
						.scaledToFit()
						.frame(width: 150, height: 150)
				}
				.buttonStyle(PlainButtonStyle())
				Spacer()
			}
			.frame(maxWidth: .infinity, maxHeight: .infinity)

			Button(action: openPreferences) {
				Image(systemName: "gear")
					.font(.title2)
					.foregroundColor(.white)
			}
			.buttonStyle(PlainButtonStyle())
			.padding(.trailing, 10)
			.padding(.bottom, 10)
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
	}

	private func openPreferences() {
		SettingsManager.shared.openSettings()
	}
}

private struct GameViewControllerRepresentable: NSViewControllerRepresentable {
	func makeNSViewController(context: Context) -> ViewController {
		return ViewController()
	}

	func updateNSViewController(_ nsViewController: ViewController, context: Context) {
		if !context.coordinator.started {
			nsViewController.startGame()
			context.coordinator.started = true
		}
	}

	func makeCoordinator() -> Coordinator {
		Coordinator()
	}

	class Coordinator {
		var started = false
	}
}
