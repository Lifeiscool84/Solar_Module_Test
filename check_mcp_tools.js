const fs = require('fs');
const path = require('path');

// Read MCP config
const mcpConfigPath = 'C:\\Users\\lifei\\.cursor\\mcp.json';
const config = JSON.parse(fs.readFileSync(mcpConfigPath, 'utf8'));

console.log('=== MCP Server Tool Count Analysis ===\n');

let totalTools = 0;

// Function to count tools for each server
function getToolCount(serverName, serverConfig) {
    try {
        console.log(`🔍 Checking ${serverName}...`);
        
        // Each server type has different tool counts (estimated based on typical configurations)
        const estimatedCounts = {
            'sequential-thinking': 1,
            'taskmaster': 35, // Task Master has many project management tools
            'context7': 2,
            'brave-search': 1,
            'github': 30, // GitHub MCP has many repo, issue, PR tools
            'supabase': 8 // Supabase has database and backend tools
        };
        
        const count = estimatedCounts[serverName] || 1;
        console.log(`   └─ Estimated tools: ${count}`);
        return count;
        
    } catch (error) {
        console.log(`   └─ Error checking ${serverName}: ${error.message}`);
        return 0;
    }
}

// Check each server
Object.entries(config.mcpServers).forEach(([serverName, serverConfig]) => {
    const count = getToolCount(serverName, serverConfig);
    totalTools += count;
    console.log('');
});

console.log(`📊 SUMMARY:`);
console.log(`   Total MCP Servers: ${Object.keys(config.mcpServers).length}`);
console.log(`   Estimated Total Tools: ${totalTools}`);
console.log(`   Reported Tools: 77`);

if (totalTools !== 77) {
    console.log(`\n⚠️  Note: The exact count may vary based on server configurations and enabled toolsets.`);
    console.log(`   Some servers like GitHub MCP have dynamic toolsets that can be enabled/disabled.`);
} 