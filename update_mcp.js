const fs = require('fs');
const path = require('path');

// Path to the MCP configuration file
const mcpConfigPath = 'c:/Users/lifei/.cursor/mcp.json';

console.log(`Attempting to update MCP config at: ${mcpConfigPath}`);
console.log(`Current directory: ${process.cwd()}`);

try {
    // Check if the file exists
    if (!fs.existsSync(mcpConfigPath)) {
        console.error(`Error: MCP config file not found at ${mcpConfigPath}`);
        process.exit(1);
    }
    
    console.log('Reading existing configuration...');
    
    // Read the existing configuration
    const configContent = fs.readFileSync(mcpConfigPath, 'utf8');
    console.log(`Config content: ${configContent.substring(0, 100)}...`);
    
    const config = JSON.parse(configContent);
    
    console.log('Current servers:', Object.keys(config.mcpServers).join(', '));
    
    // Add the GitHub server configuration
    config.mcpServers.github = {
        args: [
            "C:\\Users\\lifei\\AppData\\Roaming\\npm\\node_modules\\@modelcontextprotocol\\server-github\\dist\\index.js"
        ],
        command: "C:\\Program Files\\nodejs\\node.exe",
        env: {
            GITHUB_PERSONAL_ACCESS_TOKEN: ""
        }
    };
    
    console.log('Updated servers:', Object.keys(config.mcpServers).join(', '));
    
    // Write the updated configuration back to the file
    const updatedConfig = JSON.stringify(config, null, 4);
    fs.writeFileSync(mcpConfigPath, updatedConfig);
    
    console.log('MCP configuration updated successfully!');
    console.log(`Updated config preview: ${updatedConfig.substring(0, 100)}...`);
} catch (error) {
    console.error('Error updating MCP configuration:', error);
} 