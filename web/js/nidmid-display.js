async function loadView() {
    const res = await fetch('/api/sequencer/view');
    const data = await res.json();

    renderSteps(data.steps);
}

function renderSteps(steps) {
    const container = document.getElementById("seqView");
    container.innerHTML = "";

    let currentMeasure = -1;

    steps.forEach((step, i) => {

        if (step.measure !== currentMeasure) {
            currentMeasure = step.measure;

            const title = document.createElement("div");
            title.innerHTML = `<b>Measure ${currentMeasure}</b>`;
            container.appendChild(title);
        }

        const div = document.createElement("div");

        let notesStr = step.notes.map(n => `${n.pitch}(${n.velocity})`).join(", ");

        div.innerHTML = `Step ${i}: ${notesStr}`;
        div.style.padding = "5px";

        container.appendChild(div);
    });
}