# Global Chat: Rocket League Plugin

<p align="center">
  <a href="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX">
    <img src="https://img.shields.io/badge/BakkesMod-Download%20Plugin-blue" alt="Download">
  </a>
  <a href="https://github.com/eimantaslimba/global-chat/releases/latest">
    <img src="https://img.shields.io/github/v/release/eimantaslimba/global-chat" alt="Latest Release">
  </a>
  <a href="https://discord.com/invite/vBvpKG49RW">
    <img src="https://img.shields.io/discord/1340436718817644684?color=7289DA&label=Discord&logo=discord&logoColor=white" alt="Discord">
  </a>
  <a href="https://github.com/eimantaslimba/global-chat/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/eimantaslimba/global-chat" alt="License">
  </a>
</p>

Ever wanted to chat with other Rocket League players, no matter what lobby you're in? **Global Chat** connects you to a global chat room right inside the game, allowing you to talk, find teammates, and interact with the community in real-time.

See who you're talking to with **rank tags** displayed next to each player's name, so you always know who has the skills to back up their chat!

---

## ✨ Features

- **🌐 Live Chat Room:** Connect to a persistent chat server and talk with every other player who has the plugin installed.
- **🏆 Rank Tags:** Automatically displays each user's highest-achieved competitive rank next to their name. (e.g., `[SSL]`, `[GC2]`, `[C1]`)
- **💬 Clean In-Game UI:** A simple, intuitive, and draggable chat window that integrates smoothly with the Rocket League experience.
- **🚀 Real-Time Communication:** Powered by a C++ Boost WebSockets for fast and reliable messaging.
- **⚙️ Easy to Use:** Simply open the chat window with a hotkey or via plugin settings and start typing.

---

## 📥 Installation

There are two ways to install the plugin. The first method is highly recommended for most users.

### Recommended Method (via BakkesMod Plugin Manager)

1.  Go to the **Global Chat** page on the [BakkesMod Plugins website](https://XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.com)
2.  Click "Install with BakkesMod".
3.  The plugin will be automatically installed and updated. That's it!

### Manual Installation (from GitHub)

1.  Go to the [Releases page](https://github.com/eimantaslimba/global-chat/releases/latest).
2.  Download the latest `GlobalChat.zip` file.
3.  Extract the ZIP file into your BakkesMod `plugins` folder. (Open BakkesMod -> File -> Open BakkesMod Folder -> `plugins`)
4.  Load the plugin by restarting Rocket League or by typing `plugin load globalchat` in the BakkesMod console (F6).

---

## 🎮 How to Use

1.  **Open the Chat Window:**

    - Press **F3** to open the Chat window.
    - Or go to plugin settings and press Show/Hide

2.  **Start Chatting:**
    - Click on the input box at the bottom of the chat window.
    - Type your message and press **Enter** to send.
    - Your message, name, and rank tag will appear in the chat for everyone to see!

---

## 🛠️ How It Works (For the Nerds)

This plugin is more than just a simple overlay. It consists of two main components:

1.  **The BakkesMod Plugin (Client):**

    - Written in C++.
    - Handles rendering the ImGui user interface in-game.
    - Fetches your Steam/Epic ID and peak rank data via the BakkesMod API.
    - Uses **Boost.Beast** to establish and maintain a persistent WebSocket connection to our central server.

2.  **The WebSocket Server (Backend):**
    - A Node JS application, running on **Render** server.
    - Accepts connections from all plugin clients.
    - When a message is received from a client, it authenticates the user's data (name, rank) and then broadcasts the message to all other connected clients in real-time.

This client-server architecture ensures that communication is fast, efficient, and scalable.

---

## 🐛 Reporting Bugs & Suggestions

Found a bug or have a great idea for a new feature?

- **[Join our Discord](https://discord.com/invite/vBvpKG49RW):** for the quickest support.
- **Create a GitHub Issue:** If you're more technically inclined, feel free to open a new issue on our [Issues Page](https://github.com/eimantaslimba/global-chat/issues). Please provide as much detail as possible!

---

## 🤝 Contributing

Contributions are welcome! If you'd like to help improve **Global Chat**, please feel free to fork the repository, make your changes, and submit a pull request.

### Building from Source

1.  Clone the repository.
2.  Ensure you have the BakkesMod SDK set up correctly.
3.  Setup vcpkg
4.  Install `boost-beast:x64-windows-static` and `openssl:x64-windows-static`
5.  Link them to visual studio

---

## 🙏 Acknowledgements

- The **BakkesMod Team** for creating an incredible framework for the community.
- The developers of the **Boost C++ Libraries** for the powerful `Beast` and `Asio` libraries that power our communication.

---

## 📜 License

Distributed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for more information.
