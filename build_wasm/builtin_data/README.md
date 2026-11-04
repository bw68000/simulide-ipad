# Built-in component sets (from SimulIDE 1.1.0-SR2)

Component sets that SimulIDE 2.0's installer does not offer, copied from the
`data` folder of SimulIDE 1.1.0-SR2 (GPLv3, https://simulide.com).
The web build zips this folder to `builtin/data.zip`; the app installs it
on first start (and again after each new build).

Changes from the original files:
- `icCD.xml`: CD4521 removed (its files are missing in 1.1.0-SR2).
- `unsorted.xml`: LM555 removed (SimulIDE 2.0 has its own LM555).
- `scripted` set left out (its "DAC" replaced SimulIDE 2.0's DAC).
