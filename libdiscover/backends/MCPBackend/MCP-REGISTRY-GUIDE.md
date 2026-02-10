# MCP Registry Guide

How to create and host an MCP server registry for KDE Discover.

## Overview

The MCP catalogue in KDE Discover fetches server listings from **registries** -- JSON files hosted at a URL. Users add your registry URL to their `~/.config/mcp/sources.list`, and Discover pulls your server catalogue automatically.

The flow looks like this:

```
Your GitHub repo                     User's machine
  registry.json         -->        KDE Discover
  (hosted via raw URL)              fetches & caches
                                    shows servers in UI
                                    user clicks Install
                                    pkexec installs into /usr/share/mcp/installed/{id}/
                                    manifest.json written to installed/{id}/
                                    mcp.json index updated
```

## Registry File Format

A registry is a single JSON file with this structure:

```json
{
  "version": "1.0",
  "updated": "2025-02-03T00:00:00Z",
  "servers": [
    { ... },
    { ... }
  ]
}
```

| Field       | Type   | Description                              |
|-------------|--------|------------------------------------------|
| `version`   | string | Registry format version (use `"1.0"`)    |
| `updated`   | string | ISO 8601 timestamp of last update        |
| `servers`   | array  | Array of server entry objects (see below) |

## Server Entry Schema

Each object in the `servers` array describes one MCP server.

### Required Fields

```json
{
  "id": "com.yourorg.mcp.servername",
  "name": "My MCP Server",
  "summary": "One-line description shown in the listing",
  "version": "1.0.0",
  "type": "stdio",
  "transport": { ... },
  "source": { ... }
}
```

| Field       | Type   | Description                                              |
|-------------|--------|----------------------------------------------------------|
| `id`        | string | Unique identifier. Use reverse-domain notation.          |
| `name`      | string | Display name in the catalogue.                           |
| `summary`   | string | Short description shown in listings.                     |
| `version`   | string | Semantic version of the server.                          |
| `type`      | string | Transport type: `"stdio"`, `"sse"`, or `"websocket"`.    |
| `transport` | object | Transport-specific connection configuration (see below). |
| `source`    | object | How to install the server (see below).                   |

### Optional Fields

| Field                | Type   | Description                                                     |
|----------------------|--------|-----------------------------------------------------------------|
| `description`        | string | Long description. Supports `\n` for line breaks.                |
| `author`             | string | Author name.                                                    |
| `homepage`           | string | URL to the project homepage.                                    |
| `bugUrl`             | string | URL to the issue tracker.                                       |
| `donationUrl`        | string | URL for donations.                                              |
| `icon`               | string | Freedesktop icon name (e.g. `"folder"`, `"network-server"`).   |
| `categories`         | array  | Category strings for filtering (see Categories below).          |
| `capabilities`       | array  | What the server can do (freeform strings for display).          |
| `permissions`        | array  | Permissions the server requires (freeform strings for display). |
| `tools`              | array  | Tools provided (strings or `{"name": ..., "description": ...}`).|
| `requiredParameters` | array  | Parameters the user must fill in before install (see below).    |
| `optionalParameters` | array  | Optional parameters with defaults, editable post-install.       |
| `license`            | object | `{"name": "MIT", "url": "https://..."}`.                       |
| `releaseDate`        | string | ISO 8601 date string (`"2025-01-15"`).                          |
| `size`               | number | Approximate size in bytes (for display).                        |
| `screenshots`        | array  | Screenshot URLs or `{"thumbnail": ..., "url": ...}` objects.   |
| `changelog`          | string | Changelog text.                                                 |

## Transport Configuration

The `transport` object depends on the server `type`.

### stdio (Local Process)

The server runs as a local process. Discover launches it via the specified command.

```json
{
  "type": "stdio",
  "transport": {
    "command": "mcp-server-filesystem",
    "args": ["--root", "/home"]
  }
}
```

| Field     | Type  | Description                          |
|-----------|-------|--------------------------------------|
| `command` | string | Executable name or path.            |
| `args`    | array  | Command-line arguments (optional).  |

### sse (Server-Sent Events)

The server is remote. No local executable is installed -- Discover just records the URL.

```json
{
  "type": "sse",
  "transport": {
    "url": "https://api.example.com/mcp/sse"
  }
}
```

| Field | Type   | Description               |
|-------|--------|---------------------------|
| `url` | string | SSE endpoint URL.         |

