# Build Instructions

## One-click Build

```bash
python scripts/release.py bread-compact-wifi-lcd -c config.json
```

## Manual Configuration and Build

```bash
idf.py set-target esp32s3
```

**Configuration**

```bash
idf.py menuconfig
```

Select the board:

```
Xiaozhi Assistant -> Board Type -> CompactWifiBoardLCD
```

## Build and Flash

```bash
idf.py -DBOARD_NAME=bread-compact-wifi-lcd build flash
```

Note: 