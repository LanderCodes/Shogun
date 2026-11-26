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

On Windows with MSYS2 UCRT64:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-SDL2 git
```

## Build

From the main game directory:

```bash
cmake -S online -B online/build
cmake --build online/build -j2
ctest --test-dir online/build --output-on-failure
```

This creates `online/build/shogun` and `online/build/shogun-server`.

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

Best option: put the server and both players on Tailscale or WireGuard, then use
the server's VPN address in the game menu. No router port forward is needed.

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
4. Enter the server as `host:port`, or `AUTO` for LAN discovery.
5. Choose `NEXT`, then enter your player name.
6. When the opponent appears, choose `ACCEPT`.

The token and server values are saved to `~/.shogun_online.cfg` so they are
prefilled next time. On Unix that file is created with mode `600`; it is not
encrypted. Environment variables `SHOGUN_TOKEN` and `SHOGUN_SERVER` are used
only as initial defaults if no saved values exist.

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
- `COULD NOT CONNECT TO SERVER`: check the address, firewall, VPN, and port
  forward.
- No opponent appears: make sure both players use the same server and token,
  and that the server service is active.

See `notes.md` for implementation and design notes.
