# Security reports

Do not post credentials, private files, personal save exports or sensitive exploit details in a public issue. If GitHub's **Security → Report a vulnerability** button is available, use that private channel. Otherwise open an issue saying only that you need a private contact route; include no sensitive details until one is arranged. Private vulnerability reporting may require the repository owner to enable it.

Include the affected commit/URL, browser, reproduction steps, expected/actual result and a minimal synthetic test file when possible. No response SLA or bug bounty is promised.

Please reproduce resource exhaustion, malformed files and other potentially disruptive cases on your local build. Do not conduct load tests or broad scans against the public game.

Files selected through the game UI are read locally. IndexedDB stores careers and boss-screen text. Exporting a save includes boss-screen text. The deployment policy is in `web/_headers`; additional scripts, origins, APIs or embeds require a policy review.

The September 2026 review repaired import resource bounds and added browser hardening. It was a scoped engineering review, not a security certification. Cloudflare account settings and ongoing production monitoring are separate responsibilities.