### websocket

```json
{
  "type": "websocket",
  "transport": {
    "wsUrl": "wss://api.example.com/mcp/ws"
  }
}
```

## Source Configuration

The `source` object tells Discover how to install a server.

```json
"source": {
  "type": "npm",
  "package": "@yourorg/mcp-server"
}
```

| Source Type  | Fields                          | What Discover Does                                          |
|--------------|---------------------------------|-------------------------------------------------------------|
| `npm`        | `package` (npm package name)    | `npm install --prefix /usr/share/mcp/installed/{id} <pkg>`  |
| `pip`        | `package` (PyPI package name)   | `pip install --prefix /usr/share/mcp/installed/{id} <pkg>`  |
| `binary`     | `url` (download URL)            | Downloads binary to `installed/{id}/`                       |
| `container`  | `package` (image name)          | `pkexec podman pull <image>`                                |
| `git`        | `url` (git repo URL)            | Clones repo to `installed/{id}/`                            |

For SSE servers that have no local installation, you still need a `source` block. Use:

```json
"source": {
  "type": "binary",
  "url": ""
}
```

## Configuration Parameters

Servers can declare configuration parameters in two separate lists:

- **`requiredParameters`** — must be filled before installation. Discover shows a configuration dialog if any are empty.
- **`optionalParameters`** — pre-filled with defaults. Shown in an "Advanced" section or editable post-install.

The placement in the list determines whether it's required or optional — no `required` flag needed.

### Required Parameters

```json
"requiredParameters": [
  {
    "key": "api_key",
    "label": "API Key",
    "description": "Your API key from https://example.com/settings",
    "sensitive": true
  }
]
```

### Optional Parameters

```json
"optionalParameters": [
  {
    "key": "timeout",
    "label": "Timeout (seconds)",
    "description": "Request timeout in seconds",
    "default": "30"
  },
  {
    "key": "endpoint",
    "label": "Endpoint URL",
    "description": "API endpoint (defaults to production)",
    "default": "https://api.example.com/v1"
  }
]
```

### Parameter Fields

| Field         | Type    | Description                                                |
|---------------|---------|------------------------------------------------------------|
| `key`         | string  | Internal identifier. Used as the key in config storage.    |
| `label`       | string  | Label shown in the configuration dialog.                   |
| `description` | string  | Help text shown below the input field.                     |
| `default`     | string  | Default value. Pre-filled in the UI (mainly for optional). |
| `sensitive`   | boolean | If `true`, field is shown as a password input.             |

User-provided values are stored in `~/.config/mcp/config.json` (never in the system manifest), keeping sensitive data separate from the system catalogue. Optional parameter defaults are applied automatically if the user doesn't override them.

## Categories

Use these category strings so your servers appear under the right filter in Discover:

| Category              | Description              |
|-----------------------|--------------------------|
| `mcp`                 | Generic MCP server (always include this) |
| `mcp-database`        | Database servers         |
| `mcp-filesystem`      | Filesystem operations    |
| `mcp-web`             | Web-related servers      |
| `mcp-search`          | Search capabilities      |
| `mcp-development`     | Development tools        |
| `mcp-ai`              | AI-related services      |
| `mcp-shell`           | Shell execution          |
| `mcp-communication`   | Communication tools      |
| `mcp-media`           | Media handling           |
| `mcp-productivity`    | Productivity tools       |

Always include `"mcp"` in addition to any specific category. A server can belong to multiple categories.

## Hosting Your Registry

### Option 1: GitHub Raw URL (Simplest)

1. Create a `registry.json` in your repo.
2. Use the raw GitHub URL as your registry source:

```
https://raw.githubusercontent.com/yourorg/mcp-registry/main/registry.json
```

Users add this URL to their sources:

```bash
echo "https://raw.githubusercontent.com/yourorg/mcp-registry/main/registry.json" \
  >> ~/.config/mcp/sources.list
```

### Option 2: GitHub Pages

If you want a cleaner URL, serve `registry.json` via GitHub Pages:

```
https://yourorg.github.io/mcp-registry/registry.json
```

### Option 3: Your Own Server

Host `registry.json` on any web server. Discover sends a standard HTTP GET with the User-Agent `KDE Discover MCP Backend/1.0`. Ensure HTTPS is used and redirects are followed.

## Minimal Working Example

Here is a complete minimal registry with one stdio server and one SSE server:

