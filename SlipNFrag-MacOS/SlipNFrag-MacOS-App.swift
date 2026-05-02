//
//  SlipNFrag-MacOS-App.swift
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 26/4/26.
//  Copyright © 2026 Heriberto Delgado. All rights reserved.
//

import SwiftUI

@main
struct SlipNFrag_MacOS_App: App {
	@NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

	var body: some Scene {
		WindowGroup {
			StoryboardView()
				.edgesIgnoringSafeArea(.all)
		}
		Settings {
			PreferencesView()
				.frame(width: 490, height: 430)
		}
	}
}
