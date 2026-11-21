# Contributing

Hey, thanks for stopping by. This is a small project with a small team (just me), so the bar is low and the rules are few. Stick to them and we'll get along great.

## Found a bug?

Open an [issue](https://github.com/diode701/nyaapaper/issues/new). Tell me:

- your OS, desktop environment, Java version
- what you were doing
- what you expected to happen
- what actually happened
- if the app froze, what heap size you were using

That's it. I'll take it from there.

## Have an idea?

Open an issue with the `enhancement` label. Don't just pitch a feature — tell me what problem it solves for you. Concise, focused proposals land faster, and I'm more likely to say yes to something I can see the use case for.

## Want to write code?

Hell yeah. Here's the workflow:

1. Fork the repo, branch from `main`
2. Write your code
3. Run `./gradlew buildAll` — if it doesn't compile, it doesn't ship
4. Open a PR with a clear title and a short description of what and why

### A few things to keep in mind

- **Java only.** No new language runtimes, no Python, no Rust, no shell scripts in the source tree. Everything lives on the classpath inside one fat JAR. That's the whole point.

- **Proper OOP, please.** This isn't a 50-line script — keep it tidy. Single-responsibility classes, sensible abstractions, no god objects. If you're adding something new, think about where it lives before you start typing. The package layout is there for a reason:

  ```
  com.nyaa.config.*      — configuration, persistence, settings files
  com.nyaa.settings.*    — property parsing, global options, UI controls
  com.nyaa.wallpaper.*   — engine process management, GPU config, lifecycle
  com.nyaa.*             — UI views and app wiring (browser, properties, etc.)
  ```

  If a class is doing two unrelated things, split it. If a method is growing past what fits on one screen, refactor it. Future-you (and me) will thank you.

- **JavaFX for the UI, plain Java for everything else.** Don't mix concerns. Business logic doesn't belong inside event handlers. Hardware queries don't belong inside a Node constructor.

- **Keep the fat JAR philosophy.** If you need a new library, argue for it in the PR description — I'll listen, but the default is no.

### Commit messages

Write them like you're telling a coworker what changed in five seconds. A prefix helps when it fits:

- `parser:` — property parser changes
- `ui:` — layout, visuals, controls
- `playlist:` — playlist system
- `config:` — persistence, settings
- `fix:` — bug
- `docs:` — README, CHANGELOG, comments

## Got questions?

Open a [Discussion](https://github.com/diode701/nyaapaper/discussions). No question is too small — I'd rather answer a silly question than watch you bang your head against something for an hour.

Or just open an issue. I'll see it.
