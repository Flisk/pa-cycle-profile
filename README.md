# pa-cycle-profile

A simple program for cycling the active profile of a PulseAudio card.

Ideal for use as a key binding in a tiling WM when you use the digital
and analog output profiles of one card for different output devices.

## Usage

```
$ pa-cycle-profile <card name> <profile 1> <profile 2> ...
```

An arbitrary number of profiles may be specified. Each invocation
switches to the next profile in left-to-right order and wraps around
after the last profile.

The name of your card and a list of available profiles can be found in
the output of `pacmd list`.

## Dependencies

* pkg-config
* libpulse

## Installing

NixOS:

```
$ nix-env -if .
```

Other distros:

```
$ make
# sudo make install
```

## Uninstalling

NixOS:

```
$ nix-env -e pa-cycle-profile
```

Other distros:

```
$ make
# sudo make install
```
