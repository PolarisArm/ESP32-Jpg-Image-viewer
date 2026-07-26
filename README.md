# ESP32 JPG Image Viewer

An ESP32-based image viewer that reads `.JPG` and `.JPEG` files from an SD card and displays them on a TFT screen.

The project provides a simple file browser interface controlled by four physical buttons. The user can navigate through the images, select an image, and display it on the TFT screen. A scrolling menu system is implemented so that only a fixed number of files are displayed at a time.

## Features

* ESP32-based JPG/JPEG image viewer
* Reads images directly from an SD card
* Supports `.jpg` and `.jpeg` image files
* Displays images on a TFT display
* JPEG decoding using `JPEGDEC`
* SD card access using the ESP32 `SD` library
* Hardware button navigation
* Scrollable file menu
* Displays up to 6 files at a time
* Highlights the currently selected file
* Opens the selected image in full-screen mode
* Back button returns to the file browser
* Efficient visible-item loading system

## Hardware Requirements

* ESP32 development board
* TFT display compatible with `TFT_eSPI`
* SD card module or SD card interface
* MicroSD card
* 4 push buttons
* Jumper wires

## Libraries

The project requires the following Arduino libraries:

* `Arduino.h`
* `TFT_eSPI`
* `SPI`
* `FS`
* `SD`
* `JPEGDEC`

Make sure the TFT display is correctly configured in the `TFT_eSPI` library configuration before compiling the project.

## Button Configuration

The current button pin configuration is:

| Button | ESP32 GPIO |
| ------ | ---------: |
| UP     |    GPIO 27 |
| DOWN   |    GPIO 26 |
| OK     |    GPIO 25 |
| BACK   |    GPIO 33 |

The buttons are configured as digital inputs.

```cpp
#define UP 27
#define DW 26
#define OK 25
#define BK 33
```

The program expects the buttons to be active LOW. When a button is pressed, `digitalRead()` returns `LOW`.

## SD Card Configuration

The SD card is initialized using GPIO 5 as the Chip Select (CS) pin:

```cpp
SD.begin(5, tft.getSPIinstance());
```

The SPI instance used by the SD card is obtained from the TFT display:

```cpp
tft.getSPIinstance()
```

The program checks:

1. Whether the SD card was successfully mounted.
2. Whether an SD card is physically attached.
3. The type of SD card.
4. The total SD card capacity.

Supported card types reported by the program include:

* MMC
* SDSC
* SDHC
* UNKNOWN

## Display Menu

The viewer displays a maximum of 6 files at once:

```cpp
#define VISIBLE_ITEMS 6
```

The file menu uses a scrolling-window system.

For example, if the SD card contains:

```text
/image01.jpg
/image02.jpg
/image03.jpg
/image04.jpg
/image05.jpg
/image06.jpg
/image07.jpg
/image08.jpg
/image09.jpg
```

Initially, the screen displays:

```text
> image01.jpg
  image02.jpg
  image03.jpg
  image04.jpg
  image05.jpg
  image06.jpg
```

When the user presses DOWN, the selection moves through the visible items.

When the selection reaches the bottom of the visible window, the menu scrolls:

```text
  image02.jpg
  image03.jpg
  image04.jpg
  image05.jpg
  image06.jpg
> image07.jpg
```

The program does not store every filename in the `visibleName` array. Instead, it only loads the files currently visible on the screen.

## Navigation State

The navigation system is controlled by the following structure:

```cpp
struct DirectoryState {
  int totalFiles = 0;
  int selectedIndex = 0;
  int topIndex = 0;
} nav;
```

### `totalFiles`

Stores the total number of JPG/JPEG files found on the SD card.

For example:

```text
image01.jpg
image02.jpg
image03.jpg
image04.jpg
image05.jpg
image06.jpg
image07.jpg
```

The value of `totalFiles` is:

```text
7
```

### `selectedIndex`

Stores the global index of the currently selected image.

For example:

```text
image01.jpg -> 0
image02.jpg -> 1
image03.jpg -> 2
image04.jpg -> 3
```

### `topIndex`

Stores the global index of the first file currently visible on the screen.

For example:

```text
topIndex = 2
```

means the visible window starts at the third JPG/JPEG file.

## Visible Filename Buffer

