// MOAG image7B11. This is the general TWR pane in the theater browser,
// distinct from TOWER.EXE mission shortcuts and generated towers.inf.
export const NO_GENERAL_INFORMATION = [
    'There is no general information available',
    'about this Theater of Operations.  You',
    'may consult the per-mission information',
    'before flying or obtain more information',
    'by pressing <F2> during flight.'
];
export function generalInformation(text) {
    if (text === null || text === undefined) return [...NO_GENERAL_INFORMATION];
    let at = 0;
    // Actual reader163E9 uses fgets(size128), then removes at most two CR/LF.
    const read = () => {
        if (at === text.length) return null;
        let end = Math.min(at + 127, text.length);
        const lf = text.indexOf('\n', at);
        if (lf >= at && lf < end) end = lf + 1;
        let line = text.slice(at, end); at = end;
        for (let i = 0; i < 2; ++i) if (/[\r\n]$/.test(line)) line = line.slice(0, -1);
        return line;
    };
    const header = read();
    if (!header || header[0] === '\f') return [...NO_GENERAL_INFORMATION];
    const lines = [];
    for (;;) {
        const line = read();
        if (line === null || line[0] === '\f' || lines.length === 8) break;
        if (!lines.length && !line) return [...NO_GENERAL_INFORMATION];
        lines.push(line.slice(0, 41));
    }
    return lines.length ? lines : [...NO_GENERAL_INFORMATION];
}
export function theaterGeneralInformation(briefings, stem) {
    const section = briefings.theaters.find(t => t.filename.toLowerCase() === `${stem}.twr`.toLowerCase())?.sections.find(s => s.tower_index === null);
    // Extractor retains the newline before the section's form-feed as its
    // last empty split line; reconstruct it without adding a visible row.
    return generalInformation(section ? section.lines.join('\n') + '\f\n' : null);
}
