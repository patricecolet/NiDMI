async function readMidiFile(file) {
    const arrayBuffer = await file.arrayBuffer();
    const bytes = new Uint8Array(arrayBuffer);

    console.log(bytes); // tableau d’octets
    return bytes;
}