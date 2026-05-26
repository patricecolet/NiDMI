async function uploadFile() {
    const file = document.getElementById('nidmidFile').files[0];
    if (!file) return alert("Select file");

    const buffer = await file.arrayBuffer();

    await fetch('/api/sequencer/load', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/octet-stream'
        },
        body: buffer
    });

    console.log("✅ Uploaded");

    loadView();
}