# gdwlroots

This is a [Godot](https://godotengine.org/) module which provides bindings to
[wlroots](https://github.com/swaywm/wlroots), for building Wayland compositors
within Godot.

## Installation

In order to use this module, you must compile Godot from source with gdwlroots.
Refer to the [upstream instructions][compiling-godot] for additional
information. To add gdwlroots, check out the git repository into the appropriate
place:

[compiling-godot]: https://docs.godotengine.org/en/latest/development/compiling/compiling_for_x11.html

	git clone https://git.sr.ht/~sircmpwn/gdwlroots modules/gdwlroots/

This will make the gdwlroots nodes available in the Godot editor.

## Design

gdwlroots attempts to provide bindings between gdscript and wlroots that export
the wlroots API as faithfully as possible. Most wlroots types are provided via
the Godot ClassDB with wl_signals rigged up as Godot signals, getters/setters
for memebers of wlroots structs, and methods which correspond to wlroots
functions. Some knowledge of wlroots is recommended for using this module.

## godotston

An example godot project which implements a simple Wayland compositor with
gdwlroots is available in the godotston directory.
