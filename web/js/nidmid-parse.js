function parseNidmid(bytes) {
    const measures = [];
    let currentMeasure = null;
    let i = 0;

    while (i < bytes.length) {

        const byte = bytes[i];

        // 🎯 Début de mesure
        if (byte === 0xFF) {
            currentMeasure = [];
            measures.push(currentMeasure);
            i++;
            continue;
        }

        // 🎯 Événement
        const noteCount = byte;
        i++;

        const notes = [];

        for (let n = 0; n < noteCount; n++) {
            const pitch = bytes[i++];
            const velocity = bytes[i++];

            notes.push({ pitch, velocity });
        }

        if (currentMeasure) {
            currentMeasure.push(notes);
        }
    }

    return measures;
}