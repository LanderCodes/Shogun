# SHOGUN online - install guide

This is the self-contained SHOGUN build with local and online two-player play.
The main game outside this folder is untouched.

## Requirements

- C99 compiler
- CMake 3.16 or newer
- pkg-config
- SDL2 development files
- Git

On Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libsdl2-dev git
```

On Fedora:

```bash
sudo dnf install gcc cmake pkgconf-pkg-config SDL2-devel git
```

Building from source on Windows needs a C99 compiler, CMake, pkgconf and SDL2.
Use `online/build-windows.ps1` or `online/build-windows.bat`; see the Windows
build section below. Playing the prepared Windows build needs none of those
tools.

## Build

From the main game directory:

```bash
cmake -S online -B online/build
cmake --build online/build -j2
ctest --test-dir online/build --output-on-failure
```

This creates `online/build/shogun` and `online/build/shogun-server`.

## Windows install

For playing, copy the prepared Windows folder containing `shogun.exe` and
`SDL2.dll`. Keep both files together and double-click `shogun.exe`. No `pacman`,
CMake, Git or compiler is needed.

If you are hosting the server on Windows, also include `shogun-server.exe` and
`SDL2.dll` in the same folder.

## Windows build

From PowerShell in the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\online\build-windows.ps1
```

Or double-click `online\build-windows.bat`.

The script expects MinGW-w64, CMake, pkg-config and SDL2 development files in
`C:\msys64\ucrt64\bin`. For another MinGW-w64 installation:

```powershell
.\online\build-windows.ps1 -MingwBin "C:\path\to\mingw\bin"
```

It runs the tests unless `-SkipTests` is passed, then copies `shogun.exe`,
`shogun-server.exe` and `SDL2.dll` to `online\windows-release`.

## Run the server

The server needs a shared access token in `SHOGUN_TOKEN`. Generate one and keep
it private:

```bash
openssl rand -hex 24
```

For a one-off server:

```bash
SHOGUN_TOKEN='paste-the-token-here' ./online/build/shogun-server
```

On Windows, run this in Command Prompt from the folder containing
`shogun-server.exe`:

```bat
set SHOGUN_TOKEN=paste-the-token-here
shogun-server.exe
```

For systemd, create an environment file containing only:

```text
SHOGUN_TOKEN=paste-the-token-here
```

Store it as `/etc/shogun-server.env`, set `chmod 600`, add this line under
`[Service]` in `/etc/systemd/system/shogun-server.service`, then reload:

```ini
EnvironmentFile=/etc/shogun-server.env
```

```bash
sudo systemctl daemon-reload
sudo systemctl restart shogun-server
sudo systemctl status shogun-server
```

The server listens on TCP/UDP port `7777` by default.

## Network

Best option: put the server and both players on Tailscale or WireGuard, then set
the compiled server address in `client.c` to that VPN address and rebuild. No
router port forward is needed.

If playing over the public internet instead, forward TCP port `7777` to the
server machine and set the router's local IP to a DHCP reservation. Enable the
forward only while playing if possible. The game traffic itself is not
encrypted, so a VPN is strongly recommended.

## Run the client

Give your friend the same authenticated client build and the same token privately.
Start the game:

```bash
./online/build/shogun
```

In-game:

1. Choose `TWO PLAYERS`.
2. Choose `ONLINE GAME`.
3. Enter the access token.
4. Choose `NEXT`, then enter your player name.
5. When the opponent appears, choose `ACCEPT`.

The server address is not typed in the menu. It is stored in a lightly
obfuscated form in `client.c`, decoded at startup and shown on the setup screen.
Only the token is saved to `~/.shogun_online.cfg` so it is prefilled next time.
On Unix that file is created with mode `600`; it is not encrypted. The
`SHOGUN_TOKEN` environment variable is used as an initial default if no saved
token exists.

## Security notes

- The token is never sent over the network. The server sends a nonce and the
  client replies with a hash of nonce + token.
- The default server address in `client.c` is only lightly obfuscated. This
  hides casual inspection; it is not encryption or secrecy.
- Game moves and messages are plaintext after authentication. Use Tailscale or
  WireGuard if you need real encryption.

## Troubleshooting

- `ACCESS TOKEN IS NOT SET`: enter the token in the online setup menu.
- `BAD PASSWORD`: the client token does not match the server token.
- `COULD NOT CONNECT TO SERVER`: check the compiled address, firewall, VPN, and
  port forward.
- No opponent appears: make sure both players use the same server and token,
  and that the server service is active.

See `notes.md` for implementation and design notes.
