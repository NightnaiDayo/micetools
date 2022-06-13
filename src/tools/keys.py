import json

patches = []
patchset = {
    "name": "Rebind keys",
    "description": "Automatically generated",
    "apply": True,
    "patches": patches
}

original = ['W', 'E', 'D', 'C', 'X', 'Z', 'A', 'Q']

base = 1517787
for i in range(8):
    key = input(f"Key for {i+1}: ")
    if len(key) != 1:
        print("That's not one key!")
        quit()

    key = f"{ord(key.upper()):02x}"
    orig = f"{ord(original[i].upper()):02x}"

    if i == 0:
        patches.append({
            "at": base,
            "from": f"c70424{orig}000000",
            "to": f"c70424{key}000000",
        })
    else:
        instr = f"c74424{4 * i:02x}"
        patches.append({
            "at": base + 7 + 8 * (i - 1),
            "from": instr + orig + "000000",
            "to": instr + key + "000000",
        })

with open("keybinds.json", "w") as f:
    json.dump([patchset], f)

print("Wrote patches to keybinds.json")
