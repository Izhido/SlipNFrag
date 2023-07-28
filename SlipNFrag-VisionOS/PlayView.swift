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
    @Environment(\.openImmersiveSpace) var openImmersiveSpace

    var body: some View {
		VStack {
			Button(action: {
				Task {
					await openImmersiveSpace(id: "ImmersiveSpace")
				}
			}) { Image("play") }
		}
    }
}
