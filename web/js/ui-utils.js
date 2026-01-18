/**
 * @file ui-utils.js
 * @brief Utilitaires UI génériques
 */

/**
 * Remplit un élément select avec des options
 * @param {HTMLElement} sel - Élément select
 * @param {Object|Array} options - Options (objet clé-valeur ou tableau)
 * @param {number|string} pre - Index ou valeur présélectionnée (défaut: 0)
 */
function setOptions(sel, options, pre = 0) {
  if (!sel) return;
  let html = '';
  let firstValue = null;
  let selectedValue = null;
  
  if (Array.isArray(options)) {
    html = options.map((o, i) => {
      if (i === 0) firstValue = o;
      if (i === pre) selectedValue = o;
      return `<option ${i === pre ? 'selected' : ''}>${o}</option>`;
    }).join('');
  } else {
    let isFirst = true;
    Object.keys(options).forEach(groupKey => {
      const group = options[groupKey];
      if (typeof group === 'string') {
        if (firstValue === null) firstValue = groupKey;
        const shouldSelect = (pre === 0 && isFirst) || (typeof pre === 'string' && groupKey === pre);
        if (shouldSelect) selectedValue = groupKey;
        html += `<option value="${groupKey}" ${shouldSelect ? 'selected' : ''}>${group}</option>`;
        if (isFirst) isFirst = false;
      } else if (typeof group === 'object' && group.label) {
        /* Objet avec label et disabled */
        /* Ne pas sélectionner les éléments désactivés comme première valeur */
        if (firstValue === null && !group.disabled) firstValue = groupKey;
        const shouldSelect = (pre === 0 && isFirst && !group.disabled) || (typeof pre === 'string' && groupKey === pre);
        if (shouldSelect) selectedValue = groupKey;
        const disabled = group.disabled ? 'disabled' : '';
        html += `<option value="${groupKey}" ${shouldSelect ? 'selected' : ''} ${disabled}>${group.label}</option>`;
        if (isFirst && !group.disabled) isFirst = false;
      } else if (group.items && Array.isArray(group.items)) {
        html += `<optgroup label="${group.label}">`;
        group.items.forEach((item) => {
          const selected = (typeof pre === 'string' && item.value === pre) ? 'selected' : '';
          if (selected) selectedValue = item.value;
          const disabled = item.disabled ? 'disabled' : '';
          html += `<option value="${item.value}" ${selected} ${disabled}>${item.label}</option>`;
        });
        html += `</optgroup>`;
      } else {
        if (firstValue === null) firstValue = groupKey;
        const shouldSelect = (pre === 0 && isFirst) || (typeof pre === 'string' && groupKey === pre);
        if (shouldSelect) selectedValue = groupKey;
        html += `<option value="${groupKey}" ${shouldSelect ? 'selected' : ''}>${group}</option>`;
        if (isFirst) isFirst = false;
      }
    });
  }
  
  sel.innerHTML = html;
  /* Définir explicitement sel.value après avoir mis le HTML */
  if (selectedValue !== null) {
    sel.value = selectedValue;
  } else if (firstValue !== null) {
    sel.value = firstValue;
  }
}
