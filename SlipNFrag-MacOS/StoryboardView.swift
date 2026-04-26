//
//  StoryboardView.swift
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 26/4/26.
//  Copyright © 2026 Heriberto Delgado. All rights reserved.
//

import SwiftUI

struct StoryboardView: NSViewControllerRepresentable {
	func makeNSViewController(context: Context) -> NSViewController {
		let storyboard = NSStoryboard(name: "Main", bundle: nil)
		
		guard let vc = storyboard.instantiateInitialController() as? NSViewController else {
			fatalError("Check your Storyboard's 'Is Initial Controller' setting.")
		}
		return vc
	}

	func updateNSViewController(_ nsViewController: NSViewController, context: Context) {
	}
}