```json
{
  "version": "1.0",
  "updated": "2025-02-09T00:00:00Z",
  "servers": [
    {
      "id": "com.yourorg.mcp.hello",
      "name": "Hello World MCP",
      "summary": "A simple MCP server example",
      "version": "1.0.0",
      "type": "stdio",
      "transport": {
        "command": "hello-mcp",
        "args": []
      },
      "source": {
        "type": "npm",
        "package": "@yourorg/hello-mcp"
      },
      "categories": ["mcp", "mcp-development"]
    },
    {
      "id": "com.yourorg.mcp.cloud-api",
      "name": "Cloud API",
      "summary": "Remote SSE server for cloud API access",
      "version": "1.0.0",
      "type": "sse",
      "transport": {
        "url": "https://api.yourorg.com/mcp/sse"
      },
      "source": {
        "type": "binary",
        "url": ""
      },
      "categories": ["mcp", "mcp-web"],
      "requiredParameters": [
        {
          "key": "api_key",
          "label": "API Key",
          "description": "Get your key at https://yourorg.com/settings",
          "sensitive": true
        }
      ]
    }
  ]
}
```

## How Discover Processes Your Registry

1. **Fetch**: On startup (and on manual refresh), Discover fetches each URL from `sources.list`.
2. **Cache**: The response is cached locally at `~/.cache/discover/mcp-registries/`.
3. **Parse**: Each server entry in the `servers` array becomes a resource in the MCP Servers catalogue.
4. **Merge**: If a server from the registry is already installed (matched by `id`), Discover compares versions and marks it as upgradeable if the registry version is newer.
5. **Display**: Servers appear under Development > MCP Servers, filterable by category and searchable by name/summary/id.

## What Happens on Install

When a user clicks Install on your server:

1. If `requiredParameters` exist and any are unconfigured, a configuration dialog is shown.
2. The user authenticates via polkit (system password prompt).
3. The appropriate install command runs with elevated privileges into an isolated directory:
   - npm: `pkexec sh -c "mkdir -p /usr/share/mcp/installed/{id} && npm install --prefix /usr/share/mcp/installed/{id} <package>"`
   - pip: `pkexec sh -c "mkdir -p /usr/share/mcp/installed/{id} && pip install --prefix /usr/share/mcp/installed/{id} <package>"`
   - container: `pkexec podman pull <image>` (stored in podman's own storage)
4. A per-server manifest is written to `/usr/share/mcp/installed/{id}/manifest.json`.
   The manifest includes an `installedCommand` field in the `transport` section that
   resolves to the full path of the binary (e.g. `/usr/share/mcp/installed/{id}/node_modules/.bin/mcp-server-filesystem`).
5. The system index at `/usr/share/mcp/mcp.json` is also updated (kept in sync).
6. User-provided configuration values are saved to `~/.config/mcp/config.json`.

### Directory Layout After Install

```
/usr/share/mcp/
├── mcp.json                                        (system index)
└── installed/
    ├── org.anthropic.mcp.filesystem/               (npm server)
    │   ├── manifest.json                           (server metadata)
    │   ├── node_modules/                           (npm package contents)
    │   │   ├── .bin/
    │   │   │   └── mcp-server-filesystem           (binary symlink)
    │   │   └── @anthropic/
    │   │       └── mcp-server-filesystem/
    │   └── package-lock.json
    ├── io.postgres.mcp/                            (pip server)
    │   ├── manifest.json
    │   ├── bin/
    │   │   └── mcp-postgres                        (script)
    │   └── lib/
    │       └── python3.x/
    │           └── site-packages/
    └── com.example.remote-api/                     (SSE server — no local files)
        └── manifest.json
```

### Uninstall

Removal is a simple `rm -rf /usr/share/mcp/installed/{id}/`. No package-manager-specific uninstall is needed since all files are contained within the server's own directory.

## Tips

- **Keep IDs stable.** The `id` field is how Discover tracks a server across registry updates. Changing it creates a "new" server.
- **Use semantic versioning.** Discover compares `installedVersion` against your registry's `version` to detect upgrades.
- **Test your JSON.** A malformed registry file is silently skipped. Validate your JSON before publishing.
- **Update the `updated` timestamp** when you publish changes, so users know the registry is maintained.
- **Provide a `bugUrl`.** It shows a "Report Bug" link on the server's detail page in Discover.
