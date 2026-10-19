# Security policy

## Supported versions

Security fixes go into the latest release and `main`. Older releases are not patched.

## Reporting a vulnerability

Do not open a public issue for a suspected vulnerability. Use GitHub's private reporting instead: [**Report a vulnerability**](https://github.com/RandyNorthrup/meta-g-link-direct/security/advisories/new) (also under the repository's *Security* tab).

Include:

- the affected version or commit;
- the steps to reproduce it and the privileges they need;
- what you expected and what happened;
- the impact, for example on the glasses' ownership, the stored identity, the Wi-Fi link or OBS.

Remove secrets and personal data first: no owner keys, tokens, Meta login addresses or codes, Wi-Fi passwords, device addresses, raw protocol captures or camera footage.

Reports about the Bluetooth or Wi-Fi session encryption, the ownership exchange, the DPAPI-sealed identity, protocol parsing of data from the glasses, or the installer's elevation get priority.

## Known gaps

These are already tracked and do not need a report:

- The PC does not yet verify the glasses' trust signature during the handshake.
- The release binaries are not code-signed. The `.sha256` file published with each release checks the download's integrity, not who published it.
- The Meta AI app's two public client tokens are built into the setup tool on purpose. They identify the app, not a person (see [AGENTS.md](AGENTS.md), revision 3).
