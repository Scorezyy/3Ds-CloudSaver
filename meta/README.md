# Meta Assets

Place the following files here for CIA building:

## icon.png
- **Size:** 48×48 pixels
- **Format:** PNG
- Used as the app icon on the 3DS home menu

## banner.png (optional)
- **Size:** 256×128 pixels
- **Format:** PNG
- Shown as the banner on the top screen when selecting the app

## audio.wav (optional)
- **Format:** WAV, 16-bit PCM
- **Duration:** ~3 seconds
- Played when selecting the app on the home menu

## Generating icon.icn and banner.bnr

Use [bannertool](https://github.com/Steveice10/bannertool):

```bash
bannertool makesmdh \
  -s "3DS CloudSaver" \
  -l "Cloud Save Manager for 3DS" \
  -p "Jxstn" \
  -i icon.png \
  -o icon.icn

bannertool makebanner \
  -i banner.png \
  -a audio.wav \
  -o banner.bnr
```
