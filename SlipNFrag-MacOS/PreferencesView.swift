import SwiftUI
import AppKit

struct PreferencesView: View {
    enum GameFolder: String, CaseIterable, Identifiable {
        case standard = "standard_quake_radio"
        case hipnotic = "hipnotic_radio"
        case rogue = "rogue_radio"
        case game = "game_radio"

        var id: String { rawValue }
        var title: String {
            switch self {
            case .standard:
                return "Core game"
            case .hipnotic:
                return "Mission Pack 1 (-hipnotic)"
            case .rogue:
                return "Mission Pack 2 (-rogue)"
            case .game:
                return "Other (-game):"
            }
        }
    }

    @State private var baseDirPath = ""
    @State private var selectedGameFolder: GameFolder = .standard
    @State private var gameText = ""
    @State private var commandLineText = ""

    var body: some View {
        VStack(alignment: .leading, spacing: 20) {
            VStack(alignment: .leading, spacing: 6) {
                Text("Game directory (-basedir):")
                TextField("Base game directory", text: $baseDirPath)
					.font(.system(.body, design: .monospaced))
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .disabled(true)
                    .lineLimit(1)

                HStack {
                    Spacer()
                    Button("Choose…", action: chooseBaseDir)
                        .controlSize(.regular)
                }
                .padding(.top, 4)
            }

            VStack(alignment: .leading, spacing: 10) {
                Picker(selection: $selectedGameFolder, label: Text("")) {
                    ForEach(GameFolder.allCases) { pack in
                        Text(pack.title).tag(pack)
                    }
                }
                .pickerStyle(RadioGroupPickerStyle())

                TextField("Game folder", text: $gameText)
					.font(.system(.body, design: .monospaced))
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .disabled(selectedGameFolder != .game)
                    .onChange(of: gameText) { newValue in
                        storeStringUserDefault(key: "game_text", value: newValue)
                    }
            }

            VStack(alignment: .leading, spacing: 6) {
                Text("Additional command-line arguments:")
                TextEditor(text: $commandLineText)
                    .font(.system(.body, design: .monospaced))
                    .frame(minHeight: 92, maxHeight: 92)
                    .padding(4)
                    .overlay(
                        RoundedRectangle(cornerRadius: 5)
                            .stroke(Color(NSColor.separatorColor), lineWidth: 0.8)
                    )
                    .onChange(of: commandLineText) { newValue in
                        storeStringUserDefault(key: "command_line_text", value: newValue)
                    }
            }
			.padding(.top, 30)

            Spacer()
        }
        .padding(16)
        .frame(minWidth: 480, minHeight: 420)
        .onAppear(perform: loadPreferences)
        .onChange(of: selectedGameFolder) { newValue in
            storeGameFolderSelection(newValue)
        }
    }

    private func loadPreferences() {
        loadBaseDirPath()
        gameText = loadStringUserDefault(key: "game_text")
        commandLineText = loadStringUserDefault(key: "command_line_text")
        selectedGameFolder = loadGameFolderSelection()
    }

    private func loadBaseDirPath() {
        guard let bookmarkData = UserDefaults.standard.data(forKey: "basedir_bookmark") else {
            baseDirPath = ""
            return
        }

        var isStale = false
        if let url = try? URL(resolvingBookmarkData: bookmarkData, options: .withSecurityScope, relativeTo: nil, bookmarkDataIsStale: &isStale) {
            baseDirPath = url.path
        } else {
            baseDirPath = ""
        }
    }

    private func loadStringUserDefault(key: String) -> String {
        return UserDefaults.standard.string(forKey: key) ?? ""
    }

    private func loadGameFolderSelection() -> GameFolder {
        let defaults = UserDefaults.standard
        if defaults.bool(forKey: GameFolder.hipnotic.rawValue) {
            return .hipnotic
        }
        if defaults.bool(forKey: GameFolder.rogue.rawValue) {
            return .rogue
        }
        if defaults.bool(forKey: GameFolder.game.rawValue) {
            return .game
        }
        return .standard
    }

    private func storeStringUserDefault(key: String, value: String) {
        if value.isEmpty {
            UserDefaults.standard.removeObject(forKey: key)
        } else {
            UserDefaults.standard.set(value, forKey: key)
        }
    }

    private func storeGameFolderSelection(_ selection: GameFolder) {
        let defaults = UserDefaults.standard
        for pack in GameFolder.allCases {
            if pack == selection {
                defaults.set(true, forKey: pack.rawValue)
            } else {
                defaults.removeObject(forKey: pack.rawValue)
            }
        }
    }

    private func chooseBaseDir() {
        let panel = NSOpenPanel()
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false

        guard panel.runModal() == .OK, let url = panel.url else {
            return
        }

        do {
            let bookmarkData = try url.bookmarkData(options: .withSecurityScope, includingResourceValuesForKeys: nil, relativeTo: nil)
            UserDefaults.standard.set(bookmarkData, forKey: "basedir_bookmark")
            baseDirPath = url.path

            let isValid = try verifyBaseDir(url)
            if !isValid {
                presentInvalidGameFolderAlert(path: url.path)
            }
        } catch {
            presentErrorAlert(error)
        }
    }

    private func verifyBaseDir(_ url: URL) throws -> Bool {
        let exists = FileManager.default.fileExists(atPath: url.appendingPathComponent("ID1/PAK0.PAK").path)
        return exists
    }

    private func presentInvalidGameFolderAlert(path: String) {
        let alert = NSAlert()
        alert.messageText = "Core game data not found"
        alert.informativeText = "The folder \(path) does not contain a folder ID1 with a file PAK0.PAK - the game might not start. Ensure that all required files are present before starting the game."
        alert.alertStyle = .critical
        if let window = NSApp.keyWindow ?? NSApp.mainWindow {
            alert.beginSheetModal(for: window, completionHandler: nil)
        } else {
            alert.runModal()
        }
    }

    private func presentErrorAlert(_ error: Error) {
        let alert = NSAlert(error: error)
        if let window = NSApp.keyWindow ?? NSApp.mainWindow {
            alert.beginSheetModal(for: window, completionHandler: nil)
        } else {
            alert.runModal()
        }
    }
}