The program stores only the currently visible filenames:

```cpp
char visibleName[VISIBLE_ITEMS][32];
```

Since:

```cpp
VISIBLE_ITEMS = 6
```

the program stores up to 6 filenames at a time.

This reduces the amount of RAM required compared with storing every filename from the SD card.

The `loadVisible()` function scans the SD card directory and loads the filenames starting from `nav.topIndex`.

## File Counting

The `loopingFile()` function scans the root directory of the SD card and counts JPG files.

It checks whether the filename ends with `.jpg`:

```cpp
strcasecmp(name + len - 4, ".jpg") == 0
```

The total number of matching files is returned and stored in:

```cpp
nav.totalFiles = loopingFile();
```

This value is used to control the navigation boundaries.

## Loading Visible Files

The `loadVisible()` function loads only the filenames needed by the current menu window.

The process is:

1. Open the root directory.
2. Start scanning files.
3. Ignore directories.
4. Check for JPG/JPEG files.
5. Count matching image files.
6. Skip files before `nav.topIndex`.
7. Copy the next visible filenames into `visibleName`.
8. Stop after `VISIBLE_ITEMS` files are loaded.

Conceptually:

```text
SD Card
│
├── image01.jpg
├── image02.jpg
├── image03.jpg
├── image04.jpg
├── image05.jpg
├── image06.jpg
├── image07.jpg
├── image08.jpg
└── image09.jpg

              topIndex
                 │
                 ▼
         ┌─────────────────┐
         │ image04.jpg     │
         │ image05.jpg     │
         │ image06.jpg     │
         │ image07.jpg     │
         │ image08.jpg     │
         │ image09.jpg     │
         └─────────────────┘
              6 visible
```

## Menu Rendering

The `drawMenu()` function clears the display and draws the complete visible menu.

```cpp
void drawMenu() {
  tft.fillScreen(TFT_BLACK);

  if (nav.totalFiles == 0) {
    tft.drawString("NO Jpg or JPEG file found", 20, 50, 4);
    return;
  }

  loadVisible();

  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    drawMenuItem(i);
  }
}
```

Each individual menu item is drawn by:

```cpp
drawMenuItem(i);
```

The selected item is highlighted with a navy background.

Unselected items have a black background and navy border.

## Selection and Visible Index

The program maintains two different indexes:

* Global index: `nav.selectedIndex`
* Local visible index: `visibleIndex`

The global index identifies the file within the entire SD card file list.

The local index identifies the file's position on the current screen.

The relationship is:

```cpp
localIndex = nav.selectedIndex - nav.topIndex;
```

For example:

```text
selectedIndex = 8
topIndex      = 5

localIndex = 8 - 5
           = 3
```

Therefore, the selected file is the fourth item on the screen:

```cpp
visibleName[3]
```

This is important because `visibleName[]` only contains the currently visible files.

## UP Button

When the UP button is pressed:

```cpp
nav.selectedIndex--;
```

If the selected file moves above the current visible window, `topIndex` is updated:

```cpp
if (nav.selectedIndex < nav.topIndex) {
  nav.topIndex = nav.selectedIndex;
  loadVisible();
}
```

The visible filenames are then reloaded and the menu is redrawn.

If the selected item is still inside the visible window, only the old and new selection states are redrawn.

This avoids unnecessarily redrawing the entire menu.

## DOWN Button

When the DOWN button is pressed:

```cpp
nav.selectedIndex++;
```

If the selection moves beyond the last visible item:

```cpp
if (nav.selectedIndex >= nav.topIndex + VISIBLE_ITEMS)
```

the visible window moves down:

```cpp
nav.topIndex = nav.selectedIndex - VISIBLE_ITEMS + 1;
```

The visible filenames are then reloaded.

For example, with six visible items:

```text
topIndex = 0
selectedIndex = 5
```

The screen contains:

```text
0
1
2
3
4
5
```

Pressing DOWN gives:

```text
selectedIndex = 6
```

Since index 6 is outside the current visible range, the new top index becomes:

```text
topIndex = 6 - 6 + 1
         = 1
```

The screen now displays:

```text
1
2
3
4
5
6
```

This creates a scrolling file browser.

## Opening an Image

When the OK button is pressed, the program converts the global selection index to a local visible index:

