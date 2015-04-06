# pdf-fill-form
Fill PDF forms and return either filled PDF or PDF created from rendered page images.

Homebrew users who get error regarding xcb-shm

The fix is to add this to your bash profile / environment:
export PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig