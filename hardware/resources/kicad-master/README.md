KiCad 5 symbols, footprints and associated 3D packages

# List of symbols / footprints
[Documentation](documentation)

# Installing
```bash
mkdir -p $HOME/.local/share/
cd $HOME/.local/share/
git clone https://gitlab.com/VictorLamoine/kicad.git
```

Add the following variables in KiCad: `Preferences` > `Configure paths`:

- `VL_FOOTPRINTS`: `/home/victor/.local/share/kicad/footprints`
- `VL_PACKAGES3D`: `/home/victor/.local/share/kicad/packages3d`
- `VL_SYMBOLS`: `/home/victor/.local/share/kicad/symbols`

## Adding a symbol library
- `Preferences` > `Manage symbol libraries`
- `Global Libraries` tab

Add an entry for each library, for example:

| Nickname | Library path |
|-|-|
| `ESP32_DevKit_V1_DOIT` | `${VL_SYMBOLS}/esp32_devkit_v1_doit.lib` |

## Adding a footprint library
- `Preferences` > `Manage footprint libraries`
- `Global Libraries` tab

Add an entry for each library, for example:

| Nickname | Library path |
|-|-|
| ESP32_DevKit_V1_DOIT | ${VL_FOOTPRINTS}/esp32_devkit_v1_doit.pretty |
