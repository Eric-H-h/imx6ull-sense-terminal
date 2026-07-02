# 02 Camera Capture

## Goal

Capture at least one real frame from OV5640 or USB UVC.

## Device Discovery

```sh
v4l2-ctl --list-devices
v4l2-ctl -d /dev/videoX --list-formats-ext
```

## First Frame

```sh
v4l2-ctl -d /dev/videoX --stream-mmap --stream-count=1 --stream-to=frame.yuv
```

## Results

TBD.

