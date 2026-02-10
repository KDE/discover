# Test Config MCP Server

A simple test MCP server that demonstrates configuration properties functionality in KDE Discover.

## Purpose

This server is designed to test that:
1. Configuration properties (API key, endpoint URL, timeout) are correctly saved by KDE Discover
2. The server can read configuration values from the manifest file
3. The configuration dialog in Discover works correctly

## Installation

### For Testing (Local Development)

1. Install dependencies:
```bash
cd libdiscover/backends/MCPBackend/examples/test-config-server
npm install
```

2. Make the script executable:
```bash
chmod +x index.js
```

3. Test locally:
```bash
# Create a test manifest file
mkdir -p ~/.local/share/mcp/installed/com.example.test-config/
cat > ~/.local/share/mcp/installed/com.example.test-config/manifest.json << EOF
{
  "id": "com.example.test-config",
  "name": "Test Config Server",
  "version": "1.0.0",
  "config": {
    "api_key": "test-api-key-12345",
    "endpoint_url": "https://api.example.com/v1",
    "timeout": "60"
  }
}
EOF

# Run the server
./index.js
```

### For KDE Discover Installation

When installed through KDE Discover:
1. The server will be installed via `npm install -g @test/mcp-server`
2. Configuration will be saved to the manifest file automatically
3. The server will read configuration from the manifest on startup

## Configuration Properties

The server expects these configuration properties:

- **api_key** (required, sensitive): API key for authentication
- **endpoint_url** (required): The API endpoint URL
- **timeout** (optional): Request timeout in seconds (default: 30)

## Testing with MCP Client

You can test the server with any MCP client:

```bash
# Example: Using Claude Desktop or another MCP client
# The server will be invoked as: test-config-server
```

## Tools

The server provides two tools:

1. **test_tool**: Demonstrates using the configuration values
   - Shows that API key, endpoint URL, and timeout are correctly loaded
   - Simulates an API call using the configured values

2. **get_config_status**: Shows current configuration status
   - Displays which properties are configured
   - Shows masked API key (last 4 characters)
   - Indicates if required properties are missing

## Manifest File Location

The server looks for configuration in these locations (in order):
1. `/usr/share/mcp/installed/com.example.test-config/manifest.json` (system-wide)
2. `~/.local/share/mcp/installed/com.example.test-config/manifest.json` (user-specific)

## Debugging

The server logs to stderr, so you can see debug output:
- Configuration loading status
- Masked API key (for verification)
- Endpoint URL and timeout values

## Notes

- The API key is masked in output (only last 4 characters shown)
- The server validates that required properties are set
- Configuration is loaded once at startup
- To reload configuration, restart the server


cmake .. -DBUILD_MCPBackend=ON
make -j$(nproc)
QT_PLUGIN_PATH=$(pwd)/bin QT_LOGGING_RULES="org.kde.plasma.libdiscover.backend.mcp.debug=true" ./bin/plasma-discover