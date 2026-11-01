# Panoramize — Architecture decision register

Record decisions with ID, status (proposed/accepted/superseded), rationale, evidence and related commit/issue.

## ADR-001 — GitHub as canonical engineering record
Status: accepted for development coordination, 2026-11-02.
Use GitHub for versioned code, verified specifications, decisions and test results. Notion holds exploratory research and links to canonical documents; Linear holds actionable issues. Do not independently maintain conflicting authoritative copies.

## ADR-002 — Research tools are optional, not build dependencies
Status: accepted, 2026-11-02.
Scite, Firecrawl and Context7 accelerate research, but the code and evidence must remain usable after a subscription or connection ends. Save DOI/URLs, summaries in original words, design implications and unresolved questions; do not copy restricted full text.

## ADR-003 — No claims of audio quality without evidence
Status: accepted, 2026-11-02.
Distinguish theoretical plausibility, compilation, unit tests, plugin validation, DAW loading, acoustic measurement and subjective listening. Each is a separate milestone.
