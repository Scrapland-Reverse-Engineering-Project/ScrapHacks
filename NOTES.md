# Infos
- Engine: ScrapEngine
- Ingame Scripting Language: Python 1.5.2

# Ingame-Console (Ctrl+\^) (Handler@0x42190):
* ":\<Var>":
 - Get Game Engine Global Variable
* ":\<Var> \<Val>"
 - Set Game Engine Global Variable
* "?":
 - Show all Global Variable
* "?\<String>":
 - Show all Global Variable matching <String>
* "/\<command>":
 - Run Command defined in QuickConsole.py(c) 'import quickconsole;quickconsole.%s()'
* "/\<command> \<arg>,\<arg>":
 - Run function in QuickConsole.py(c) with argument(s) 'import quickconsole;quickconsole.%s(%s)'

# External Console (Scenegraph Debugging?) (Handler@0x5f9520):
* "listar luces"
* "listar"
* "arbol" # Patch Scrap.exe@offset 0x314bc0 replace 0x20 with 0x00 (or just type "arbol " with the space at the end)
* "mem"
* "ver uniones"
* Easter Eggs:
 - "imbecil"
 - "idiota"
 - "capullo"

## m3d.ini loader @0x05f7000

## *.packed File Format:
    Header:
        "BFPK\0\0\0\0"
        Int32ul: number of files
        for each file:
            Int32ul: path length
            String: path
            Int32ul: size
            Int32ul: offset in file

## Interesting file:
* m3d.ini: Rendering Engine Configuration
* scripts/: Game Engine Scripts
* scripts/: Game Engine Scripts

# How to enable External Console:
1. exctract `Data.packed`
2. in m3d.ini uncomment "ConsolaWnd" (GUI Console) or "ConsolaTxt" (Text Console) and set the value to "SI"

# Other Interesting things
- sys.path contains "./lib" so you can load your own Python Modules