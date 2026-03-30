document.getElementById("midiFileInput").addEventListener("change", async (e) => {
    const file = e.target.files[0];
    if (!file) return;

    const bytes = await readMidiFile(file);
    const notes = parseNidmid(bytes);

    console.log(notes);
    displayNotes(notes);
});

