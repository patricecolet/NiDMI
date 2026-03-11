/**
 * mapping-engine.js
 */

// --- MATH FUNCTIONS ---
const MathLibrary = {
    '*': (val, factor) => val * parseFloat(factor),
    '/': (val, factor) => val / parseFloat(factor),
    '+': (val, offset) => val + parseFloat(offset),
    '-': (val, offset) => val - parseFloat(offset),
    'scale': (val, inMin, inMax, outMin, outMax) => {
        return (val - inMin) * (outMax - outMin) / (inMax - inMin) + parseFloat(outMin);
    },
    'invert': (val) => 1.0 - val
};

// --- SEND FUNCTIONS ---
const SendLibrary = {
    'ctl.out': (val, cc, ch) => {
        const finalVal = Math.min(Math.max(Math.round(val), 0), 127);
        console.log(`[MIDI SEND] CC:${cc} CH:${ch} Val:${finalVal}`);
        if (typeof sendMidiToServer === 'function') {
            sendMidiToServer('cc', parseInt(cc), parseInt(ch), finalVal);
        }
        return val; 
    },
    'note.out': (val, note, ch) => {
        const velocity = val > 0.5 ? 127 : 0;
        if (typeof sendMidiToServer === 'function') {
            sendMidiToServer('note', parseInt(note), parseInt(ch), velocity);
        }
        return val;
    }
};

/**
 * Runs the pipeline by checking both libraries
 */
function runPipeline(pipeline, initialValue) {
    if (!pipeline || pipeline.length === 0) return initialValue;
    
    return pipeline.reduce((currentValue, step) => {
        // We look for the function in Math first, then in Send
        const func = MathLibrary[step.name] || SendLibrary[step.name];
        
        if (func) {
            return func(currentValue, ...step.args);
        } else {
            console.warn(`Function ${step.name} not found in any library.`);
            return currentValue;
        }
    }, initialValue);
}

function processMapping(lbl, rawValue) {
    const cfg = pcfg[lbl];
    if (!cfg || !cfg.mapping) return;
    const pipeline = parseMappingScript(cfg.mapping);
    if (pipeline) runPipeline(pipeline, rawValue);
}