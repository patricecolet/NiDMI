/**
 * mapping-parser.js
 */

function parseMappingScript(script) {
    if (!script || script.trim() === '') return null;

    const segments = script.split(':')
        .map(s => s.trim())
        .filter(s => s !== '' && !s.startsWith('r('));
    
    const pipeline = [];

    segments.forEach(seg => {
        const match = seg.match(/^([^(]+)\(([^)]*)\)$/);
        if (match) {
            const funcName = match[1].trim();
            const args = match[2].split(',').map(a => a.trim());
            
            // We just store the intent here, the Engine handles the lookup
            pipeline.push({ 
                name: funcName, 
                args: args 
            });
        }
    });

    return pipeline;
}