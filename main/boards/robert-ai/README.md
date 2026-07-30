# Build Instructions

## One-click Build

```bash
python scripts/build.py robert-ai
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
Xiaozhi Assistant -> Board Type -> Robert
```

## Build and Flash

```bash
idf.py -DBOARD_NAME=robert-ai build flash
```

Note: 