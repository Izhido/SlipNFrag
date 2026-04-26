//
//  PreferencesStoryboardView.swift
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 26/4/26.
//  Copyright © 2026 Heriberto Delgado. All rights reserved.
//

import SwiftUI

struct PreferencesStoryboardView: NSViewControllerRepresentable {
	func makeNSViewController(context: Context) -> NSViewController {
		let storyboard = NSStoryboard(name: "Main", bundle: nil)
		guard let vc = storyboard.instantiateController(withIdentifier: "Preferences") as? NSViewController else {
			fatalError("Could not find a View Controller with identifier 'Preferences'")
		}
		return vc
	}

	func updateNSViewController(_ nsViewController: NSViewController, context: Context) {
	}
}
