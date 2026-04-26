//
//  SettingsManager.swift
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 26/4/26.
//  Copyright © 2026 Heriberto Delgado. All rights reserved.
//

import SwiftUI

@MainActor
@objc class SettingsManager: NSObject {
	@objc static let shared = SettingsManager()
	
	@objc func openSettings() {
		guard let appMenu = NSApp.mainMenu?.item(at: 0)?.submenu else { return }
		for item in appMenu.items {
			if item.action?.description.contains("showSettings") == true ||
			   item.action?.description.contains("showPreferences") == true {
				NSApp.sendAction(item.action!, to: item.target, from: item)
				return
			}
		}
		let titles = ["Settings…", "Preferences…", "Settings...", "Preferences..."]
		for title in titles {
			if let item = appMenu.item(withTitle: title) {
				NSApp.sendAction(item.action!, to: item.target, from: item)
				return
			}
		}
	}
}
