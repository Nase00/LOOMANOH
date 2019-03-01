# **LOO**k **MA NO H**ands

## RJ45 Pin Configuration

`G-`

`G+`

`1`

`2`

`3`: Ground

`4`

`5`

`6`: Preset, GPIO-18/P12 (Blue)

`7`: Lower, GPIO-27/P13 (Green)

`8`: Raise, GPIO-22/P15 (Yellow)

`Y-`

`Y+`

## Installing Muselsl

This steps are probably incomplete. I only got Muselsl working on the Pi after several attempts and re-attempts using information like [this](https://github.com/alexandrebarachant/muse-lsl/issues/1). I started with a fresh copy of `2018-11-13-raspbian-stretch.img` on a Raspberry Pi 3 Model B+.

```bash
# Preinstall dependencies for muselsl instead of letting Python compile them for ages.
# If this errors on packages not found, remove said packages and try the command again.
sudo apt-get install python-bitstring python-numpy python-pandas python-pexpect python-seaborn python-ptyprocess python-scipy python-matplotlib python-six python-enum34 python-backports.functools-lru-cache python-cycler python-pyparsing python-subprocess32 python-setuptools

# Might also work with `pip`. The behavior of these commands were extremely inconsistent.
pip3 install muselsl

# Replace the x86-compiled liblsl32.so with the arm7-compiled version.
cd ~/.local/lib/python2.7/site-packages/pylsl && wget https://github.com/chkothe/pylsl/raw/8d1a975ea66899d0fb7d58ebf320d5422f5a274c/pylsl/liblsl32armv7l.so
mv liblsl32armv7l.so liblsl32.so

# I think this fixes which Bluetooth module is used to scan for the Muse, or something.
sudo setcap 'cap_net_raw,cap_net_admin+eip' `which hcitool`

pip install pygatt==3.1.1 # or pip3 install or sudo pip install or whatever fucking path python uses

# if muselsl command does not work, try `/home/pi/.local/bin/muselsl`.
```

## Libraries

- [ESP8266/ESP32 WebServer](https://github.com/bbx10/WebServer_tng/tree/8491a56cc4090f7f4f0edfc95c5bf9f6049d85cd)

- [Micro Dot PHAT Python](https://github.com/pimoroni/microdot-phat)

- [FastLED NeoMatrix](https://github.com/marcmerlin/FastLED_NeoMatrix)

## References

- [Adafruit Blog - "Recording Brainwaves with a Raspberry Pi"](https://blog.adafruit.com/2018/05/28/recording-brainwaves-with-a-raspberry-pi/)

## Trouble-shooting

Grab the ARM-compiled liblsl:

```sh
cd /usr/local/lib/python3.5/dist-packages/pylsl/
sudo wget https://github.com/chkothe/pylsl/raw/8d1a975ea66899d0fb7d58ebf320d5422f5a274c/pylsl/liblsl32armv7l.so
# https://github.com/chkothe/pylsl/pull/2
```

https://github.com/alexandrebarachant/muse-lsl/issues/1#issuecomment-338481076
