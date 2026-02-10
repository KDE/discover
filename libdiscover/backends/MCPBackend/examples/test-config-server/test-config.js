#!/usr/bin/env node

/**
 * Simple test script to verify configuration loading
 * 
 * This script can be run independently to test that the configuration
 * is being read correctly from the manifest file.
 */

const fs = require("fs");
const path = require("path");
const os = require("os");

function loadConfig() {
  const possiblePaths = [
    "/usr/share/mcp/installed/com.example.test-config/manifest.json",
    path.join(os.homedir(), ".local/share/mcp/installed/com.example.test-config/manifest.json")
  ];

  console.log("Testing configuration loading...\n");

  for (const configPath of possiblePaths) {
    console.log(`Checking: ${configPath}`);
    if (fs.existsSync(configPath)) {
      try {
        const manifest = JSON.parse(fs.readFileSync(configPath, "utf8"));
        console.log("✅ Manifest file found!\n");
        
        console.log("Manifest contents:");
        console.log(JSON.stringify(manifest, null, 2));
        console.log("\n");
        
        if (manifest.config) {
          const config = manifest.config;
          console.log("Configuration values:");
          console.log(`  API Key: ${config.api_key ? "***" + config.api_key.slice(-4) : "NOT SET"}`);
          console.log(`  Endpoint URL: ${config.endpoint_url || "NOT SET"}`);
          console.log(`  Timeout: ${config.timeout || "NOT SET"} seconds`);
          console.log("\n✅ Configuration loaded successfully!");
          return true;
        } else {
          console.log("⚠️  Manifest found but no 'config' object present");
          return false;
        }
      } catch (error) {
        console.error(`❌ Error reading manifest: ${error.message}`);
        return false;
      }
    } else {
      console.log("  (not found)");
    }
  }
  
  console.log("\n❌ No manifest file found in any location.");
  console.log("\nTo create a test manifest, run:");
  console.log(`mkdir -p ~/.local/share/mcp/installed/com.example.test-config/`);
  console.log(`cat > ~/.local/share/mcp/installed/com.example.test-config/manifest.json << 'EOF'`);
  console.log(`{`);
  console.log(`  "id": "com.example.test-config",`);
  console.log(`  "name": "Test Config Server",`);
  console.log(`  "version": "1.0.0",`);
  console.log(`  "config": {`);
  console.log(`    "api_key": "test-key-12345",`);
  console.log(`    "endpoint_url": "https://api.example.com/v1",`);
  console.log(`    "timeout": "60"`);
  console.log(`  }`);
  console.log(`}`);
  console.log(`EOF`);
  return false;
}

if (require.main === module) {
  loadConfig();
}

module.exports = { loadConfig };
