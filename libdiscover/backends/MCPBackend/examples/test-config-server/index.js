#!/usr/bin/env node

/**
 * Test MCP Server with Configuration Properties
 * 
 * This server demonstrates how configuration properties (API key, endpoint URL, timeout)
 * configured through KDE Discover are used by an MCP server.
 * 
 * The server reads its configuration from the manifest file created by Discover:
 * - /usr/share/mcp/installed/com.example.test-config.json (system-wide)
 * - ~/.local/share/mcp/installed/com.example.test-config.json (user-specific)
 */

const { Server } = require("@modelcontextprotocol/sdk/server/index.js");
const { StdioServerTransport } = require("@modelcontextprotocol/sdk/server/stdio.js");
const {
  CallToolRequestSchema,
  ListToolsRequestSchema,
} = require("@modelcontextprotocol/sdk/types.js");
const fs = require("fs");
const path = require("path");
const os = require("os");

// Configuration values loaded from manifest
let config = {
  api_key: "",
  endpoint_url: "",
  timeout: "30"
};

/**
 * Load configuration from manifest file
 */
function loadConfig() {
  const possiblePaths = [
    "/usr/share/mcp/installed/com.example.test-config.json",
    path.join(os.homedir(), ".local/share/mcp/installed/com.example.test-config.json")
  ];

  for (const configPath of possiblePaths) {
    if (fs.existsSync(configPath)) {
      try {
        const manifest = JSON.parse(fs.readFileSync(configPath, "utf8"));
        if (manifest.config) {
          config.api_key = manifest.config.api_key || "";
          config.endpoint_url = manifest.config.endpoint_url || "";
          config.timeout = manifest.config.timeout || "30";
          console.error(`[TestConfigServer] Loaded config from ${configPath}`);
          console.error(`[TestConfigServer] API Key: ${config.api_key ? "***" + config.api_key.slice(-4) : "not set"}`);
          console.error(`[TestConfigServer] Endpoint URL: ${config.endpoint_url || "not set"}`);
          console.error(`[TestConfigServer] Timeout: ${config.timeout}s`);
          return true;
        }
      } catch (error) {
        console.error(`[TestConfigServer] Error loading config from ${configPath}:`, error.message);
      }
    }
  }
  
  console.error("[TestConfigServer] Warning: No config file found. Using defaults.");
  return false;
}

/**
 * Test tool that uses the configuration
 */
async function testTool(args) {
  const { message } = args;
  
  // Validate configuration
  if (!config.api_key) {
    return {
      content: [
        {
          type: "text",
          text: "Error: API key is not configured. Please configure it through KDE Discover."
        }
      ]
    };
  }

  if (!config.endpoint_url) {
    return {
      content: [
        {
          type: "text",
          text: "Error: Endpoint URL is not configured. Please configure it through KDE Discover."
        }
      ]
    };
  }

  // Simulate using the configuration
  const timeoutMs = parseInt(config.timeout) * 1000;
  const testMessage = message || "Hello from test tool!";
  
  return {
    content: [
      {
        type: "text",
        text: `Test Tool Execution Results:
        
Configuration Status:
- API Key: ${config.api_key ? "***" + config.api_key.slice(-4) : "NOT SET"}
- Endpoint URL: ${config.endpoint_url}
- Timeout: ${config.timeout} seconds

Test Message: "${testMessage}"

Simulated API Call:
- Would connect to: ${config.endpoint_url}
- Would use API key: ${config.api_key ? "***" + config.api_key.slice(-4) : "NOT SET"}
- Would timeout after: ${config.timeout}s

✅ Configuration is working correctly!
The values were successfully loaded from the manifest file created by KDE Discover.
`
      }
    ]
  };
}

/**
 * Get configuration status
 */
async function getConfigStatus() {
  const hasApiKey = config.api_key && config.api_key.length > 0;
  const hasEndpoint = config.endpoint_url && config.endpoint_url.length > 0;
  
  return {
    content: [
      {
        type: "text",
        text: `Configuration Status:
        
Required Properties:
- API Key: ${hasApiKey ? "✅ Configured (" + "***" + config.api_key.slice(-4) + ")" : "❌ Not configured"}
- Endpoint URL: ${hasEndpoint ? "✅ Configured (" + config.endpoint_url + ")" : "❌ Not configured"}

Optional Properties:
- Timeout: ${config.timeout || "30"} seconds

${hasApiKey && hasEndpoint ? "✅ All required properties are configured!" : "⚠️ Some required properties are missing."}
`
      }
    ]
  };
}

/**
 * Main server setup
 */
async function main() {
  // Load configuration from manifest
  loadConfig();

  const server = new Server(
    {
      name: "test-config-server",
      version: "1.0.0",
    },
    {
      capabilities: {
        tools: {},
      },
    }
  );

  // List available tools
  server.setRequestHandler(ListToolsRequestSchema, async () => ({
    tools: [
      {
        name: "test_tool",
        description: "A test tool that uses configuration properties (API key, endpoint URL, timeout)",
        inputSchema: {
          type: "object",
          properties: {
            message: {
              type: "string",
              description: "Optional test message"
            }
          }
        }
      },
      {
        name: "get_config_status",
        description: "Get the current configuration status",
        inputSchema: {
          type: "object",
          properties: {}
        }
      }
    ]
  }));

  // Handle tool calls
  server.setRequestHandler(CallToolRequestSchema, async (request) => {
    const { name, arguments: args } = request.params;

    switch (name) {
      case "test_tool":
        return await testTool(args || {});
      
      case "get_config_status":
        return await getConfigStatus();
      
      default:
        throw new Error(`Unknown tool: ${name}`);
    }
  });

  // Connect via stdio
  const transport = new StdioServerTransport();
  await server.connect(transport);

  console.error("[TestConfigServer] Test MCP Server started");
  console.error("[TestConfigServer] Configuration loaded successfully");
}

main().catch((error) => {
  console.error("[TestConfigServer] Fatal error:", error);
  process.exit(1);
});