```cpp
int localIndex = nav.selectedIndex - nav.topIndex;
```

Then it gets the selected filename:

```cpp
char* selectedPath = visibleName[localIndex];
```

The JPEG decoder opens the file using custom SD card callbacks:

```cpp
jpg.open(
  (char*)selectedPath,
  myOpen,
  myClose,
  myRead,
  mySeek,
  (JPEG_DRAW_CALLBACK*)JPEGDraw
);
```

The image is decoded with:

```cpp
jpg.decode(0, 0, 0);
```

The program then changes to image-viewer mode:

```cpp
screen = 1;
```

## JPEG File Callbacks

The `JPEGDEC` library uses custom callbacks to access image data from the SD card.

### Open

```cpp
myOpen()
```

Opens the selected image from the SD card and returns its size.

### Read

```cpp
myRead()
```

Reads image data from the currently opened SD file.

### Seek

```cpp
mySeek()
```

Moves the SD file pointer to a requested position.

### Close

```cpp
myClose()
```

Closes the currently opened image file.

This allows `JPEGDEC` to decode an image directly from the SD card.

## JPEG Drawing Callback

The JPEG decoder provides decoded pixel data to:

```cpp
JPEGDraw()
```

The decoded image data is sent directly to the TFT display:

```cpp
tft.pushImage(
  pDraw->x,
  pDraw->y,
  pDraw->iWidth,
  pDraw->iHeight,
  pDraw->pPixels
);
```

The image is therefore rendered in blocks as the JPEG decoder processes it.

The pixel format is configured as:

```cpp
jpg.setPixelType(RGB565_BIG_ENDIAN);
```

## Screen States

The program uses the `screen` variable to determine the current UI state.

### Screen 0 — File Browser

```cpp
screen = 0;
```

The user can:

* Press UP to move the selection up.
* Press DOWN to move the selection down.
* Press OK to open the selected image.

### Screen 1 — Image Viewer

```cpp
screen = 1;
```

The selected image is displayed on the TFT.

Pressing the BACK button returns to the file browser:

```cpp
screen = 0;
```

The menu is then redrawn.

## Program Flow

The overall program flow is:

```text
             ESP32 Starts
                  │
                  ▼
            Initialize TFT
                  │
                  ▼
            Initialize SD Card
                  │
                  ▼
          Count JPG/JPEG Files
                  │
                  ▼
            Load Visible Files
                  │
                  ▼
             Draw Menu
                  │
                  ▼
              User Input
                  │
       ┌──────────┼──────────┐
       │          │          │
       ▼          ▼          ▼
      UP         DOWN        OK
       │          │          │
       ▼          ▼          ▼
   Move Up    Move Down   Open Image
       │          │          │
       └──────────┼──────────┘
                  │
                  ▼
          Update Menu Window
                  │
                  ▼
             Display Image
                  │
                  ▼
             Press BACK
                  │
                  ▼
              Draw Menu
```

## Important Notes

The current implementation scans the SD card directory whenever the visible menu window needs to be refreshed. This approach keeps RAM usage low because only six filenames are stored.

However, the SD card directory is scanned repeatedly when scrolling. For a small number of images this is usually acceptable, but with a very large number of images, navigation may become slower.

The filename buffer is currently limited to 31 characters plus the null terminator:

```cpp
char visibleName[VISIBLE_ITEMS][32];
```

Therefore, filenames or paths longer than 31 characters may be truncated.

The program currently searches the root directory:

```cpp
SD.open("/");
```

It does not recursively search inside subdirectories.

## Future Improvements

Possible improvements include:

* Add support for nested directories.
* Add a thumbnail preview mode.
* Display image dimensions.
* Display the current file number, such as `3 / 25`.
* Add a loading screen while decoding large images.
* Add automatic image scaling to fit the TFT display.
* Add LEFT/RIGHT buttons for previous/next image.
* Add image rotation.
* Add slideshow mode.
* Cache the file list for faster navigation.
* Sort files alphabetically.
* Support more JPEG extensions.
* Improve long filename handling.
* Add a progress indicator while loading images.
* Add touch-screen support.

## License

This project is provided for personal, educational, and hobbyist use. Modify and distribute it according to the licenses of the third-party libraries used by the project.
