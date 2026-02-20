# tiny-csgo-server
Tiny CS2 fake server for logging on to Steam game servers, establishing a GC connection and being displayed in the in-game server browser. It is **not** a real game server.

## Dependencies
 - [hl2sdk-csgo](https://github.com/alliedmodders/hl2sdk/tree/csgo)
 - [Asio](https://github.com/chriskohlhoff/asio)
 - CMake

## Compile and Run
### Windows
1. Configure path of hl2sdk-csgo and Asio in `build.bat`.
2. Run `build.bat`.
3. Move files from `bin/windows` next to `tiny-csgo-server.exe`.
4. Run `tiny-csgo-server.exe`.

### Linux
1. Configure path of hl2sdk-csgo and Asio in `build.sh`.
2. Run `build.sh`.
3. Move files from `bin/linux` next to `tiny-csgo-server`.
4. Set the executable directory to `LD_LIBRARY_PATH`.
5. Run `tiny-csgo-server`.

## CLI options
- `-port` Server listening port.
- `-version` CS2 version string (`PatchVersion` from `steam.inf`).
- `-gslt` Game server logon token.
- `-rdip` Redirect IP address (for example `127.0.0.1:27015`).
- `-vac` Enable VAC.
- `-mirror` Mirror A2S info from `-rdip`.
- `-hostname` Browser hostname.
- `-map` Browser map.
- `-region` Region code (`0`..`7`).
- `-tags` Browser tags.
- `-description` Browser game description.
- `-players` Visible player count.
- `-maxplayers` Visible max player count.
- `-bots` Visible bot count.

## Config launch mode (multiple servers + tokens.txt)
Use `-config` to start many fake servers from one JSON file and automatically apply GSLT tokens from `tokens.txt`.

### Example `config.json`
```json
{
  "start_port": 27016,
  "version": "2.0.0.0",
  "servers": [
    {
      "count": 1,
      "server_address": "45.95.31.112:27215",
      "players": 63,
      "max_players": 64,
      "bots": 0,
      "hostname": "➡️ PUBLIC | ALkoGoLiki | FPS+",
      "map": "de_dust2",
      "region": "3",
      "secure": true,
      "tags": "empty,free,insecure",
      "description": "Counter-Strike 2"
    }
  ]
}
```

### Example `tokens.txt`
One token per line:
```txt
GSLT_TOKEN_1
GSLT_TOKEN_2
```

### Run with config
```bash
./tiny-csgo-server -config config.json -tokens tokens.txt
```

Notes:
- Ports are assigned sequentially from `start_port`.
- `count` creates several instances from one server template.
- If tokens are fewer than instances, extra instances run without `-gslt`.
