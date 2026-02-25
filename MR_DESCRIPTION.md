# Add MCPBackend: discovery and management of MCP servers

## Summary

This merge request adds a new backend plugin — **MCPBackend** — that lets users
browse, install, configure, and remove
[Model Context Protocol (MCP)](https://modelcontextprotocol.io/) servers through
KDE Discover.

MCP is an open standard (originally published by Anthropic, now community-driven)
that allows LLM-based tools (editors, assistants, etc.) to connect to local or
remote "servers" that expose tools, resources and prompts. Examples include
filesystem access, web-search, database queries, and shell execution. Users
increasingly install these servers by hand via `npm`, `pip` or direct download;
this backend gives them a proper package-manager-style workflow inside Discover.

---

## What this adds

### New plugin: `mcp-backend`

| File | Purpose |
|------|---------|
| `MCPBackend.{h,cpp}` | `AbstractResourcesBackend` implementation; loads registries, manages installed servers |
| `MCPResource.{h,cpp}` | `AbstractResource` per server; transport type (stdio/SSE/WebSocket), source type (npm/pip/binary/container/git), required/optional parameters |
| `MCPTransaction.{h,cpp}` | `Transaction` that drives `npm install`, `pip install`, binary download, or container pull/remove via `QProcess`; uses `pkexec` for privileged writes to `/usr/share/mcp/` |
| `mcp-backend-categories.xml` | Adds an **MCP Servers** sub-category under **Development** |

### Storage layout managed by the backend

```
/usr/share/mcp/
  installed/{id}/manifest.json   ← per-server metadata (source of truth)
  mcp.json                       ← flat index kept in sync (for compatibility)
~/.config/mcp/
  sources.list                   ← registry URLs (one per line, '#' comments)
  config.json                    ← user-specific values (API keys, tokens, etc.)
```

### UI additions

* `MCPConfigDialog.qml` — overlay sheet that collects **required** and
  **optional** parameters before or after installation (e.g. API tokens,
  base URLs).  Sensitive fields are rendered as password inputs.
* Integration points in `DiscoverWindow.qml` and `ApplicationPage.qml` that
  open the config dialog when a transaction emits `configRequest`.

### Category integration

`CategoriesReader` already picks up any `{backend-name}-categories.xml` found
under `libdiscover/categories/`; the new XML nests **MCP Servers** inside the
existing **Development** top-level category.

---

## How to test

1. Build with `-DBUILD_MCPBackend=ON` (the option defaults to ON but has no
   mandatory external dependencies).
2. Add a registry URL to `~/.config/mcp/sources.list` — the file format is one
   URL per line, comment lines start with `#`.  Any JSON file that matches the
   schema in `examples/registry.json` works.
3. Open Discover → **Development** → **MCP Servers**.  Servers from the
   registry should appear.
4. Install a server; Discover will prompt for required parameters if the server
   declares any.
5. Check that `/usr/share/mcp/installed/{id}/manifest.json` was written (pkexec
   will ask for the user's password).
6. Remove the server; verify the manifest directory and the `mcp.json` entry are
   removed.

---

## Known limitations / follow-up work

* **Privileged writes use `pkexec cp` synchronously** — this blocks the GUI
  thread briefly.  The correct long-term solution is to move privileged I/O to a
  KAuth helper action.  Filed as a follow-up; the current approach is safe but
  not ideal.
* No unit tests yet.  The transaction logic is tightly coupled to `QProcess`;
  adding an abstraction layer to allow mocking is the first step.
* The backend has no external library dependencies (`Qt::Network` only), so it
  is always built.  If the project wants it off-by-default until it reaches
  maturity, change `"ON"` to `OFF` in `backends/CMakeLists.txt`.

---

## Checklist

- [x] SPDX headers on all new files
- [x] KDE logging category (`org.kde.plasma.libdiscover.backend.mcp`)
- [x] `ecm_qt_declare_logging_category` used correctly
- [x] `DISCOVER_BACKEND_PLUGIN` macro used
- [x] All user-visible strings wrapped in `i18n()`
- [x] No hardcoded registry URLs shipped; sources are user/distro-configured
- [ ] KAuth helper for privileged writes (follow-up)
- [ ] Unit tests (follow-up)
