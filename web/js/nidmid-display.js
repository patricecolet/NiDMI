/**
 * Charge et affiche l'aperçu de la séquence actuelle
 */
async function loadView() {
    const container = document.getElementById('seqView');
    if (!container) {
        return;
    }

    try {
        const res = await fetch('/api/sequencer/view');
        if (!res.ok) {
            container.innerHTML = '<div class="seq-empty">Aperçu indisponible.</div>';
            return;
        }

        const data = await res.json();
        renderSteps(Array.isArray(data.steps) ? data.steps : []);
    } catch (error) {
        console.error('[loadView] Erreur:', error);
        container.innerHTML = '<div class="seq-empty">Erreur de lecture de la séquence.</div>';
    }
}

/**
 * Affiche les étapes/notes dans la console
 */
function renderSteps(steps) {
    if (!steps || steps.length === 0) {
        console.log('[renderSteps] Aucune séquence chargée');
        return;
    }

    console.log('=== Sequence chargee ===');

    let currentMeasure = -1;
    let measureCount = 0;

    steps.forEach((step, index) => {
        if (step.measure !== currentMeasure) {
            currentMeasure = step.measure;
            measureCount++;
            console.log(`\n--- Mesure ${step.measure} ---`);
        }

        let notesStr = '';
        if (step.notes && step.notes.length > 0) {
            notesStr = step.notes
                .map(n => `P:${n.pitch} V:${n.velocity}`)
                .join(', ');
        } else {
            notesStr = '—';
        }

        console.log(`Step ${index}: ${notesStr}`);
    });

    console.log('\n=== Fin de la sequence ===');
}
