#!/usr/bin/env python3
"""
JARVIS SuperMCP Integration Example

This script demonstrates how Project JARVIS's SuperMCP orchestration layer
can discover and use MCP servers installed via KDE Discover.

The servers are installed to:
- /usr/share/mcp/installed/ (system-wide)
- ~/.local/share/mcp/installed/ (user-specific)
"""

import json
import os
from pathlib import Path
from typing import Dict, List, Optional

# Standard XDG data directories
XDG_DATA_HOME = os.environ.get('XDG_DATA_HOME', os.path.expanduser('~/.local/share'))
XDG_DATA_DIRS = os.environ.get('XDG_DATA_DIRS', '/usr/local/share:/usr/share').split(':')


def get_mcp_directories() -> List[Path]:
    """Get all directories where MCP servers might be installed."""
    dirs = []

    # User-specific directory (highest priority)
    user_dir = Path(XDG_DATA_HOME) / 'mcp' / 'installed'
    dirs.append(user_dir)

    # System directories
    for data_dir in XDG_DATA_DIRS:
        system_dir = Path(data_dir) / 'mcp' / 'installed'
        dirs.append(system_dir)

    return dirs


def discover_installed_servers() -> Dict[str, dict]:
    """
    Discover all installed MCP servers from the system registry.

    Returns a dictionary mapping server IDs to their configuration.
    """
    servers = {}

    for mcp_dir in get_mcp_directories():
        if not mcp_dir.exists():
            continue

        for manifest_path in mcp_dir.glob('*.json'):
            try:
                with open(manifest_path, 'r') as f:
                    manifest = json.load(f)

                server_id = manifest.get('id')
                if server_id and server_id not in servers:
                    servers[server_id] = manifest
                    print(f"Discovered: {manifest.get('name')} ({server_id})")

            except (json.JSONDecodeError, IOError) as e:
                print(f"Warning: Failed to load {manifest_path}: {e}")

    return servers


def get_server_config(server: dict) -> dict:
    """
    Convert a server manifest to a configuration suitable for MCP clients.

    This format is compatible with Claude Code and other MCP clients.
    """
    transport = server.get('transport', {})
    server_type = server.get('type', 'stdio')

    if server_type == 'stdio':
        return {
            'command': transport.get('command'),
            'args': transport.get('args', []),
        }
    elif server_type == 'sse':
        return {
            'url': transport.get('url'),
            'auth': transport.get('auth'),
        }
    elif server_type == 'websocket':
        return {
            'url': transport.get('wsUrl'),
        }

    return {}


def generate_claude_config(servers: Dict[str, dict]) -> dict:
    """
    Generate a Claude Code compatible configuration from discovered servers.
    """
    mcp_servers = {}

    for server_id, server in servers.items():
        # Use a simplified name for the config key
        config_name = server_id.split('.')[-1]  # e.g., "filesystem" from "org.anthropic.mcp.filesystem"

        config = get_server_config(server)
        if config:
            mcp_servers[config_name] = config

    return {'mcpServers': mcp_servers}


def find_servers_by_capability(servers: Dict[str, dict], capability: str) -> List[dict]:
    """
    Find all servers that provide a specific capability.

    This is useful for SuperMCP's dynamic routing feature.
    """
    matching = []

    for server_id, server in servers.items():
        capabilities = server.get('capabilities', [])
        if capability in capabilities:
            matching.append({
                'id': server_id,
                'name': server.get('name'),
                'tools': server.get('tools', []),
                'config': get_server_config(server),
            })

    return matching


def find_servers_by_tool(servers: Dict[str, dict], tool_name: str) -> List[dict]:
    """
    Find all servers that provide a specific tool.

    This enables SuperMCP to route tool calls to the appropriate server.
    """
    matching = []

    for server_id, server in servers.items():
        tools = server.get('tools', [])
        tool_names = []

        for tool in tools:
            if isinstance(tool, str):
                tool_names.append(tool)
            elif isinstance(tool, dict):
                tool_names.append(tool.get('name', ''))

        if tool_name in tool_names:
            matching.append({
                'id': server_id,
                'name': server.get('name'),
                'config': get_server_config(server),
            })

    return matching


class SuperMCPRouter:
    """
    Example SuperMCP routing implementation.

    This class demonstrates how JARVIS's orchestration layer could use
    the system MCP registry to dynamically route requests to appropriate servers.
    """

    def __init__(self):
        self.servers = discover_installed_servers()
        self._build_tool_index()

    def _build_tool_index(self):
        """Build an index of tools to servers for fast lookup."""
        self.tool_index: Dict[str, List[str]] = {}

        for server_id, server in self.servers.items():
            for tool in server.get('tools', []):
                tool_name = tool if isinstance(tool, str) else tool.get('name', '')
                if tool_name:
                    if tool_name not in self.tool_index:
                        self.tool_index[tool_name] = []
                    self.tool_index[tool_name].append(server_id)

    def route_tool_call(self, tool_name: str) -> Optional[dict]:
        """
        Route a tool call to the appropriate MCP server.

        Returns the server configuration for the tool, or None if not found.
        """
        server_ids = self.tool_index.get(tool_name, [])

        if not server_ids:
            return None

        # Use the first available server (could implement more sophisticated routing)
        server_id = server_ids[0]
        server = self.servers[server_id]

        return {
            'server_id': server_id,
            'server_name': server.get('name'),
            'config': get_server_config(server),
        }

    def get_available_tools(self) -> List[str]:
        """Get a list of all available tools across all servers."""
        return list(self.tool_index.keys())

    def get_server_info(self, server_id: str) -> Optional[dict]:
        """Get detailed information about a specific server."""
        return self.servers.get(server_id)


def main():
    """Main demonstration function."""
    print("=" * 60)
    print("JARVIS SuperMCP - MCP Server Discovery")
    print("=" * 60)
    print()

    # Discover installed servers
    print("Scanning for installed MCP servers...")
    servers = discover_installed_servers()

    print(f"\nFound {len(servers)} installed MCP server(s)")
    print()

    if not servers:
        print("No MCP servers found. Install some via KDE Discover!")
        return

    # Show server details
    print("Installed Servers:")
    print("-" * 40)
    for server_id, server in servers.items():
        print(f"  {server.get('name')}")
        print(f"    ID: {server_id}")
        print(f"    Version: {server.get('installedVersion', server.get('version'))}")
        print(f"    Type: {server.get('type', 'stdio')}")
        print(f"    Tools: {', '.join(t if isinstance(t, str) else t.get('name', '') for t in server.get('tools', []))}")
        print()

    # Generate Claude Code config
    print("Claude Code Configuration:")
    print("-" * 40)
    claude_config = generate_claude_config(servers)
    print(json.dumps(claude_config, indent=2))
    print()

    # Demonstrate SuperMCP routing
    print("SuperMCP Router Demo:")
    print("-" * 40)
    router = SuperMCPRouter()

    print(f"Available tools: {', '.join(router.get_available_tools())}")
    print()

    # Example: Route a tool call
    test_tools = ['read_file', 'web_search', 'query', 'execute']
    for tool in test_tools:
        result = router.route_tool_call(tool)
        if result:
            print(f"Tool '{tool}' -> {result['server_name']} ({result['server_id']})")
        else:
            print(f"Tool '{tool}' -> No server available")


if __name__ == '__main__':
    main()
