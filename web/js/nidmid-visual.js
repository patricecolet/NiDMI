function displayNotes(notes) {
    const container = document.getElementById("midiPreview");

    container.innerHTML = "";

    notes.forEach((n, i) => {
        const div = document.createElement("div");
        div.textContent = `Step ${i}: Note ${n.note} Vel ${n.velocity}`;
        container.appendChild(div);
    });
}

